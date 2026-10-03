#pragma once
#include <JuceHeader.h>

// ============================================================
//  WaveformPeaks  --  min/max overview of an audio buffer,
//  computed once so the waveform can be drawn fast at any zoom.
// ============================================================
struct WaveformPeaks
{
    static constexpr int SAMPLES_PER_PEAK = 128;
    std::vector<float> mins, maxs;

    static std::shared_ptr<WaveformPeaks> build(const juce::AudioBuffer<float>& buf)
    {
        auto p = std::make_shared<WaveformPeaks>();
        const int n   = buf.getNumSamples();
        const int chs = buf.getNumChannels();
        const int numPeaks = (n + SAMPLES_PER_PEAK - 1) / SAMPLES_PER_PEAK;
        p->mins.assign((size_t)numPeaks, 0.0f);
        p->maxs.assign((size_t)numPeaks, 0.0f);

        for (int k = 0; k < numPeaks; ++k)
        {
            const int s0 = k * SAMPLES_PER_PEAK;
            const int s1 = std::min(n, s0 + SAMPLES_PER_PEAK);
            float lo = 0.0f, hi = 0.0f;
            for (int ch = 0; ch < chs; ++ch)
            {
                const float* d = buf.getReadPointer(ch);
                for (int s = s0; s < s1; ++s)
                {
                    lo = std::min(lo, d[s]);
                    hi = std::max(hi, d[s]);
                }
            }
            p->mins[(size_t)k] = lo;
            p->maxs[(size_t)k] = hi;
        }
        return p;
    }
};

// ============================================================
//  AudioClip
//  The audio data is shared (shared_ptr) so that copies, splits
//  and undo snapshots never duplicate the samples in memory.
// ============================================================
struct AudioClip
{
    std::shared_ptr<juce::AudioBuffer<float>> data;   // source audio (never modified)
    std::shared_ptr<WaveformPeaks>            peaks;  // cached overview

    double sampleRate   = 44100.0;
    double startTimeSec = 0.0;   // position on the timeline
    double offsetSec    = 0.0;   // where the clip starts inside the source (trim)
    double lengthSec    = 0.0;   // visible / audible length
    double fadeInSec    = 0.0;
    double fadeOutSec   = 0.0;
    float  gain         = 1.0f;

    juce::String name   = "Clip";
    juce::String sourceFilePath;   // file on disk (empty for unsaved recordings)
    bool   muted        = false;

    bool   hasAudio()        const { return data != nullptr && data->getNumSamples() > 0; }
    int    numChannels()     const { return data ? data->getNumChannels() : 0; }
    double endTimeSec()      const { return startTimeSec + lengthSec; }
    double sourceLengthSec() const
    {
        return (data && sampleRate > 0.0) ? (double)data->getNumSamples() / sampleRate : 0.0;
    }

    // Assign new audio and reset trim / fades to cover the whole source
    void setSource(std::shared_ptr<juce::AudioBuffer<float>> buf, double sr)
    {
        data       = std::move(buf);
        sampleRate = sr;
        peaks      = data ? WaveformPeaks::build(*data) : nullptr;
        offsetSec  = 0.0;
        lengthSec  = sourceLengthSec();
        fadeInSec  = fadeOutSec = 0.0;
    }

    // Gain multiplier from the fades at a time relative to clip start
    inline float fadeGainAt(double tInClip) const noexcept
    {
        float g = 1.0f;
        if (fadeInSec > 0.0 && tInClip < fadeInSec)
            g *= (float)(tInClip / fadeInSec);
        const double toEnd = lengthSec - tInClip;
        if (fadeOutSec > 0.0 && toEnd < fadeOutSec)
            g *= (float)(toEnd / fadeOutSec);
        return g * g * (3.0f - 2.0f * g);   // smoothstep curve
    }
};

// ============================================================
//  EQBand  --  dirty flag for lazy coefficient recalculation
// ============================================================
struct EQBand
{
    float frequencyHz = 1000.0f;
    float gainDb      = 0.0f;
    float q           = 0.707f;
    std::atomic<bool> dirty { true };
};

// ============================================================
//  TrackFX  --  built-in effect rack parameters (per track)
//  Written by the UI, read by the audio thread -> atomics.
// ============================================================
struct TrackFX
{
    // Overdrive (modelled after the analog pedal of the project)
    std::atomic<bool>  driveOn       { false };
    std::atomic<float> driveAmount   { 0.5f };   // 0..1
    std::atomic<float> driveTone     { 0.5f };   // 0..1  dark -> bright
    std::atomic<float> driveLevel    { 0.7f };   // 0..1

