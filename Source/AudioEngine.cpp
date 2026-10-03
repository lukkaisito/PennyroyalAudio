#include "AudioEngine.h"
#include "Lang.h"
#include <cmath>
#include <algorithm>
#include <map>

AudioEngine::AudioEngine()  = default;
AudioEngine::~AudioEngine() { shutdown(); }

// ============================================================
//  Device management
// ============================================================
void AudioEngine::initialise()
{
    auto err = deviceManager.initialiseWithDefaultDevices(1, 2);
    if (err.isNotEmpty())
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, TR("Audio Device Error"), err);
    deviceManager.addAudioCallback(this);
}

void AudioEngine::shutdown()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

void AudioEngine::setBufferSize(int samples)
{
    auto* device = deviceManager.getCurrentAudioDevice();
    if (!device) return;
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    setup.bufferSize = samples;
    deviceManager.setAudioDeviceSetup(setup, true);
}

// ============================================================
//  Tracks
// ============================================================
Track* AudioEngine::addTrack(const juce::String& name)
{
    auto dsp = std::make_unique<TrackDSP>();
    dsp->prepare(deviceSampleRate);          // allocate outside the audio thread

    const juce::ScopedLock sl(audioLock);
    auto* t = project.addTrack(name);
    liveDSP.push_back(std::move(dsp));
    clearUndoHistory();
    return t;
}

void AudioEngine::removeTrack(int index)
{
    const juce::ScopedLock sl(audioLock);
    if (index < 0 || index >= (int)project.tracks.size()) return;
    project.removeTrack(index);
    if (index < (int)liveDSP.size())
        liveDSP.erase(liveDSP.begin() + index);
    if (recordingTrackIndex == index) recordingTrackIndex = -1;
    clearUndoHistory();
}

void AudioEngine::syncDSPPool(DSPPool& pool, double sr)
{
    while (pool.size() < project.tracks.size())
    {
        auto dsp = std::make_unique<TrackDSP>();
        dsp->prepare(sr);
        pool.push_back(std::move(dsp));
        auto& t = *project.tracks[pool.size() - 1];
        t.eqLow.dirty = true; t.eqMid.dirty = true; t.eqHigh.dirty = true;
    }
    while (pool.size() > project.tracks.size())
        pool.pop_back();
}

// ============================================================
//  Transport
// ============================================================
void AudioEngine::play()
{
    const juce::ScopedLock sl(audioLock);
    if (transportState == TransportState::Stopped)
        transportState = TransportState::Playing;
}

void AudioEngine::pause()
{
    const juce::ScopedLock sl(audioLock);
    if (transportState == TransportState::Playing)
        transportState = TransportState::Stopped;
}

void AudioEngine::stop()
{
    const juce::ScopedLock sl(audioLock);
    stopRecording();
    transportState = TransportState::Stopped;
    playheadSample = project.loopEnabled ? (juce::int64)(project.loopStart * deviceSampleRate) : 0;
    sendChangeMessage();
}

void AudioEngine::startRecording(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= (int)project.tracks.size())
        return;

    maxRecordSamples = (int)(deviceSampleRate * 300.0);   // 5 minutes
    auto buf = std::make_unique<juce::AudioBuffer<float>>(1, maxRecordSamples);
    buf->clear();
    std::vector<float> pMin((size_t)(maxRecordSamples / WaveformPeaks::SAMPLES_PER_PEAK + 2), 0.0f);
    std::vector<float> pMax(pMin.size(), 0.0f);

    const juce::ScopedLock sl(audioLock);
    recordBuffer        = std::move(buf);
    livePeakMin.swap(pMin);
    livePeakMax.swap(pMax);
    livePeakCount.store(0);
    liveAccMin = liveAccMax = 0.0f;
    liveAccN = 0;
    recordWritePos      = 0;
    recordStartSample   = playheadSample;
    recordingTrackIndex = trackIndex;
    transportState      = TransportState::Recording;
}

void AudioEngine::stopRecording()
{
    const juce::ScopedLock sl(audioLock);
    if (transportState != TransportState::Recording) return;

    if (recordingTrackIndex >= 0 &&
        recordingTrackIndex < (int)project.tracks.size() &&
        recordBuffer && recordWritePos > 0)
    {
        pushUndoState();

        auto data = std::make_shared<juce::AudioBuffer<float>>(1, recordWritePos);
        data->copyFrom(0, 0, *recordBuffer, 0, 0, recordWritePos);

        auto* track = project.tracks[(size_t)recordingTrackIndex].get();
        auto* clip  = track->addClip();
        clip->setSource(data, deviceSampleRate);
        clip->startTimeSec = (double)recordStartSample / deviceSampleRate;
        clip->name = TR("Take ") + juce::String((int)track->clips.size());
    }

    recordingTrackIndex = -1;
    recordBuffer.reset();
    recordWritePos = 0;
    transportState = TransportState::Playing;
    sendChangeMessage();
}

