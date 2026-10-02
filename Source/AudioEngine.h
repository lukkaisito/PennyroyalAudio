#pragma once
#include <JuceHeader.h>
#include "TrackModels.h"

// ============================================================
//  BiquadFilter  --  Direct Form II Transposed, double precision
// ============================================================
struct BiquadFilter
{
    double b0=1, b1=0, b2=0, a1=0, a2=0;
    double z1=0, z2=0;

    void reset() noexcept { z1 = z2 = 0.0; }

    void setPeakEQ(double sr, double freqHz, double gainDb, double q) noexcept
    {
        double A     = std::pow(10.0, gainDb / 40.0);
        double w0    = 2.0 * juce::MathConstants<double>::pi * freqHz / sr;
        double alpha = std::sin(w0) / (2.0 * q);
        double a0r   = 1.0 / (1.0 + alpha / A);
        b0 = (1.0 + alpha * A) * a0r;
        b1 = (-2.0 * std::cos(w0)) * a0r;
        b2 = (1.0 - alpha * A) * a0r;
        a1 = (-2.0 * std::cos(w0)) * a0r;
        a2 = (1.0 - alpha / A)  * a0r;
    }

    inline float process(float x) noexcept
    {
        double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return static_cast<float>(y);
    }
};

// ============================================================
//  AudioEngine
// ============================================================
class AudioEngine : public juce::AudioIODeviceCallback,
                    public juce::ChangeBroadcaster
{
public:
    enum class TransportState { Stopped, Playing, Recording };

    static constexpr int TUNER_RING_SIZE = 4096;

    AudioEngine();
    ~AudioEngine() override;

    // ---- Device ------------------------------------------------
    void initialise();
    void shutdown();
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    // ---- Project -----------------------------------------------
    Project& getProject() { return project; }

    // ---- Transport ---------------------------------------------
    void play();
    void stop();
    void pause();
    void startRecording(int trackIndex);
    void stopRecording();
    void undoLastRecording();

    TransportState getTransportState()      const { return transportState; }
    double         getPlayheadPositionSec() const;
    void           setPlayheadPositionSec(double sec);

    // ---- Loop --------------------------------------------------
    void   setLoopEnabled(bool on)         { project.loopEnabled = on; }
    bool   isLoopEnabled()           const { return project.loopEnabled; }
    void   setLoopPoints(double s, double e);

    // ---- Metronome ---------------------------------------------
    void setMetronomeEnabled(bool on) { metronomeEnabled = on; }
    bool isMetronomeEnabled()   const { return metronomeEnabled; }

    // ---- Input monitor -----------------------------------------
    void setInputMonitorEnabled(bool on) { inputMonitorEnabled = on; }
    bool isInputMonitorEnabled()   const { return inputMonitorEnabled; }

    // ---- Master volume -----------------------------------------
    void  setMasterVolume(float v) { masterVolume = juce::jlimit(0.0f, 2.0f, v); }
    float getMasterVolume()  const { return masterVolume; }

    // ---- Buffer size -------------------------------------------
    void setBufferSize(int samples);

    // ---- Export ------------------------------------------------
    bool exportToWav(const juce::File& outputFile);
    // NOTE: MP3 export via LAME requires linking liblame.
    // To enable: install LAME, add liblame to CMake, and uncomment:
    //   bool exportToMp3(const juce::File& outputFile, int bitrate = 192);
    // The implementation is provided in AudioEngine.cpp (guarded by #ifdef JUCE_USE_LAME).

    // ---- Level meters ------------------------------------------
    float getOutputLevelL() const { return outputLevelL.load(std::memory_order_relaxed); }
    float getOutputLevelR() const { return outputLevelR.load(std::memory_order_relaxed); }

    // ---- Tuner ring buffer -------------------------------------
    int getTunerBuffer(float* dest, int maxSamples) const;
    double getDeviceSampleRate() const { return deviceSampleRate; }

    // ---- Track EQ management -----------------------------------
    // Called when a track is removed to clean up EQ state
    void removeTrackEQ(int trackIndex);

    // ---- Project serialization ---------------------------------
    std::unique_ptr<juce::XmlElement> saveProjectToXml() const;
    bool loadProjectFromXml(const juce::XmlElement& xml);

    // ---- AudioIODeviceCallback ---------------------------------
    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,  int numInputChannels,
        float* const* outputChannelData,       int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    void renderBlock(float* const* out, int numOut,
                     const float* const* in, int numIn,
                     int numSamples);
    void mixTrackIntoOutput(int trackIndex,
                            float* L, float* R,
                            int numSamples,
                            double blockStartSec);
    void renderMetronomeClick(float* L, float* R, int numSamples);
    void rebuildEQPool();

    // ---- Data --------------------------------------------------
    juce::AudioDeviceManager deviceManager;
    Project                  project;

    TransportState     transportState   { TransportState::Stopped };
    juce::int64        playheadSample   { 0 };
    double             deviceSampleRate { 44100.0 };
    int                deviceBufferSize { 512 };

    // Recording
    int  recordingTrackIndex    { -1 };
    int  lastRecordedTrackIndex { -1 };
    int  lastRecordedClipIndex  { -1 };
    std::unique_ptr<juce::AudioBuffer<float>> recordBuffer;
    int  recordWritePos         { 0 };
    static constexpr int MAX_RECORD_SAMPLES = 44100 * 300; // 5 min

    // Metronome
    bool  metronomeEnabled       { false };
    int   metronomeSampleCounter { 0 };

    // Input monitor
    std::atomic<bool>  inputMonitorEnabled { false };

    // Master volume
    float masterVolume { 0.85f };

    // Level meters
    std::atomic<float> outputLevelL { 0.0f };
    std::atomic<float> outputLevelR { 0.0f };

    // Per-track EQ
    struct TrackEQ { BiquadFilter low, mid, high; };
    std::vector<std::unique_ptr<TrackEQ>> trackEQs;

    // Tuner ring buffer
    float              tunerRing[TUNER_RING_SIZE] {};
    std::atomic<int>   tunerWriteHead { 0 };

    juce::CriticalSection audioLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
