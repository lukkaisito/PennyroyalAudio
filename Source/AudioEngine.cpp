#include "AudioEngine.h"
#include <cmath>
#include <algorithm>

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
            juce::AlertWindow::WarningIcon, "Audio Device Error", err);
    deviceManager.addAudioCallback(this);
}

void AudioEngine::shutdown()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

// ============================================================
//  Transport
// ============================================================
void AudioEngine::play()
{
    const juce::ScopedLock sl(audioLock);
    if (transportState != TransportState::Playing)
        transportState = TransportState::Playing;
}

void AudioEngine::stop()
{
    const juce::ScopedLock sl(audioLock);
    stopRecording();
    transportState = TransportState::Stopped;
    playheadSample = 0;
    sendChangeMessage();
}

void AudioEngine::pause()
{
    const juce::ScopedLock sl(audioLock);
    if (transportState == TransportState::Playing)
        transportState = TransportState::Stopped;
}

void AudioEngine::startRecording(int trackIndex)
{
    const juce::ScopedLock sl(audioLock);
    if (trackIndex < 0 || trackIndex >= (int)project.tracks.size())
        return;
    recordBuffer = std::make_unique<juce::AudioBuffer<float>>(1, MAX_RECORD_SAMPLES);
    recordBuffer->clear();
    recordWritePos      = 0;
    recordingTrackIndex = trackIndex;
    transportState      = TransportState::Recording;
}

void AudioEngine::stopRecording()
{
    if (transportState != TransportState::Recording) return;

    if (recordingTrackIndex >= 0 &&
        recordingTrackIndex < (int)project.tracks.size() &&
        recordBuffer && recordWritePos > 0)
    {
        auto* track = project.tracks[recordingTrackIndex].get();
        auto* clip  = track->addClip();
        clip->buffer.setSize(1, recordWritePos);
        clip->buffer.copyFrom(0, 0, *recordBuffer, 0, 0, recordWritePos);
        clip->sampleRate   = deviceSampleRate;
        clip->startTimeSec = (double)(playheadSample - recordWritePos) / deviceSampleRate;
        if (clip->startTimeSec < 0.0) clip->startTimeSec = 0.0;
        clip->refreshLength();

        lastRecordedTrackIndex = recordingTrackIndex;
        lastRecordedClipIndex  = (int)track->clips.size() - 1;
    }

    recordingTrackIndex = -1;
    recordBuffer.reset();
    recordWritePos = 0;
    transportState = TransportState::Playing;
    sendChangeMessage();
}

void AudioEngine::undoLastRecording()
{
    if (transportState == TransportState::Recording) return;
    if (lastRecordedTrackIndex < 0 ||
        lastRecordedTrackIndex >= (int)project.tracks.size()) return;

    auto* track = project.tracks[lastRecordedTrackIndex].get();
    if (lastRecordedClipIndex >= 0 &&
        lastRecordedClipIndex < (int)track->clips.size())
    {
        track->removeClip(lastRecordedClipIndex);
        lastRecordedTrackIndex = -1;
        lastRecordedClipIndex  = -1;
        sendChangeMessage();
    }
}

double AudioEngine::getPlayheadPositionSec() const
{
    return (double)playheadSample / deviceSampleRate;
}

void AudioEngine::setPlayheadPositionSec(double sec)
{
    const juce::ScopedLock sl(audioLock);
    playheadSample = (juce::int64)(sec * deviceSampleRate);
    if (playheadSample < 0) playheadSample = 0;
}