bool AudioEngine::getRecordingInfo(int& trackIndex, double& startSec, double& lengthSec) const
{
    if (transportState != TransportState::Recording || recordingTrackIndex < 0)
        return false;
    trackIndex = recordingTrackIndex;
    startSec   = (double)recordStartSample / deviceSampleRate;
    lengthSec  = (double)recordWritePos / deviceSampleRate;
    return true;
}

int AudioEngine::getLivePeaks(const float*& mins, const float*& maxs) const
{
    if (transportState != TransportState::Recording || livePeakMin.empty()) return 0;
    mins = livePeakMin.data();
    maxs = livePeakMax.data();
    return livePeakCount.load(std::memory_order_acquire);
}

int AudioEngine::getRecordTargetTrack() const
{
    const auto& tracks = project.tracks;
    if (tracks.empty()) return -1;
    for (size_t i = 0; i < tracks.size(); ++i)
        if (tracks[i]->armed) return (int)i;
    return juce::jlimit(0, (int)tracks.size() - 1, selectedTrack.load());
}

double AudioEngine::getPlayheadPositionSec() const
{
    return (double)playheadSample / deviceSampleRate;
}

void AudioEngine::setPlayheadPositionSec(double sec)
{
    const juce::ScopedLock sl(audioLock);
    if (transportState == TransportState::Recording) return;
    playheadSample = std::max<juce::int64>(0, (juce::int64)(sec * deviceSampleRate));
}

void AudioEngine::setLoopPoints(double s, double e)
{
    project.loopStart = std::max(0.0, s);
    project.loopEnd   = std::max(project.loopStart + 0.1, e);
}

// ============================================================
//  Undo / Redo
// ============================================================
AudioEngine::ClipSnapshot AudioEngine::takeSnapshot() const
{
    ClipSnapshot s;
    s.reserve(project.tracks.size());
    for (auto& t : project.tracks)
    {
        std::vector<AudioClip> clips;
        clips.reserve(t->clips.size());
        for (auto& c : t->clips) clips.push_back(*c);
        s.push_back(std::move(clips));
    }
    return s;
}

void AudioEngine::restoreSnapshot(const ClipSnapshot& s)
{
    const juce::ScopedLock sl(audioLock);
    for (size_t ti = 0; ti < project.tracks.size() && ti < s.size(); ++ti)
    {
        auto& t = *project.tracks[ti];
        t.clips.clear();
        for (auto& c : s[ti])
            t.clips.push_back(std::make_unique<AudioClip>(c));
    }
}

void AudioEngine::pushUndoState()
{
    undoStack.push_back(takeSnapshot());
    if (undoStack.size() > MAX_UNDO)
        undoStack.erase(undoStack.begin());
    redoStack.clear();
}

bool AudioEngine::undo()
{
    if (undoStack.empty() || transportState == TransportState::Recording) return false;
    if (undoStack.back().size() != project.tracks.size()) { clearUndoHistory(); return false; }
    redoStack.push_back(takeSnapshot());
    restoreSnapshot(undoStack.back());
    undoStack.pop_back();
    sendChangeMessage();
    return true;
}

bool AudioEngine::redo()
{
    if (redoStack.empty() || transportState == TransportState::Recording) return false;
    if (redoStack.back().size() != project.tracks.size()) { clearUndoHistory(); return false; }
    undoStack.push_back(takeSnapshot());
    restoreSnapshot(redoStack.back());
    redoStack.pop_back();
    sendChangeMessage();
    return true;
}

// ============================================================
//  Tuner ring buffer
// ============================================================
int AudioEngine::getTunerBuffer(float* dest, int maxSamples) const
{
    int n    = std::min(maxSamples, TUNER_RING_SIZE);
    int head = tunerWriteHead.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
    {
        int idx = (head - n + i + TUNER_RING_SIZE) % TUNER_RING_SIZE;
        dest[i] = tunerRing[idx];
    }
    return n;
}

// ============================================================
//  AudioIODeviceCallback
// ============================================================
void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const double sr = device->getCurrentSampleRate();
    const int    bs = device->getCurrentBufferSizeSamples();

    DSPPool fresh;
    for (size_t i = 0; i < project.tracks.size(); ++i)
    {
        auto d = std::make_unique<TrackDSP>();
        d->prepare(sr);
        fresh.push_back(std::move(d));
    }

    const juce::ScopedLock sl(audioLock);
    // keep the playhead at the same musical time if the sample rate changed
    playheadSample   = (juce::int64)((double)playheadSample / deviceSampleRate * sr);
    deviceSampleRate = sr;
    deviceBufferSize = bs;
    liveDSP.swap(fresh);
    liveScratch.setSize(2, std::max(bs, 4096));
    for (auto& t : project.tracks)
    {
        t->eqLow.dirty = true; t->eqMid.dirty = true; t->eqHigh.dirty = true;
    }
    std::fill(std::begin(tunerRing), std::end(tunerRing), 0.0f);
    tunerWriteHead.store(0, std::memory_order_relaxed);
}

