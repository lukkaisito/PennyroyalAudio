#pragma once
#include <JuceHeader.h>

// ============================================================
//  AppSettings  --  settings file location + UI scaling helpers
// ============================================================
namespace AppSettings
{
    inline juce::PropertiesFile::Options options()
    {
        juce::PropertiesFile::Options o;
        o.applicationName     = "Pennyroyal Audio";
        o.filenameSuffix      = "settings";
        o.folderName          = "Pennyroyal Audio";
        o.osxLibrarySubFolder = "Application Support";
        return o;
    }

    // Available interface sizes (global scale factors)
    inline const float scales[] = { 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.25f };
    constexpr int numScales = 6;

    inline int indexOfScale(float s)
    {
        int best = 3;
        for (int i = 0; i < numScales; ++i)
            if (std::abs(scales[i] - s) < std::abs(scales[best] - s)) best = i;
        return best;
    }

    // First run: pick the biggest size that lets the full layout fit the screen
    inline float autoScale()
    {
        auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
        if (d == nullptr) return 1.0f;
        const auto a = d->userArea;
        const float fit = juce::jmin(1.0f, (float)a.getWidth() / 1400.0f, (float)a.getHeight() / 900.0f);
        float chosen = scales[0];
        for (float s : scales)
            if (s <= fit + 0.001f) chosen = s;
        return chosen;
    }

    // Make sure a window is not bigger than the screen it is on
    inline void fitWindowToScreen(juce::Component& window)
    {
        auto& displays = juce::Desktop::getInstance().getDisplays();
        auto* d = displays.getDisplayForRect(window.getScreenBounds());
        if (d == nullptr) d = displays.getPrimaryDisplay();
        if (d == nullptr) return;

        const auto area = d->userArea.reduced(12);

        // the minimum size must never be bigger than the screen
        if (auto* rw = dynamic_cast<juce::ResizableWindow*>(&window))
            rw->setResizeLimits(juce::jmin(960, area.getWidth()), juce::jmin(600, area.getHeight()), 10000, 10000);

        auto b = window.getBounds();
        if (b.getWidth() <= area.getWidth() && b.getHeight() <= area.getHeight() && area.contains(b))
            return;

        const int w = juce::jmin(b.getWidth(),  area.getWidth());
        const int h = juce::jmin(b.getHeight(), area.getHeight());
        window.setBounds(juce::Rectangle<int>(w, h).withCentre(area.getCentre()));
    }
}