void AudioEngine::setLoopPoints(double s, double e)
{
    project.loopStart = s;
    project.loopEnd   = std::max(s + 0.1, e);
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

void AudioEngine::removeTrackEQ(int trackIndex)
{
    const juce::ScopedLock sl(audioLock);
    if (trackIndex >= 0 && trackIndex < (int)trackEQs.size())
        trackEQs.erase(trackEQs.begin() + trackIndex);
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
    deviceSampleRate       = device->getCurrentSampleRate();
    deviceBufferSize       = device->getCurrentBufferSizeSamples();
    metronomeSampleCounter = 0;

    const juce::ScopedLock sl(audioLock);
    rebuildEQPool();
    std::fill(std::begin(tunerRing), std::end(tunerRing), 0.0f);
    tunerWriteHead.store(0, std::memory_order_relaxed);
}

void AudioEngine::audioDeviceStopped() {}

void AudioEngine::rebuildEQPool()
{
    trackEQs.clear();
    for (auto& t : project.tracks)
    {
        trackEQs.push_back(std::make_unique<TrackEQ>());
        t->eqLow.dirty.store(true);
        t->eqMid.dirty.store(true);
        t->eqHigh.dirty.store(true);
    }
}

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

    // Fill tuner ring
    if (numInputChannels > 0 && inputChannelData[0] != nullptr)
    {
        int head = tunerWriteHead.load(std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i)
        {
            tunerRing[head] = inputChannelData[0][i];
            head = (head + 1) % TUNER_RING_SIZE;
        }
        tunerWriteHead.store(head, std::memory_order_release);
    }

    // Mic recording
    if (transportState == TransportState::Recording &&
        recordBuffer && numInputChannels > 0 &&
        inputChannelData[0] != nullptr)
    {
        int canWrite = std::min(numSamples, MAX_RECORD_SAMPLES - recordWritePos);
        if (canWrite > 0)
        {
            recordBuffer->copyFrom(0, recordWritePos, inputChannelData[0], canWrite);
            recordWritePos += canWrite;
        }
    }

    // Render
    if (transportState != TransportState::Stopped)
    {
        renderBlock(outputChannelData, numOutputChannels,
                    inputChannelData,  numInputChannels,
                    numSamples);
        playheadSample += numSamples;

        // Loop wrap (audio thread)
        if (project.loopEnabled &&
            transportState == TransportState::Playing &&
            project.loopEnd > project.loopStart)
        {
            double posSec = (double)playheadSample / deviceSampleRate;
            if (posSec >= project.loopEnd)
                playheadSample = (juce::int64)(project.loopStart * deviceSampleRate);
        }
    }

    // Level meters
    if (numOutputChannels >= 1)
    {
        float peakL = 0.0f, peakR = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peakL = std::max(peakL, std::abs(outputChannelData[0][i]));
        if (numOutputChannels >= 2)
            for (int i = 0; i < numSamples; ++i)
                peakR = std::max(peakR, std::abs(outputChannelData[1][i]));
        else
            peakR = peakL;

        float decay = std::pow(0.001f, (float)numSamples / (float)deviceSampleRate / 0.1f);
        outputLevelL.store(std::max(peakL, outputLevelL.load(std::memory_order_relaxed) * decay),
                           std::memory_order_relaxed);
        outputLevelR.store(std::max(peakR, outputLevelR.load(std::memory_order_relaxed) * decay),
                           std::memory_order_relaxed);
    }
}

// ============================================================
//  renderBlock
// ============================================================
void AudioEngine::renderBlock(float* const* out, int numOut,
                              const float* const* in,  int numIn,
                              int numSamples)
{
    if (numOut < 1 || numSamples <= 0) return;

    float* L = out[0];
    float* R = (numOut >= 2) ? out[1] : out[0];

    const double blockStartSec = (double)playheadSample / deviceSampleRate;

    // Grow EQ pool if tracks were added
    while ((int)trackEQs.size() < (int)project.tracks.size())
    {
        trackEQs.push_back(std::make_unique<TrackEQ>());
        int ti = (int)trackEQs.size() - 1;
        project.tracks[ti]->eqLow.dirty.store(true);
        project.tracks[ti]->eqMid.dirty.store(true);
        project.tracks[ti]->eqHigh.dirty.store(true);
    }

    // Mix each track
    for (int ti = 0; ti < (int)project.tracks.size(); ++ti)
    {
        auto& track = *project.tracks[ti];
        if (track.muted) continue;

        auto& eq = *trackEQs[ti];
        if (track.eqLow.dirty.exchange(false))
        {
            eq.low.setPeakEQ(deviceSampleRate,
                track.eqLow.frequencyHz, track.eqLow.gainDb, track.eqLow.q);
            eq.low.reset();
        }
        if (track.eqMid.dirty.exchange(false))
        {
            eq.mid.setPeakEQ(deviceSampleRate,
                track.eqMid.frequencyHz, track.eqMid.gainDb, track.eqMid.q);
            eq.mid.reset();
        }
        if (track.eqHigh.dirty.exchange(false))
        {
            eq.high.setPeakEQ(deviceSampleRate,
                track.eqHigh.frequencyHz, track.eqHigh.gainDb, track.eqHigh.q);
            eq.high.reset();
        }

        mixTrackIntoOutput(ti, L, R, numSamples, blockStartSec);
    }

    if (metronomeEnabled)
        renderMetronomeClick(L, R, numSamples);

    if (inputMonitorEnabled.load(std::memory_order_relaxed) &&
        numIn > 0 && in[0] != nullptr)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            L[i] += in[0][i];
            R[i] += in[0][i];
        }
    }

    juce::FloatVectorOperations::multiply(L, masterVolume, numSamples);
    if (L != R)
        juce::FloatVectorOperations::multiply(R, masterVolume, numSamples);
}