void AudioEngine::audioDeviceStopped() {}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int   numInputChannels,
    float* const* outputChannelData,
    int   numOutputChannels,
    int   numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    const juce::ScopedLock sl(audioLock);

    for (int ch = 0; ch < numOutputChannels; ++ch)
        juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

    if (numOutputChannels < 1) return;
    float* L = outputChannelData[0];
    float* R = numOutputChannels >= 2 ? outputChannelData[1] : outputChannelData[0];

    const float* in = (numInputChannels > 0) ? inputChannelData[0] : nullptr;

    // Tuner ring
    if (in != nullptr)
    {
        int head = tunerWriteHead.load(std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i)
        {
            tunerRing[head] = in[i];
            head = (head + 1) % TUNER_RING_SIZE;
        }
        tunerWriteHead.store(head, std::memory_order_release);
    }

    // Recording
    if (transportState == TransportState::Recording && recordBuffer && in != nullptr)
    {
        int canWrite = std::min(numSamples, maxRecordSamples - recordWritePos);
        if (canWrite > 0)
        {
            recordBuffer->copyFrom(0, recordWritePos, in, canWrite);
            recordWritePos += canWrite;

            // live waveform peaks
            int count = livePeakCount.load(std::memory_order_relaxed);
            for (int i = 0; i < canWrite; ++i)
            {
                liveAccMin = std::min(liveAccMin, in[i]);
                liveAccMax = std::max(liveAccMax, in[i]);
                if (++liveAccN >= WaveformPeaks::SAMPLES_PER_PEAK && count < (int)livePeakMin.size())
                {
                    livePeakMin[(size_t)count] = liveAccMin;
                    livePeakMax[(size_t)count] = liveAccMax;
                    ++count;
                    liveAccMin = liveAccMax = 0.0f;
                    liveAccN = 0;
                }
            }
            livePeakCount.store(count, std::memory_order_release);
        }
    }

    const bool monitor   = inputMonitorEnabled.load(std::memory_order_relaxed) && in != nullptr;
    const int  monTrack  = monitor ? getRecordTargetTrack() : -1;

    // Tracks + metronome
    if (transportState != TransportState::Stopped)
    {
        if (liveScratch.getNumSamples() < numSamples)
            liveScratch.setSize(2, numSamples, false, false, true);

        renderTracks(L, R, numSamples, (double)playheadSample / deviceSampleRate,
                     deviceSampleRate, liveDSP, liveScratch, true, true,
                     monitor ? in : nullptr, monTrack);

        if (metronomeEnabled)
            renderMetronomeClick(L, R, numSamples);

        playheadSample += numSamples;

        if (project.loopEnabled && transportState == TransportState::Playing &&
            project.loopEnd > project.loopStart)
        {
            if ((double)playheadSample / deviceSampleRate >= project.loopEnd)
                playheadSample = (juce::int64)(project.loopStart * deviceSampleRate);
        }
    }
    else if (monitor && monTrack >= 0)
    {
        // Stopped but monitoring: run the tracks' effects (no clips) so the
        // input is heard through the target track's chain.
        if (liveScratch.getNumSamples() < numSamples)
            liveScratch.setSize(2, numSamples, false, false, true);
        renderTracks(L, R, numSamples, 0.0, deviceSampleRate, liveDSP, liveScratch,
                     true, false, in, monTrack);
    }
    else
    {
        for (auto& t : project.tracks)
        {
            t->meterL = t->meterL * 0.8f;
            t->meterR = t->meterR * 0.8f;
        }
    }

    // Monitor without any track: plain dry input
    if (monitor && monTrack < 0)
    {
        juce::FloatVectorOperations::add(L, in, numSamples);
        if (R != L) juce::FloatVectorOperations::add(R, in, numSamples);
    }

    juce::FloatVectorOperations::multiply(L, masterVolume, numSamples);
    if (R != L) juce::FloatVectorOperations::multiply(R, masterVolume, numSamples);

    // Master meters
    const float peakL = juce::FloatVectorOperations::findMaximum(L, numSamples);
    const float minL  = juce::FloatVectorOperations::findMinimum(L, numSamples);
    const float peakR = juce::FloatVectorOperations::findMaximum(R, numSamples);
    const float minR  = juce::FloatVectorOperations::findMinimum(R, numSamples);
    const float decay = std::pow(0.001f, (float)numSamples / (float)deviceSampleRate / 0.3f);
    outputLevelL.store(std::max(std::max(peakL, -minL), outputLevelL.load() * decay));
    outputLevelR.store(std::max(std::max(peakR, -minR), outputLevelR.load() * decay));
}

