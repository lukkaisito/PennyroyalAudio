#include "TunerPanel.h"
#include <cmath>
#include <numeric>
#include <algorithm>

// ============================================================
//  Note table  --  A4 = 440 Hz, 12-TET
// ============================================================
static const char* NOTE_NAMES[] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};

TunerPanel::NoteInfo TunerPanel::nearestNote(float hz)
{
    if (hz <= 0.0f) return { "---", 0.0f };

    float midiF  = 69.0f + 12.0f * std::log2(hz / 440.0f);
    int   midiI  = (int)std::round(midiF);
    int   octave = (midiI / 12) - 1;
    int   note   = ((midiI % 12) + 12) % 12;

    float idealHz = 440.0f * std::pow(2.0f, (float)(midiI - 69) / 12.0f);

    juce::String name = juce::String(NOTE_NAMES[note]) +
                        juce::String(octave);
    return { name, idealHz };
}

float TunerPanel::centsDiff(float detectedHz, float referenceHz)
{
    if (referenceHz <= 0.0f || detectedHz <= 0.0f) return 0.0f;
    return 1200.0f * std::log2(detectedHz / referenceHz);
}

// ============================================================
//  Constructor / Destructor
// ============================================================
TunerPanel::TunerPanel(AudioEngine& eng) : engine(eng)
{
    analysisBuffer.resize(AudioEngine::TUNER_RING_SIZE, 0.0f);
    startTimerHz(20);
}

TunerPanel::~TunerPanel()
{
    stopTimer();
}

// ============================================================
//  Timer callback
// ============================================================
void TunerPanel::timerCallback()
{
    int n = engine.getTunerBuffer(analysisBuffer.data(),
                                  (int)analysisBuffer.size());
    if (n > 0)
    {
        float hz = detectPitch(analysisBuffer.data(), n,
                               engine.getDeviceSampleRate());
        if (hz > 0.0f)
        {
            detectedHz    = detectedHz * 0.7f + hz * 0.3f;
            auto info     = nearestNote(detectedHz);
            noteName      = info.name;
            detectedCents = centsDiff(detectedHz, info.idealHz);
        }
        else
        {
            detectedHz    *= 0.95f;
            if (detectedHz < 20.0f) { detectedHz = 0.0f; noteName = "---"; }
            detectedCents = 0.0f;
        }
    }
    repaint();
}

// ============================================================
//  Normalised Square Difference Function (NSDF) pitch detector
// ============================================================
float TunerPanel::detectPitch(const float* data, int N, double sr) const
{
    if (N < 256) return 0.0f;

    int analysisN = std::min(N, 2048);

    float sum = 0.0f;
    for (int i = 0; i < analysisN; ++i) sum += data[i];
    float mean = sum / (float)analysisN;

    std::vector<double> cleanData(analysisN);
    std::vector<double> cumSq(analysisN + 1, 0.0);
    for (int i = 0; i < analysisN; ++i)
    {
        cleanData[i] = (double)data[i] - mean;
        cumSq[i + 1] = cumSq[i] + cleanData[i] * cleanData[i];
    }

    double totalSq = cumSq[analysisN];
    if (totalSq < 1e-4) return 0.0f;

    int lagMin = (int)std::floor(sr / 1200.0);
    int lagMax = (int)std::ceil(sr / 60.0);
    lagMin = std::max(1, lagMin);
    lagMax = std::min(lagMax, analysisN / 2 - 1);
    if (lagMax <= lagMin) return 0.0f;

    std::vector<float> nsdf(lagMax + 1, 0.0f);

    for (int tau = lagMin; tau <= lagMax; ++tau)
    {
        double r = 0.0;
        for (int i = 0; i < analysisN - tau; ++i)
            r += cleanData[i] * cleanData[i + tau];
        double m = cumSq[analysisN - tau] + (totalSq - cumSq[tau]);
        nsdf[tau] = (m > 1e-9) ? (float)(2.0 * r / m) : 0.0f;
    }

    struct Peak { int lag; float val; };
    std::vector<Peak> peaks;

    float globalMax = -1.0f;
    for (int tau = lagMin + 1; tau < lagMax; ++tau)
    {
        if (nsdf[tau] > nsdf[tau - 1] && nsdf[tau] > nsdf[tau + 1])
        {
            peaks.push_back({ tau, nsdf[tau] });
            if (nsdf[tau] > globalMax)
                globalMax = nsdf[tau];
        }
    }

    if (globalMax < 0.18f || peaks.empty()) return 0.0f;

    float peakThreshold = 0.85f * globalMax;
    int bestLag = 0;
    for (const auto& peak : peaks)
    {
        if (peak.val >= peakThreshold)
        {
            bestLag = peak.lag;
            break;
        }
    }

    if (bestLag == 0) return 0.0f;

    float y1 = nsdf[bestLag - 1];
    float y2 = nsdf[bestLag];
    float y3 = nsdf[bestLag + 1];
    float denom = 2.0f * y2 - y1 - y3;
    float refinedLag = (float)bestLag;
    if (std::abs(denom) > 1e-6f)
        refinedLag += 0.5f * (y1 - y3) / denom;

    float freq = (float)(sr / refinedLag);
    if (freq < 20.0f || freq > 4000.0f) return 0.0f;
    return freq;
}