// ============================================================
//  mixTrackIntoOutput
// ============================================================
void AudioEngine::mixTrackIntoOutput(int trackIndex,
                                     float* L, float* R,
                                     int numSamples,
                                     double blockStartSec)
{
    auto& track = *project.tracks[trackIndex];
    auto& eq    = *trackEQs[trackIndex];

    const double blockEndSec = blockStartSec + (double)numSamples / deviceSampleRate;

    const float panAngle = (track.pan + 1.0f) * 0.5f *
                           juce::MathConstants<float>::halfPi;
    const float panL = std::cos(panAngle);
    const float panR = std::sin(panAngle);
    const float vol  = track.volume;

    for (auto& clipPtr : track.clips)
    {
        if (clipPtr->muted)   continue;
        if (clipPtr->buffer.getNumSamples() == 0) continue;

        const double cStart = clipPtr->startTimeSec;
        const double cEnd   = cStart + clipPtr->lengthSec;
        if (cEnd <= blockStartSec || cStart >= blockEndSec) continue;

        const int numClipCh    = clipPtr->buffer.getNumChannels();
        const int totalSamples = clipPtr->buffer.getNumSamples();
        const double clipSR    = clipPtr->sampleRate;

        int iStart = (int)std::ceil( (cStart - blockStartSec) * deviceSampleRate);
        int iEnd   = (int)std::floor((cEnd   - blockStartSec) * deviceSampleRate);
        iStart = juce::jlimit(0, numSamples, iStart);
        iEnd   = juce::jlimit(0, numSamples, iEnd);

        for (int i = iStart; i < iEnd; ++i)
        {
            const double sampleTimeSec = blockStartSec + (double)i / deviceSampleRate;
            int clipIdx = (int)((sampleTimeSec - cStart) * clipSR);
            if (clipIdx < 0 || clipIdx >= totalSamples) continue;

            float sL, sR;
            if (numClipCh >= 2)
            {
                sL = clipPtr->buffer.getSample(0, clipIdx);
                sR = clipPtr->buffer.getSample(1, clipIdx);
            }
            else
            {
                sL = sR = clipPtr->buffer.getSample(0, clipIdx);
            }

            sL = eq.high.process(eq.mid.process(eq.low.process(sL)));
            sR = eq.high.process(eq.mid.process(eq.low.process(sR)));

            L[i] += sL * vol * panL;
            R[i] += sR * vol * panR;
        }
    }
}

// ============================================================
//  Metronome click
// ============================================================
void AudioEngine::renderMetronomeClick(float* L, float* R, int numSamples)
{
    const double samplesPerBeat = deviceSampleRate * 60.0 / project.bpm;
    const int clickLen = (int)(deviceSampleRate * 0.020);

    for (int i = 0; i < numSamples; ++i)
    {
        float click = 0.0f;
        if (metronomeSampleCounter < clickLen)
        {
            float t   = (float)metronomeSampleCounter / (float)clickLen;
            float env = (1.0f - t) * (1.0f - t);
            click = env * 0.35f *
                    std::sin(juce::MathConstants<float>::twoPi *
                             1000.0f * (float)metronomeSampleCounter /
                             (float)deviceSampleRate);
        }
        L[i] += click;
        R[i] += click;
        if (++metronomeSampleCounter >= (int)samplesPerBeat)
            metronomeSampleCounter = 0;
    }
}