// ============================================================
//  renderTracks  --  sum every audible track (with its FX)
// ============================================================
void AudioEngine::renderTracks(float* L, float* R, int numSamples, double blockStartSec,
                               double sr, DSPPool& pool, juce::AudioBuffer<float>& scratch,
                               bool live, bool renderClips, const float* monitorIn, int monitorTrack)
{
    syncDSPPool(pool, sr);
    const bool solo = project.anySoloed();
    float* tl = scratch.getWritePointer(0);
    float* tr = scratch.getWritePointer(1);
    const float meterDecay = std::pow(0.001f, (float)numSamples / (float)sr / 0.3f);

    for (size_t ti = 0; ti < project.tracks.size(); ++ti)
    {
        auto& track = *project.tracks[ti];
        auto& dsp   = *pool[ti];

        if (live)
        {
            EQBand* bands[3] = { &track.eqLow, &track.eqMid, &track.eqHigh };
            for (int b = 0; b < 3; ++b)
                if (bands[b]->dirty.exchange(false))
                {
                    dsp.eqL[b].setPeakEQ(sr, bands[b]->frequencyHz, bands[b]->gainDb, bands[b]->q);
                    dsp.eqR[b] = dsp.eqL[b];
                    dsp.eqL[b].reset(); dsp.eqR[b].reset();
                }
        }

        const bool isMonitored = monitorIn != nullptr && (int)ti == monitorTrack;
        const bool audible = isMonitored || (!track.muted && (!solo || track.soloed));
        if (!audible)
        {
            if (live) { track.meterL = track.meterL * meterDecay; track.meterR = track.meterR * meterDecay; }
            continue;
        }

        juce::FloatVectorOperations::clear(tl, numSamples);
        juce::FloatVectorOperations::clear(tr, numSamples);

        if (renderClips && !track.muted && (!solo || track.soloed))
            for (auto& clip : track.clips)
                renderClip(*clip, tl, tr, numSamples, blockStartSec, sr);

        // input monitoring goes INTO the track, so it gets the track's effects
        if (isMonitored)
        {
            juce::FloatVectorOperations::add(tl, monitorIn, numSamples);
            juce::FloatVectorOperations::add(tr, monitorIn, numSamples);
        }

        processTrackFX(track, dsp, tl, tr, numSamples);

        const float panAngle = (track.pan + 1.0f) * 0.5f * juce::MathConstants<float>::halfPi;
        const float gL = std::cos(panAngle) * track.volume * juce::MathConstants<float>::sqrt2;
        const float gR = std::sin(panAngle) * track.volume * juce::MathConstants<float>::sqrt2;

        float pkL = 0.0f, pkR = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float l = tl[i] * gL;
            const float r = tr[i] * gR;
            L[i] += l;
            if (R != L) R[i] += r;
            pkL = std::max(pkL, std::abs(l));
            pkR = std::max(pkR, std::abs(r));
        }

        if (live)
        {
            track.meterL = std::max(pkL, track.meterL.load() * meterDecay);
            track.meterR = std::max(pkR, track.meterR.load() * meterDecay);
        }
    }
}

// ============================================================
//  renderClip  --  trim, fades, gain and sample-rate conversion
// ============================================================
void AudioEngine::renderClip(const AudioClip& clip, float* tl, float* tr, int numSamples,
                             double blockStartSec, double sr)
{
    if (clip.muted || !clip.hasAudio()) return;

    const double cStart   = clip.startTimeSec;
    const double cEnd     = clip.endTimeSec();
    const double blockEnd = blockStartSec + (double)numSamples / sr;
    if (cEnd <= blockStartSec || cStart >= blockEnd) return;

    int i0 = (int)std::ceil((cStart - blockStartSec) * sr);
    int i1 = (int)std::ceil((cEnd   - blockStartSec) * sr);
    i0 = juce::jlimit(0, numSamples, i0);
    i1 = juce::jlimit(0, numSamples, i1);

    const auto& d   = *clip.data;
    const float* s0 = d.getReadPointer(0);
    const float* s1 = d.getNumChannels() > 1 ? d.getReadPointer(1) : s0;
    const int total = d.getNumSamples();

    for (int i = i0; i < i1; ++i)
    {
        const double tIn    = blockStartSec + (double)i / sr - cStart;
        const double srcPos = (tIn + clip.offsetSec) * clip.sampleRate;
        const int    idx    = (int)srcPos;
        if (idx < 0 || idx >= total) continue;

        const int    nxt  = std::min(idx + 1, total - 1);
        const float  frac = (float)(srcPos - (double)idx);
        const float  l    = s0[idx] + frac * (s0[nxt] - s0[idx]);
        const float  r    = s1[idx] + frac * (s1[nxt] - s1[idx]);
        const float  g    = clip.gain * clip.fadeGainAt(tIn);

        tl[i] += l * g;
        tr[i] += r * g;
    }
}