// ============================================================
//  paint  --  InUtero dark aesthetic
// ============================================================
void TunerPanel::paint(juce::Graphics& g)
{
    using namespace InUtero;

    const int   w  = getWidth();
    const int   h  = getHeight();
    const float cx = (float)w * 0.5f;
    const float cy = (float)h * 0.43f;

    // Background
    g.setColour(c(Negro));
    g.fillAll();

    // Subtle grid
    g.setColour(c(GrisOscuro).withAlpha(0.3f));
    const int numCols = 16;
    for (int i = 0; i <= numCols; ++i)
        g.drawVerticalLine((int)((float)i * (float)w / (float)numCols), 0.0f, (float)h);
    const int numRows = 10;
    for (int i = 0; i <= numRows; ++i)
        g.drawHorizontalLine((int)((float)i * (float)h / (float)numRows), 0.0f, (float)w);

    // Title
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f).withStyle("Bold")));
    g.setColour(c(GrisClaro));
    g.drawText(TR("CHROMATIC TUNER"), 0, 14, w, 20, juce::Justification::centred);

    const float radius  = std::min(cx, cy) * 0.75f;
    const float arcFrom = juce::MathConstants<float>::pi * 0.85f;
    const float arcTo   = juce::MathConstants<float>::pi * 2.15f;
    const float arcMid  = juce::MathConstants<float>::pi * 1.5f;
    const float arcRange = (arcTo - arcFrom) * 0.5f;

    // Arc background
    juce::Path bgArc;
    bgArc.addCentredArc(cx, cy, radius, radius, 0.0f, arcFrom, arcTo, true);
    g.setColour(c(GrisOscuro));
    g.strokePath(bgArc, juce::PathStrokeType(4.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float maxCents   = 50.0f;
    float clampedCents     = juce::jlimit(-maxCents, maxCents, detectedCents);
    float needleAngle      = arcMid + (clampedCents / maxCents) * arcRange;

    bool inTune = (std::abs(detectedCents) < 5.0f && detectedHz > 20.0f);
    juce::Colour needleCol = (detectedHz <= 20.0f) ? c(GrisMedio) :
                             (inTune ? c(VerdeClaro) :
                             (detectedCents > 0.0f ? c(Ocre) : juce::Colour(0xFF5A7090)));

    // Filled arc
    juce::Path fillArc;
    fillArc.addCentredArc(cx, cy, radius, radius, 0.0f, arcMid, needleAngle, true);
    g.setColour(needleCol.withAlpha(0.6f));
    g.strokePath(fillArc, juce::PathStrokeType(4.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Tick marks
    auto drawTick = [&](float cents, float len, juce::Colour col, float thickness)
    {
        float angle = arcMid + (cents / maxCents) * arcRange - juce::MathConstants<float>::halfPi;
        float x1 = cx + (radius - len - 4.0f) * std::cos(angle);
        float y1 = cy + (radius - len - 4.0f) * std::sin(angle);
        float x2 = cx + (radius + 4.0f)       * std::cos(angle);
        float y2 = cy + (radius + 4.0f)       * std::sin(angle);
        g.setColour(col);
        g.drawLine(x1, y1, x2, y2, thickness);
    };

    drawTick(0.0f, 15.0f, inTune ? c(VerdeClaro) : c(GrisClaro), 2.0f);
    for (float cVal = -50.0f; cVal <= 50.0f; cVal += 10.0f)
    {
        if (std::abs(cVal) < 1.0f) continue;
        bool isMajor = ((int)std::round(cVal) % 25 == 0 || std::abs(cVal) == 50.0f);
        drawTick(cVal, isMajor ? 10.0f : 5.0f,
                 isMajor ? c(GrisClaro) : c(GrisMedio),
                 isMajor ? 1.5f : 1.0f);
    }

    // Needle
    if (detectedHz > 20.0f)
    {
        float angle = needleAngle - juce::MathConstants<float>::halfPi;
        float nx = cx + (radius - 8.0f) * std::cos(angle);
        float ny = cy + (radius - 8.0f) * std::sin(angle);
        g.setColour(needleCol.withAlpha(0.2f));
        g.drawLine(cx, cy, nx, ny, 6.0f);
        g.setColour(needleCol);
        g.drawLine(cx, cy, nx, ny, 2.0f);
    }

    // Hub
    g.setColour(c(GrisOscuro));
    g.fillEllipse(cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
    g.setColour(needleCol);
    g.drawEllipse(cx - 8.0f, cy - 8.0f, 16.0f, 16.0f, 1.5f);

    // Display box
    float displayW = 140.0f;
    float displayH = 80.0f;
    float displayX = cx - displayW * 0.5f;
    float displayY = cy + radius * 0.35f;

    g.setColour(c(NegroSuave));
    g.fillRoundedRectangle(displayX, displayY, displayW, displayH, 6.0f);
    g.setColour(inTune ? c(VerdeClaro).withAlpha(0.8f) : c(GrisMedio));
    g.drawRoundedRectangle(displayX, displayY, displayW, displayH, 6.0f, 1.5f);

    if (inTune)
    {
        g.setColour(c(VerdeClaro).withAlpha(0.07f));
        g.fillRoundedRectangle(displayX + 1.0f, displayY + 1.0f,
                               displayW - 2.0f, displayH - 2.0f, 5.0f);
    }

    // Note name
    g.setFont(juce::Font(juce::FontOptions().withHeight(38.0f).withStyle("Bold")));
    g.setColour(detectedHz > 20.0f ? (inTune ? c(VerdeClaro) : c(Crema)) : c(GrisMedio));
    g.drawText(detectedHz > 20.0f ? noteName : "---",
               (int)displayX, (int)displayY + 8, (int)displayW, 40,
               juce::Justification::centred);

    // Hz reading
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    g.setColour(detectedHz > 20.0f ? c(CremaOscuro) : c(GrisClaro));
    juce::String hzStr = (detectedHz > 20.0f)
        ? (juce::String(detectedHz, 1) + " Hz") : "--- Hz";
    g.drawText(hzStr, (int)displayX, (int)displayY + 48, (int)displayW, 20,
               juce::Justification::centred);

    // Cent bar
    const int barW = 280, barH = 8;
    const int barX = (int)(cx - barW * 0.5f);
    const int barY = h - 60;

    g.setColour(c(GrisOscuro));
    g.fillRoundedRectangle((float)barX, (float)barY, (float)barW, (float)barH, 4.0f);
    g.setColour(c(GrisMedio));
    g.drawRoundedRectangle((float)barX, (float)barY, (float)barW, (float)barH, 4.0f, 1.0f);

    float halfW = (float)barW * 0.5f;
    float fillX = (float)barX + halfW;
    float fillW = (clampedCents / maxCents) * halfW;
    if (fillW < 0) { fillX += fillW; fillW = -fillW; }

    g.setColour(needleCol.withAlpha(0.85f));
    g.fillRoundedRectangle(fillX, (float)barY, fillW, (float)barH, 3.0f);

    g.setColour(inTune ? c(VerdeClaro) : c(GrisClaro));
    g.drawVerticalLine((int)(barX + halfW), (float)barY - 2.0f, (float)(barY + barH + 2.0f));

    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    g.setColour(c(GrisClaro));
    g.drawText("-50c", barX - 2, barY + barH + 4, 36, 14, juce::Justification::centredLeft);
    g.drawText("0",    (int)(cx - 10), barY + barH + 4, 20, 14, juce::Justification::centred);
    g.drawText("+50c", barX + barW - 34, barY + barH + 4, 36, 14, juce::Justification::centredRight);
}

void TunerPanel::resized() {}