// ============================================================
//  Export to WAV
// ============================================================
bool AudioEngine::exportToWav(const juce::File& outputFile)
{
    const double sr           = deviceSampleRate > 0.0 ? deviceSampleRate : 44100.0;
    const int    totalSamples = (int)(project.getTotalLengthSec() * sr);
    if (totalSamples <= 0) return false;

    juce::AudioBuffer<float> mixBuf(2, totalSamples);
    mixBuf.clear();

    std::vector<TrackEQ> offlineEQs(project.tracks.size());
    for (int ti = 0; ti < (int)project.tracks.size(); ++ti)
    {
        auto& t  = *project.tracks[ti];
        auto& eq = offlineEQs[ti];
        eq.low.setPeakEQ (sr, t.eqLow.frequencyHz,  t.eqLow.gainDb,  t.eqLow.q);
        eq.mid.setPeakEQ (sr, t.eqMid.frequencyHz,  t.eqMid.gainDb,  t.eqMid.q);
        eq.high.setPeakEQ(sr, t.eqHigh.frequencyHz, t.eqHigh.gainDb, t.eqHigh.q);
    }

    const int blockSize = 512;
    for (int pos = 0; pos < totalSamples; pos += blockSize)
    {
        const int    n        = std::min(blockSize, totalSamples - pos);
        const double startSec = (double)pos / sr;

        float* Lp = mixBuf.getWritePointer(0, pos);
        float* Rp = mixBuf.getWritePointer(1, pos);

        for (int ti = 0; ti < (int)project.tracks.size(); ++ti)
        {
            auto& track = *project.tracks[ti];
            if (track.muted) continue;

            const double blockEndSec = startSec + (double)n / sr;
            const float panAngle = (track.pan + 1.0f) * 0.5f *
                                   juce::MathConstants<float>::halfPi;
            const float panL = std::cos(panAngle);
            const float panR = std::sin(panAngle);
            auto& eq = offlineEQs[ti];

            for (auto& clipPtr : track.clips)
            {
                if (clipPtr->muted) continue;
                const double cStart = clipPtr->startTimeSec;
                const double cEnd   = cStart + clipPtr->lengthSec;
                if (cEnd <= startSec || cStart >= blockEndSec) continue;

                const int numClipCh    = clipPtr->buffer.getNumChannels();
                const int totalClipSmp = clipPtr->buffer.getNumSamples();

                for (int i = 0; i < n; ++i)
                {
                    double sampleTimeSec = startSec + (double)i / sr;
                    if (sampleTimeSec < cStart || sampleTimeSec >= cEnd) continue;
                    int clipIdx = (int)((sampleTimeSec - cStart) * clipPtr->sampleRate);
                    if (clipIdx >= totalClipSmp) continue;

                    float sL = clipPtr->buffer.getSample(0, clipIdx);
                    float sR = (numClipCh >= 2) ? clipPtr->buffer.getSample(1, clipIdx) : sL;

                    sL = eq.high.process(eq.mid.process(eq.low.process(sL)));
                    sR = eq.high.process(eq.mid.process(eq.low.process(sR)));

                    Lp[i] += sL * track.volume * panL;
                    Rp[i] += sR * track.volume * panR;
                }
            }
        }

        juce::FloatVectorOperations::multiply(Lp, masterVolume, n);
        juce::FloatVectorOperations::multiply(Rp, masterVolume, n);
    }

    juce::WavAudioFormat wavFormat;
    auto outStream = outputFile.createOutputStream();
    if (outStream == nullptr) return false;

    juce::StringPairArray metaData;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outStream.release(), sr, 2, 24, metaData, 0));
    if (!writer) return false;

    writer->writeFromAudioSampleBuffer(mixBuf, 0, totalSamples);
    return true;
}

// ============================================================
//  MP3 Export (requires LAME)
// ============================================================
// To enable MP3 export:
//   1. Download and build LAME (lame.sourceforge.net)
//   2. Add to CMakeLists.txt: target_link_libraries(... lame)
//   3. Add include path for lame/lame.h
//   4. Uncomment the implementation below
//
// bool AudioEngine::exportToMp3(const juce::File& outputFile, int bitrate)
// {
//     #ifdef HAVE_LAME
//     // ... LAME encoding logic here ...
//     #else
//     juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
//         "MP3 Export", "LAME not linked. Exporting as WAV instead.");
//     return exportToWav(outputFile.withFileExtension("wav"));
//     #endif
// }

// ============================================================
//  Project XML Serialization
// ============================================================
std::unique_ptr<juce::XmlElement> AudioEngine::saveProjectToXml() const
{
    auto root = std::make_unique<juce::XmlElement>("PennyroyalProject");
    root->setAttribute("name",     project.name);
    root->setAttribute("bpm",      project.bpm);
    root->setAttribute("timeSigN", project.timeSignatureN);
    root->setAttribute("timeSigD", project.timeSignatureD);
    root->setAttribute("loopStart",   project.loopStart);
    root->setAttribute("loopEnd",     project.loopEnd);
    root->setAttribute("loopEnabled", project.loopEnabled);
    root->setAttribute("masterVol",   (double)masterVolume);

    auto* tracksEl = root->createNewChildElement("Tracks");
    for (auto& t : project.tracks)
    {
        auto* tel = tracksEl->createNewChildElement("Track");
        tel->setAttribute("name",   t->name);
        tel->setAttribute("volume", (double)t->volume);
        tel->setAttribute("pan",    (double)t->pan);
        tel->setAttribute("muted",  t->muted);
        tel->setAttribute("soloed", t->soloed);
        tel->setAttribute("armed",  t->armed);
        tel->setAttribute("colour", t->colour.toString());
        tel->setAttribute("eqLowGain",  (double)t->eqLow.gainDb);
        tel->setAttribute("eqLowFreq",  (double)t->eqLow.frequencyHz);
        tel->setAttribute("eqMidGain",  (double)t->eqMid.gainDb);
        tel->setAttribute("eqMidFreq",  (double)t->eqMid.frequencyHz);
        tel->setAttribute("eqHighGain", (double)t->eqHigh.gainDb);
        tel->setAttribute("eqHighFreq", (double)t->eqHigh.frequencyHz);

        auto* clipsEl = tel->createNewChildElement("Clips");
        for (auto& c : t->clips)
        {
            auto* cel = clipsEl->createNewChildElement("Clip");
            cel->setAttribute("name",       c->name);
            cel->setAttribute("startTime",  c->startTimeSec);
            cel->setAttribute("length",     c->lengthSec);
            cel->setAttribute("muted",      c->muted);
            cel->setAttribute("filePath",   c->sourceFilePath);
        }
    }
    return root;
}

