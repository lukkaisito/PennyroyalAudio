#pragma once
#include <JuceHeader.h>

// ============================================================
//  AudioClip
// ============================================================
struct AudioClip
{
    juce::AudioBuffer<float> buffer;
    double sampleRate   = 44100.0;
    double startTimeSec = 0.0;
    double lengthSec    = 0.0;
    juce::String name   = "Clip";
    juce::String sourceFilePath;   // absolute path for project save/load
    bool   muted        = false;

    void refreshLength()
    {
        lengthSec = (sampleRate > 0.0 && buffer.getNumSamples() > 0)
                    ? (double)buffer.getNumSamples() / sampleRate
                    : 0.0;
    }
};

// ============================================================
//  EQBand  -- dirty flag for lazy coefficient recalculation
// ============================================================
struct EQBand
{
    float frequencyHz = 1000.0f;
    float gainDb      = 0.0f;
    float q           = 0.707f;
    std::atomic<bool> dirty { true };
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

    EQBand eqLow  {  80.0f, 0.0f, 0.707f };
    EQBand eqMid  { 1000.0f, 0.0f, 0.707f };
    EQBand eqHigh { 8000.0f, 0.0f, 0.707f };

    std::vector<std::unique_ptr<AudioClip>> clips;

    Track()
    {
        static int colourIndex = 0;
        // Earthy, organic palette fitting the In Utero aesthetic
        static const juce::Colour palette[] = {
            juce::Colour(0xFF4A5240),  // verde musgo
            juce::Colour(0xFF7A6840),  // ocre
            juce::Colour(0xFF6B5A4A),  // terracota
            juce::Colour(0xFF5A4A6B),  // violeta oscuro
            juce::Colour(0xFF4A6B5A),  // verde salvia
            juce::Colour(0xFF8B1A1A),  // rojo sangre
            juce::Colour(0xFF4A5A6B),  // azul acero
            juce::Colour(0xFF6B6B4A),  // amarillo musgo
        };
        colour = palette[colourIndex % 8];
        ++colourIndex;
    }

    AudioClip* addClip()
    {
        clips.push_back(std::make_unique<AudioClip>());
        return clips.back().get();
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
            maxEnd = std::max(maxEnd, c->startTimeSec + c->lengthSec);
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
    double loopStart = 0.0;
    double loopEnd   = 8.0;
    bool   loopEnabled = false;

    std::vector<std::unique_ptr<Track>> tracks;

    Track* addTrack(const juce::String& trackName = "Track")
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

    double getTotalLengthSec() const
    {
        double maxLen = 0.0;
        for (auto& t : tracks)
            maxLen = std::max(maxLen, t->getTotalLengthSec());
        return std::max(maxLen, 30.0);
    }
};
