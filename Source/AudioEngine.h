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
//  TrackDSP  --  processing state of one track (EQ + FX rack)
// ============================================================
struct TrackDSP
{
    BiquadFilter eqL[3], eqR[3];      // low / mid / high, one set per channel

    // Overdrive
    float driveHpL = 0, driveHpR = 0, driveHpInL = 0, driveHpInR = 0;
    float toneL = 0, toneR = 0;

    // Compressor
    float compEnv = 0.0f;

    // Delay
    std::vector<float> delayL, delayR;
    int delayWrite = 0;

    // Reverb
    juce::Reverb reverb;
    juce::Reverb::Parameters revParams;

    double sampleRate = 44100.0;

    void prepare(double sr)
    {
        sampleRate = sr;
        delayL.assign((size_t)(sr * 2.0) + 1, 0.0f);
        delayR.assign((size_t)(sr * 2.0) + 1, 0.0f);
        delayWrite = 0;
        reverb.setSampleRate(sr);
        reverb.reset();
        for (auto& f : eqL) f.reset();
        for (auto& f : eqR) f.reset();
        driveHpL = driveHpR = driveHpInL = driveHpInR = toneL = toneR = 0.0f;
        compEnv = 0.0f;
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
    double getDeviceSampleRate() const { return deviceSampleRate; }
    int    getDeviceBufferSize() const { return deviceBufferSize; }

    // Lock that protects the project while the audio thread reads it.
    // Take it (ScopedLock) whenever clips / tracks are added, moved or removed.
    juce::CriticalSection& getLock() { return audioLock; }

    // ---- Project -----------------------------------------------
    Project& getProject() { return project; }
    Track*   addTrack(const juce::String& name = "Audio");
    void     removeTrack(int index);

    // ---- Transport ---------------------------------------------
    void play();
    void stop();
    void pause();
    void startRecording(int trackIndex);
    void stopRecording();

    TransportState getTransportState()      const { return transportState; }
    double         getPlayheadPositionSec() const;
    void           setPlayheadPositionSec(double sec);

    // Live recording info for drawing the growing region
    bool getRecordingInfo(int& trackIndex, double& startSec, double& lengthSec) const;

    // Live waveform of the take being recorded (min/max per 128 samples).
    // Returns the number of valid peaks; pointers stay valid while recording.
    int  getLivePeaks(const float*& mins, const float*& maxs) const;

    // ---- Track selection / record target -----------------------
    // The record target (and the track the input monitor goes through) is the
    // armed track, or the selected track when no track is armed.
    void setSelectedTrack(int index) { selectedTrack.store(index); }
    int  getSelectedTrack() const    { return selectedTrack.load(); }
    int  getRecordTargetTrack() const;

    // ---- Loop --------------------------------------------------
    void setLoopEnabled(bool on)       { project.loopEnabled = on; }
    bool isLoopEnabled()         const { return project.loopEnabled; }
    void setLoopPoints(double s, double e);

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

    // ---- Undo / Redo (clip edits) ------------------------------
    void pushUndoState();          // call BEFORE changing clips
    bool undo();
    bool redo();
    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }
    void clearUndoHistory() { undoStack.clear(); redoStack.clear(); }

    // ---- Export ------------------------------------------------
    bool exportToWav(const juce::File& outputFile);

    // ---- Level meters ------------------------------------------
    float getOutputLevelL() const { return outputLevelL.load(std::memory_order_relaxed); }
    float getOutputLevelR() const { return outputLevelR.load(std::memory_order_relaxed); }

    // ---- Tuner ring buffer -------------------------------------
    int getTunerBuffer(float* dest, int maxSamples) const;

    // ---- Project serialization ---------------------------------
    // Recorded clips that have no file yet are written as WAVs into
    // "<project name>_Audio/" next to the project file.
    std::unique_ptr<juce::XmlElement> saveProjectToXml(const juce::File& projectFile);
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
    using DSPPool = std::vector<std::unique_ptr<TrackDSP>>;

    // Renders all tracks (+ FX) into L/R, adding to what is there.
    void renderTracks(float* L, float* R, int numSamples, double blockStartSec,
                      double sr, DSPPool& pool, juce::AudioBuffer<float>& scratch,
                      bool live, bool renderClips = true,
                      const float* monitorIn = nullptr, int monitorTrack = -1);
    void renderClip(const AudioClip& clip, float* tl, float* tr, int numSamples,
                    double blockStartSec, double sr);
    void processTrackFX(Track& track, TrackDSP& dsp, float* l, float* r, int n);
    void renderMetronomeClick(float* L, float* R, int numSamples);
    void syncDSPPool(DSPPool& pool, double sr);

    using ClipSnapshot = std::vector<std::vector<AudioClip>>;
    ClipSnapshot takeSnapshot() const;
    void         restoreSnapshot(const ClipSnapshot& s);

    // ---- Data --------------------------------------------------
    juce::AudioDeviceManager deviceManager;
    Project                  project;

    TransportState     transportState   { TransportState::Stopped };
    juce::int64        playheadSample   { 0 };
    double             deviceSampleRate { 44100.0 };
    int                deviceBufferSize { 512 };

    // Recording
    int  recordingTrackIndex { -1 };
    juce::int64 recordStartSample { 0 };
    std::unique_ptr<juce::AudioBuffer<float>> recordBuffer;
    int  recordWritePos { 0 };
    int  maxRecordSamples { 44100 * 600 };   // 10 min

    // live waveform of the current take
    std::vector<float> livePeakMin, livePeakMax;
    std::atomic<int>   livePeakCount { 0 };
    float liveAccMin { 0.0f }, liveAccMax { 0.0f };
    int   liveAccN { 0 };

    std::atomic<int> selectedTrack { 0 };

    // Metronome
    bool  metronomeEnabled { false };

    // Input monitor
    std::atomic<bool> inputMonitorEnabled { false };

    // Master
    float masterVolume { 0.85f };
    std::atomic<float> outputLevelL { 0.0f };
    std::atomic<float> outputLevelR { 0.0f };

    // Per-track DSP (live)
    DSPPool liveDSP;
    juce::AudioBuffer<float> liveScratch;

    // Undo
    std::vector<ClipSnapshot> undoStack, redoStack;
    static constexpr size_t MAX_UNDO = 60;

    // Tuner ring buffer
    float            tunerRing[TUNER_RING_SIZE] {};
    std::atomic<int> tunerWriteHead { 0 };

    juce::CriticalSection audioLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