bool AudioEngine::loadProjectFromXml(const juce::XmlElement& xml)
{
    if (xml.getTagName() != "PennyroyalProject") return false;

    const juce::ScopedLock sl(audioLock);

    project.name             = xml.getStringAttribute("name", "Untitled Project");
    project.bpm              = xml.getDoubleAttribute("bpm", 120.0);
    project.timeSignatureN   = xml.getIntAttribute("timeSigN", 4);
    project.timeSignatureD   = xml.getIntAttribute("timeSigD", 4);
    project.loopStart        = xml.getDoubleAttribute("loopStart", 0.0);
    project.loopEnd          = xml.getDoubleAttribute("loopEnd", 8.0);
    project.loopEnabled      = xml.getBoolAttribute("loopEnabled", false);
    masterVolume             = (float)xml.getDoubleAttribute("masterVol", 0.85);

    project.tracks.clear();
    trackEQs.clear();

    auto* tracksEl = xml.getChildByName("Tracks");
    if (!tracksEl) return true;

    juce::AudioFormatManager fmt;
    fmt.registerBasicFormats();

    for (auto* tel : tracksEl->getChildIterator())
    {
        if (tel->getTagName() != "Track") continue;

        auto* t = project.addTrack();
        t->name   = tel->getStringAttribute("name", "Track");
        t->volume = (float)tel->getDoubleAttribute("volume", 1.0);
        t->pan    = (float)tel->getDoubleAttribute("pan", 0.0);
        t->muted  = tel->getBoolAttribute("muted", false);
        t->soloed = tel->getBoolAttribute("soloed", false);
        t->armed  = tel->getBoolAttribute("armed", false);
        t->colour = juce::Colour::fromString(tel->getStringAttribute("colour",
                                             juce::Colours::green.toString()));

        t->eqLow.gainDb      = (float)tel->getDoubleAttribute("eqLowGain",  0.0);
        t->eqLow.frequencyHz = (float)tel->getDoubleAttribute("eqLowFreq",  80.0);
        t->eqMid.gainDb      = (float)tel->getDoubleAttribute("eqMidGain",  0.0);
        t->eqMid.frequencyHz = (float)tel->getDoubleAttribute("eqMidFreq",  1000.0);
        t->eqHigh.gainDb     = (float)tel->getDoubleAttribute("eqHighGain", 0.0);
        t->eqHigh.frequencyHz= (float)tel->getDoubleAttribute("eqHighFreq", 8000.0);
        t->eqLow.dirty.store(true);
        t->eqMid.dirty.store(true);
        t->eqHigh.dirty.store(true);

        auto* clipsEl = tel->getChildByName("Clips");
        if (clipsEl)
        {
            for (auto* cel : clipsEl->getChildIterator())
            {
                if (cel->getTagName() != "Clip") continue;

                juce::String path = cel->getStringAttribute("filePath", "");
                if (path.isEmpty()) continue;

                juce::File f(path);
                if (!f.existsAsFile()) continue;

                std::unique_ptr<juce::AudioFormatReader> reader(fmt.createReaderFor(f));
                if (!reader) continue;

                auto* clip = t->addClip();
                clip->name           = cel->getStringAttribute("name", "Clip");
                clip->startTimeSec   = cel->getDoubleAttribute("startTime", 0.0);
                clip->muted          = cel->getBoolAttribute("muted", false);
                clip->sourceFilePath = path;
                clip->buffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
                reader->read(&clip->buffer, 0, (int)reader->lengthInSamples, 0, true, true);
                clip->sampleRate = reader->sampleRate;
                clip->refreshLength();
            }
        }

        trackEQs.push_back(std::make_unique<TrackEQ>());
    }

    transportState = TransportState::Stopped;
    playheadSample = 0;
    return true;
}