// ============================================================
//  processTrackFX  --  Drive -> EQ -> Comp -> Delay -> Reverb
// ============================================================
void AudioEngine::processTrackFX(Track& track, TrackDSP& dsp, float* l, float* r, int n)
{
    auto& fx = track.fx;
    const double sr = dsp.sampleRate;
    const float twoPi = juce::MathConstants<float>::twoPi;

    // ---- 1. Overdrive (op-amp + diode soft clipping, like the pedal) ----
    if (fx.driveOn.load())
    {
        const float amt    = fx.driveAmount.load();
        const float inGain = 1.0f + amt * amt * 80.0f;
        const float fc     = 700.0f * std::pow(2.0f, fx.driveTone.load() * 4.0f); // 700..11k Hz
        const float lpA    = 1.0f - std::exp(-twoPi * fc / (float)sr);
        const float hpA    = std::exp(-twoPi * 120.0f / (float)sr);
        const float level  = fx.driveLevel.load() * 0.8f;

        for (int i = 0; i < n; ++i)
        {
            // high-pass before the clipper (tightens the low end, like a TS)
            float hl = hpA * (dsp.driveHpL + l[i] - dsp.driveHpInL);
            float hr = hpA * (dsp.driveHpR + r[i] - dsp.driveHpInR);
            dsp.driveHpInL = l[i]; dsp.driveHpInR = r[i];
            dsp.driveHpL = hl;     dsp.driveHpR = hr;

            // asymmetric soft clip (diodes)
            float yl = std::tanh(inGain * hl + 0.05f * amt) - std::tanh(0.05f * amt);
            float yr = std::tanh(inGain * hr + 0.05f * amt) - std::tanh(0.05f * amt);

            // tone control (one-pole low-pass)
            dsp.toneL += lpA * (yl - dsp.toneL);
            dsp.toneR += lpA * (yr - dsp.toneR);

            l[i] = dsp.toneL * level;
            r[i] = dsp.toneR * level;
        }
    }

    // ---- 2. 3-band EQ (bypassed when flat) ----
    const EQBand* bands[3] = { &track.eqLow, &track.eqMid, &track.eqHigh };
    for (int b = 0; b < 3; ++b)
    {
        if (std::abs(bands[b]->gainDb) < 0.05f) continue;
        auto& fl = dsp.eqL[b];
        auto& fr = dsp.eqR[b];
        for (int i = 0; i < n; ++i)
        {
            l[i] = fl.process(l[i]);
            r[i] = fr.process(r[i]);
        }
    }

    // ---- 3. Compressor (stereo-linked peak) ----
    if (fx.compOn.load())
    {
        const float thr    = fx.compThreshold.load();
        const float ratio  = std::max(1.0f, fx.compRatio.load());
        const float makeup = fx.compMakeup.load();
        const float att    = std::exp(-1.0f / (0.005f * (float)sr));
        const float rel    = std::exp(-1.0f / (0.150f * (float)sr));

        for (int i = 0; i < n; ++i)
        {
            const float pk = std::max(std::abs(l[i]), std::abs(r[i]));
            dsp.compEnv = pk > dsp.compEnv ? att * dsp.compEnv + (1.0f - att) * pk
                                           : rel * dsp.compEnv + (1.0f - rel) * pk;
            const float envDb = juce::Decibels::gainToDecibels(dsp.compEnv, -100.0f);
            const float over  = envDb - thr;
            const float gr    = over > 0.0f ? over * (1.0f - 1.0f / ratio) : 0.0f;
            const float g     = juce::Decibels::decibelsToGain(makeup - gr);
            l[i] *= g;
            r[i] *= g;
        }
    }

    // ---- 4. Delay ----
    if (fx.delayOn.load() && !dsp.delayL.empty())
    {
        const int size  = (int)dsp.delayL.size();
        const int dSamp = juce::jlimit(1, size - 1, (int)(fx.delayTimeMs.load() * 0.001 * sr));
        const float fb  = juce::jlimit(0.0f, 0.92f, fx.delayFeedback.load());
        const float mix = fx.delayMix.load();

        for (int i = 0; i < n; ++i)
        {
            int rd = dsp.delayWrite - dSamp;
            if (rd < 0) rd += size;
            const float dl = dsp.delayL[(size_t)rd];
            const float dr = dsp.delayR[(size_t)rd];
            dsp.delayL[(size_t)dsp.delayWrite] = l[i] + dr * fb;   // slight ping-pong
            dsp.delayR[(size_t)dsp.delayWrite] = r[i] + dl * fb;
            l[i] += dl * mix;
            r[i] += dr * mix;
            if (++dsp.delayWrite >= size) dsp.delayWrite = 0;
        }
    }

    // ---- 5. Reverb ----
    if (fx.reverbOn.load())
    {
        const float mix = fx.reverbMix.load();
        dsp.revParams.roomSize   = fx.reverbSize.load();
        dsp.revParams.damping    = fx.reverbDamp.load();
        dsp.revParams.wetLevel   = mix * 0.6f;
        dsp.revParams.dryLevel   = 1.0f - mix * 0.5f;
        dsp.revParams.width      = 1.0f;
        dsp.revParams.freezeMode = 0.0f;
        dsp.reverb.setParameters(dsp.revParams);
        dsp.reverb.processStereo(l, r, n);
    }
}

