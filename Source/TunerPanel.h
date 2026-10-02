#pragma once
#include <JuceHeader.h>
#include "AudioEngine.h"
#include "InUteroTheme.h"

// ============================================================
//  TunerPanel  --  chromatic tuner reading live mic audio
// ============================================================
class TunerPanel : public juce::Component,
                   private juce::Timer
{
public:
    explicit TunerPanel(AudioEngine& engine);
    ~TunerPanel() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    float detectPitch(const float* data, int numSamples, double sr) const;

    struct NoteInfo { juce::String name; float idealHz; };
    static NoteInfo nearestNote(float hz);
    static float    centsDiff(float detectedHz, float referenceHz);

    AudioEngine& engine;

    float        detectedHz    { 0.0f };
    float        detectedCents { 0.0f };
    juce::String noteName;

    std::vector<float> analysisBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TunerPanel)
};