    // Compressor
    std::atomic<bool>  compOn        { false };
    std::atomic<float> compThreshold { -18.0f }; // dB
    std::atomic<float> compRatio     { 4.0f };   // 1..20
    std::atomic<float> compMakeup    { 0.0f };   // dB

    // Delay
    std::atomic<bool>  delayOn       { false };
    std::atomic<float> delayTimeMs   { 350.0f }; // 20..1500
    std::atomic<float> delayFeedback { 0.35f };  // 0..0.9
    std::atomic<float> delayMix      { 0.25f };  // 0..1

    // Reverb
    std::atomic<bool>  reverbOn      { false };
    std::atomic<float> reverbSize    { 0.5f };
    std::atomic<float> reverbDamp    { 0.5f };
    std::atomic<float> reverbMix     { 0.25f };
};

// ============================================================
//  Track
// ============================================================
struct Track
{
    juce::String name   = "Track";
    float        volume = 1.0f;
    float        pan    = 0.0f;
    bool         muted  = false;
    bool         soloed = false;
    bool         armed  = false;
    juce::Colour colour;

    EQBand eqLow  {  100.0f, 0.0f, 0.707f };
    EQBand eqMid  { 1000.0f, 0.0f, 0.707f };
    EQBand eqHigh { 8000.0f, 0.0f, 0.707f };

    TrackFX fx;

    // Post-fader peak level, written by the audio thread
    std::atomic<float> meterL { 0.0f };
    std::atomic<float> meterR { 0.0f };

    std::vector<std::unique_ptr<AudioClip>> clips;

    Track()
    {
        static int colourIndex = 0;
        // Earthy, organic palette fitting the In Utero aesthetic
        static const juce::Colour palette[] = {
            juce::Colour(0xFF8A9A6E),  // verde musgo claro
            juce::Colour(0xFFC79A4B),  // ocre
            juce::Colour(0xFFB5654A),  // terracota
            juce::Colour(0xFF8C7BA8),  // violeta
            juce::Colour(0xFF6FA08A),  // salvia
            juce::Colour(0xFFC0443C),  // rojo
            juce::Colour(0xFF6F8FB0),  // azul acero
            juce::Colour(0xFFD1C38A),  // amarillo crema
        };
        colour = palette[colourIndex % 8];
        ++colourIndex;
    }

    AudioClip* addClip()
    {
        clips.push_back(std::make_unique<AudioClip>());
        return clips.back().get();
    }

    int indexOf(const AudioClip* c) const
    {
        for (int i = 0; i < (int)clips.size(); ++i)
            if (clips[(size_t)i].get() == c) return i;
        return -1;
    }

    void removeClip(int index)
    {
        if (index >= 0 && index < (int)clips.size())
            clips.erase(clips.begin() + index);
    }

    double getTotalLengthSec() const
    {
        double maxEnd = 0.0;
        for (auto& c : clips)
            maxEnd = std::max(maxEnd, c->endTimeSec());
        return maxEnd;
    }
};

// ============================================================
//  Project
// ============================================================
struct Project
{
    juce::String name     = "Untitled Project";
    double bpm            = 120.0;
    int    timeSignatureN = 4;
    int    timeSignatureD = 4;
    double sampleRate     = 44100.0;
    int    bufferSize     = 512;

    // Loop region (seconds)
    double loopStart   = 0.0;
    double loopEnd     = 8.0;
    bool   loopEnabled = false;

    std::vector<std::unique_ptr<Track>> tracks;

    double secondsPerBeat() const { return 60.0 / std::max(1.0, bpm); }
    double secondsPerBar()  const { return secondsPerBeat() * timeSignatureN; }

    Track* addTrack(const juce::String& trackName = "Audio")
    {
        auto* t = tracks.emplace_back(std::make_unique<Track>()).get();
        t->name = trackName + " " + juce::String((int)tracks.size());
        return t;
    }

    void removeTrack(int index)
    {
        if (index >= 0 && index < (int)tracks.size())
            tracks.erase(tracks.begin() + index);
    }

    bool anySoloed() const
    {
        for (auto& t : tracks) if (t->soloed) return true;
        return false;
    }

    double getTotalLengthSec() const
    {
        double maxLen = 0.0;
        for (auto& t : tracks)
            maxLen = std::max(maxLen, t->getTotalLengthSec());
        return std::max(maxLen, 1.0);
    }
};