// ============================================================
//  Metronome  --  locked to the playhead, accent on beat 1
// ============================================================
void AudioEngine::renderMetronomeClick(float* L, float* R, int numSamples)
{
    const double spBeat   = deviceSampleRate * project.secondsPerBeat();
    const int    clickLen = (int)(deviceSampleRate * 0.025);

    for (int i = 0; i < numSamples; ++i)
    {
        const double pos   = (double)(playheadSample + i);
        const double beatF = pos / spBeat;
        const juce::int64 beatIndex = (juce::int64)std::floor(beatF);
        const int phase = (int)((beatF - (double)beatIndex) * spBeat);
        if (phase >= clickLen) continue;

        const bool  accent = (beatIndex % std::max(1, project.timeSignatureN)) == 0;
        const float freq   = accent ? 1600.0f : 1000.0f;
        const float t      = (float)phase / (float)clickLen;
        const float env    = (1.0f - t) * (1.0f - t);
        const float click  = env * (accent ? 0.45f : 0.3f) *
            std::sin(juce::MathConstants<float>::twoPi * freq * (float)phase / (float)deviceSampleRate);
        L[i] += click;
        if (R != L) R[i] += click;
    }
}

// ============================================================
//  Export to WAV (offline, includes EQ + FX)
// ============================================================
bool AudioEngine::exportToWav(const juce::File& outputFile)
{
    const double sr    = deviceSampleRate > 0.0 ? deviceSampleRate : 44100.0;
    const double tail  = 2.0;   // let reverb / delay ring out
    const int    total = (int)((project.getTotalLengthSec() + tail) * sr);
    if (total <= 0) return false;

    DSPPool pool;
    for (auto& t : project.tracks)
    {
        auto d = std::make_unique<TrackDSP>();
        d->prepare(sr);
        const EQBand* bands[3] = { &t->eqLow, &t->eqMid, &t->eqHigh };
        for (int b = 0; b < 3; ++b)
        {
            d->eqL[b].setPeakEQ(sr, bands[b]->frequencyHz, bands[b]->gainDb, bands[b]->q);
            d->eqR[b] = d->eqL[b];
        }
        pool.push_back(std::move(d));
    }

    juce::AudioBuffer<float> mix(2, total);
    mix.clear();
    juce::AudioBuffer<float> scratch(2, 512);

    for (int pos = 0; pos < total; pos += 512)
    {
        const int n = std::min(512, total - pos);
        float* Lp = mix.getWritePointer(0, pos);
        float* Rp = mix.getWritePointer(1, pos);
        renderTracks(Lp, Rp, n, (double)pos / sr, sr, pool, scratch, false);
        juce::FloatVectorOperations::multiply(Lp, masterVolume, n);
        juce::FloatVectorOperations::multiply(Rp, masterVolume, n);
    }

    outputFile.deleteFile();
    juce::WavAudioFormat wavFormat;
    auto outStream = outputFile.createOutputStream();
    if (outStream == nullptr) return false;

    juce::StringPairArray metaData;
    JUCE_BEGIN_IGNORE_DEPRECATION_WARNINGS
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outStream.get(), sr, 2, 24, metaData, 0));
    JUCE_END_IGNORE_DEPRECATION_WARNINGS
    if (!writer) return false;
    outStream.release();   // the writer owns the stream now

    writer->writeFromAudioSampleBuffer(mix, 0, total);
    return true;
}

// ============================================================
//  Project XML Serialization
// ============================================================
static void writeFX(juce::XmlElement& e, const TrackFX& fx)
{
    e.setAttribute("drvOn",  fx.driveOn.load());
    e.setAttribute("drvAmt", (double)fx.driveAmount.load());
    e.setAttribute("drvTone",(double)fx.driveTone.load());
    e.setAttribute("drvLvl", (double)fx.driveLevel.load());
    e.setAttribute("cmpOn",  fx.compOn.load());
    e.setAttribute("cmpThr", (double)fx.compThreshold.load());
    e.setAttribute("cmpRat", (double)fx.compRatio.load());
    e.setAttribute("cmpMk",  (double)fx.compMakeup.load());
    e.setAttribute("dlyOn",  fx.delayOn.load());
    e.setAttribute("dlyMs",  (double)fx.delayTimeMs.load());
    e.setAttribute("dlyFb",  (double)fx.delayFeedback.load());
    e.setAttribute("dlyMix", (double)fx.delayMix.load());
    e.setAttribute("revOn",  fx.reverbOn.load());
    e.setAttribute("revSz",  (double)fx.reverbSize.load());
    e.setAttribute("revDmp", (double)fx.reverbDamp.load());
    e.setAttribute("revMix", (double)fx.reverbMix.load());
}

