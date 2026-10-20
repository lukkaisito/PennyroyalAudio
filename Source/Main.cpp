#include <JuceHeader.h>
#include "MainComponent.h"
#include "AppSettings.h"

// ============================================================
//  Application entry point
// ============================================================
class PennyroyalAudioApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName()    override { return "Pennyroyal Audio"; }
    const juce::String getApplicationVersion() override { return "1.4.1"; }
    bool moreThanOneInstanceAllowed()           override { return true; }

    void initialise(const juce::String& /*commandLine*/) override
    {
        // Interface size: saved choice, or an automatic size that fits this screen
        {
            juce::PropertiesFile props(AppSettings::options());
            const float scale = props.containsKey("uiScale")
                                    ? (float)props.getDoubleValue("uiScale", 1.0)
                                    : AppSettings::autoScale();
            juce::Desktop::getInstance().setGlobalScaleFactor(juce::jlimit(0.5f, 2.0f, scale));
        }
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    // ---- Main Window ----
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name,
                             juce::Desktop::getInstance()
                                 .getDefaultLookAndFeel()
                                 .findColour(ResizableWindow::backgroundColourId),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

            setResizable(true, true);

            // never open bigger than the screen (small laptops, Windows zoom 125% / 150%)
            int w = getWidth(), h = getHeight();
            if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            {
                w = juce::jmin(w, d->userArea.getWidth()  - 24);
                h = juce::jmin(h, d->userArea.getHeight() - 24);
            }
            setResizeLimits(juce::jmin(960, w), juce::jmin(600, h), 10000, 10000);
            centreWithSize(w, h);

            // reopen with the size / position of the last session, always inside the screen
            if (auto* mc = dynamic_cast<MainComponent*>(getContentComponent()))
            {
                const auto saved = mc->getSavedWindowState();
                if (saved.isNotEmpty() && restoreWindowStateFromString(saved) && !isFullScreen())
                    AppSettings::fitWindowToScreen(*this);
            }
            setVisible(true);
        }

        ~MainWindow() override
        {
            // the content (MainComponent) is still alive here
            if (!isMinimised())
                if (auto* mc = dynamic_cast<MainComponent*>(getContentComponent()))
                    mc->saveWindowState(getWindowStateAsString());
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(PennyroyalAudioApp)