static void readFX(const juce::XmlElement& e, TrackFX& fx)
{
    fx.driveOn       = e.getBoolAttribute("drvOn", false);
    fx.driveAmount   = (float)e.getDoubleAttribute("drvAmt", 0.5);
    fx.driveTone     = (float)e.getDoubleAttribute("drvTone", 0.5);
    fx.driveLevel    = (float)e.getDoubleAttribute("drvLvl", 0.7);
    fx.compOn        = e.getBoolAttribute("cmpOn", false);
    fx.compThreshold = (float)e.getDoubleAttribute("cmpThr", -18.0);
    fx.compRatio     = (float)e.getDoubleAttribute("cmpRat", 4.0);
    fx.compMakeup    = (float)e.getDoubleAttribute("cmpMk", 0.0);
    fx.delayOn       = e.getBoolAttribute("dlyOn", false);
    fx.delayTimeMs   = (float)e.getDoubleAttribute("dlyMs", 350.0);
    fx.delayFeedback = (float)e.getDoubleAttribute("dlyFb", 0.35);
    fx.delayMix      = (float)e.getDoubleAttribute("dlyMix", 0.25);
    fx.reverbOn      = e.getBoolAttribute("revOn", false);
    fx.reverbSize    = (float)e.getDoubleAttribute("revSz", 0.5);
    fx.reverbDamp    = (float)e.getDoubleAttribute("revDmp", 0.5);
    fx.reverbMix     = (float)e.getDoubleAttribute("revMix", 0.25);
}

std::unique_ptr<juce::XmlElement> AudioEngine::saveProjectToXml(const juce::File& projectFile)
{
    auto root = std::make_unique<juce::XmlElement>("PennyroyalProject");
    root->setAttribute("version",     2);
    root->setAttribute("name",        project.name);
    root->setAttribute("bpm",         project.bpm);
    root->setAttribute("timeSigN",    project.timeSignatureN);
    root->setAttribute("timeSigD",    project.timeSignatureD);
    root->setAttribute("loopStart",   project.loopStart);
    root->setAttribute("loopEnd",     project.loopEnd);
    root->setAttribute("loopEnabled", project.loopEnabled);
    root->setAttribute("masterVol",   (double)masterVolume);

    // Folder for recordings that only exist in memory
    const juce::File audioDir = projectFile.getSiblingFile(
        projectFile.getFileNameWithoutExtension() + "_Audio");
    std::map<const juce::AudioBuffer<float>*, juce::String> written;

    auto* tracksEl = root->createNewChildElement("Tracks");
    int trackNum = 0;
    for (auto& t : project.tracks)
    {
        ++trackNum;
        auto* tel = tracksEl->createNewChildElement("Track");
        tel->setAttribute("name",       t->name);
        tel->setAttribute("volume",     (double)t->volume);
        tel->setAttribute("pan",        (double)t->pan);
        tel->setAttribute("muted",      t->muted);
        tel->setAttribute("soloed",     t->soloed);
        tel->setAttribute("colour",     t->colour.toString());
        tel->setAttribute("eqLowGain",  (double)t->eqLow.gainDb);
        tel->setAttribute("eqLowFreq",  (double)t->eqLow.frequencyHz);
        tel->setAttribute("eqMidGain",  (double)t->eqMid.gainDb);
        tel->setAttribute("eqMidFreq",  (double)t->eqMid.frequencyHz);
        tel->setAttribute("eqHighGain", (double)t->eqHigh.gainDb);
        tel->setAttribute("eqHighFreq", (double)t->eqHigh.frequencyHz);
        writeFX(*tel->createNewChildElement("FX"), t->fx);

        auto* clipsEl = tel->createNewChildElement("Clips");
        int clipNum = 0;
        for (auto& c : t->clips)
        {
            ++clipNum;
            if (!c->hasAudio()) continue;

            // Write in-memory recordings to disk
            if (c->sourceFilePath.isEmpty() || !juce::File(c->sourceFilePath).existsAsFile())
            {
                auto it = written.find(c->data.get());
                if (it != written.end())
                {
                    c->sourceFilePath = it->second;
                }
                else
                {
                    audioDir.createDirectory();
                    auto f = audioDir.getNonexistentChildFile(
                        "Track" + juce::String(trackNum) + "_Clip" + juce::String(clipNum), ".wav", false);
                    juce::WavAudioFormat wav;
                    if (auto os = f.createOutputStream())
                    {
                        juce::StringPairArray md;
                        JUCE_BEGIN_IGNORE_DEPRECATION_WARNINGS
                        std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(
                            os.get(), c->sampleRate, (unsigned int)c->numChannels(), 24, md, 0));
                        JUCE_END_IGNORE_DEPRECATION_WARNINGS
                        if (w)
                        {
                            os.release();
                            w->writeFromAudioSampleBuffer(*c->data, 0, c->data->getNumSamples());
                            c->sourceFilePath = f.getFullPathName();
                            written[c->data.get()] = c->sourceFilePath;
                        }
                    }
                }
            }

            auto* cel = clipsEl->createNewChildElement("Clip");
            cel->setAttribute("name",      c->name);
            cel->setAttribute("startTime", c->startTimeSec);
            cel->setAttribute("offset",    c->offsetSec);
            cel->setAttribute("length",    c->lengthSec);
            cel->setAttribute("fadeIn",    c->fadeInSec);
            cel->setAttribute("fadeOut",   c->fadeOutSec);
            cel->setAttribute("gain",      (double)c->gain);
            cel->setAttribute("muted",     c->muted);
            cel->setAttribute("filePath",  c->sourceFilePath);
        }
    }
    return root;
}

bool AudioEngine::loadProjectFromXml(const juce::XmlElement& xml)
{
    if (xml.getTagName() != "PennyroyalProject") return false;

    // Build everything first, then swap in under the lock (no audio glitch while reading files)
    std::vector<std::unique_ptr<Track>> newTracks;
    juce::AudioFormatManager fmt;
    fmt.registerBasicFormats();
    std::map<juce::String, std::pair<std::shared_ptr<juce::AudioBuffer<float>>, double>> cache;

    if (auto* tracksEl = xml.getChildByName("Tracks"))
    {
        for (auto* tel : tracksEl->getChildIterator())
        {
            if (tel->getTagName() != "Track") continue;

            auto t = std::make_unique<Track>();
            t->name   = tel->getStringAttribute("name", "Track");
            t->volume = (float)tel->getDoubleAttribute("volume", 1.0);
            t->pan    = (float)tel->getDoubleAttribute("pan", 0.0);
            t->muted  = tel->getBoolAttribute("muted", false);
            t->soloed = tel->getBoolAttribute("soloed", false);
            if (tel->hasAttribute("colour"))
                t->colour = juce::Colour::fromString(tel->getStringAttribute("colour"));

            t->eqLow.gainDb       = (float)tel->getDoubleAttribute("eqLowGain",  0.0);
            t->eqLow.frequencyHz  = (float)tel->getDoubleAttribute("eqLowFreq",  100.0);
            t->eqMid.gainDb       = (float)tel->getDoubleAttribute("eqMidGain",  0.0);
            t->eqMid.frequencyHz  = (float)tel->getDoubleAttribute("eqMidFreq",  1000.0);
            t->eqHigh.gainDb      = (float)tel->getDoubleAttribute("eqHighGain", 0.0);
            t->eqHigh.frequencyHz = (float)tel->getDoubleAttribute("eqHighFreq", 8000.0);

            if (auto* fxEl = tel->getChildByName("FX"))
                readFX(*fxEl, t->fx);

            if (auto* clipsEl = tel->getChildByName("Clips"))
            {
                for (auto* cel : clipsEl->getChildIterator())
                {
                    if (cel->getTagName() != "Clip") continue;
                    const juce::String path = cel->getStringAttribute("filePath");
                    if (path.isEmpty()) continue;

                    auto it = cache.find(path);
                    if (it == cache.end())
                    {
                        juce::File f(path);
                        if (!f.existsAsFile()) continue;
                        std::unique_ptr<juce::AudioFormatReader> reader(fmt.createReaderFor(f));
                        if (!reader) continue;
                        auto buf = std::make_shared<juce::AudioBuffer<float>>(
                            (int)reader->numChannels, (int)reader->lengthInSamples);
                        reader->read(buf.get(), 0, (int)reader->lengthInSamples, 0, true, true);
                        it = cache.emplace(path, std::make_pair(buf, reader->sampleRate)).first;
                    }

                    auto* clip = t->addClip();
                    clip->setSource(it->second.first, it->second.second);
                    clip->sourceFilePath = path;
                    clip->name         = cel->getStringAttribute("name", "Clip");
                    clip->startTimeSec = cel->getDoubleAttribute("startTime", 0.0);
                    clip->offsetSec    = cel->getDoubleAttribute("offset", 0.0);
                    clip->lengthSec    = cel->getDoubleAttribute("length", clip->sourceLengthSec());
                    clip->fadeInSec    = cel->getDoubleAttribute("fadeIn", 0.0);
                    clip->fadeOutSec   = cel->getDoubleAttribute("fadeOut", 0.0);
                    clip->gain         = (float)cel->getDoubleAttribute("gain", 1.0);
                    clip->muted        = cel->getBoolAttribute("muted", false);
                    clip->lengthSec    = juce::jlimit(0.01, clip->sourceLengthSec() - clip->offsetSec,
                                                      clip->lengthSec);
                }
            }
            newTracks.push_back(std::move(t));
        }
    }

    DSPPool fresh;
    for (size_t i = 0; i < newTracks.size(); ++i)
    {
        auto d = std::make_unique<TrackDSP>();
        d->prepare(deviceSampleRate);
        fresh.push_back(std::move(d));
    }

    {
        const juce::ScopedLock sl(audioLock);
        project.name           = xml.getStringAttribute("name", "Untitled Project");
        project.bpm            = xml.getDoubleAttribute("bpm", 120.0);
        project.timeSignatureN = xml.getIntAttribute("timeSigN", 4);
        project.timeSignatureD = xml.getIntAttribute("timeSigD", 4);
        project.loopStart      = xml.getDoubleAttribute("loopStart", 0.0);
        project.loopEnd        = xml.getDoubleAttribute("loopEnd", 8.0);
        project.loopEnabled    = xml.getBoolAttribute("loopEnabled", false);
        masterVolume           = (float)xml.getDoubleAttribute("masterVol", 0.85);

        project.tracks.swap(newTracks);
        liveDSP.swap(fresh);
        transportState = TransportState::Stopped;
        playheadSample = 0;
    }
    clearUndoHistory();
    sendChangeMessage();
    return true;
}
