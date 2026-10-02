#pragma once
#include <JuceHeader.h>
#include "AudioEngine.h"
#include "InUteroTheme.h"
#include "TrackModels.h"
#include "TunerPanel.h"

// ============================================================
//  Forward declarations
// ============================================================
class SplashScreenComponent;
class TransportBar;
class TimelineRuler;
class TrackHeader;
class TrackLane;
class TrackPanel;
class MixerPanel;
class PluginsPanel;
class SettingsPanel;
class ExportPanel;
class MainComponent;

// ============================================================
//  Clipboard  (for clip cut/copy/paste)
// ============================================================
struct ClipClipboard
{
    std::unique_ptr<AudioClip> clip;   // owned copy
    bool hasData() const { return clip != nullptr; }
};

// ============================================================
//  SplashScreenComponent
// ============================================================
class SplashScreenComponent : public juce::Component,
                               private juce::Timer
{
public:
    explicit SplashScreenComponent(std::function<void()> onFinished);
    void paint  (juce::Graphics&) override;
    void resized() override {}

private:
    void timerCallback() override;

    std::function<void()> finishedCallback;
    juce::int64 startTimeMs { 0 };
    float       alpha       { 1.0f };
    bool        fadingOut   { false };
    static constexpr int HOLD_MS = 2000;
    static constexpr int FADE_MS = 500;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashScreenComponent)
};

// ============================================================
//  TransportBar
// ============================================================
class TransportBar : public juce::Component,
                     public juce::Slider::Listener,
                     private juce::Timer
{
public:
    explicit TransportBar(AudioEngine& engine);
    void resized() override;
    void paint   (juce::Graphics&) override;
    void sliderValueChanged(juce::Slider*) override;
    void refreshTrackList();

    void triggerPlay()   { playBtn.triggerClick(); }
    void triggerStop()   { stopBtn.triggerClick(); }
    void triggerRecord() { recBtn.triggerClick(); }
    void updateButtonStates();

private:
    void timerCallback() override;

    AudioEngine& engine;

    juce::TextButton playBtn  { "PLAY" };
    juce::TextButton stopBtn  { "STOP" };
    juce::TextButton recBtn   { "REC"  };
    juce::TextButton loopBtn  { "LOOP" };

    juce::TextButton monitorBtn { "MON" };
    juce::TextButton metroBtn   { "CLICK" };

    juce::ComboBox recordTrackBox;
    juce::Label    recordTrackLabel;

    juce::Label  posLabel;
    juce::Label  bpmLabel;
    juce::Slider bpmSlider;

    juce::Slider masterVolSlider;
    juce::Label  volLabel;

    float levelL { 0.0f }, levelR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

// ============================================================
//  TimelineRuler
// ============================================================
class TimelineRuler : public juce::Component
{
public:
    TimelineRuler();
    void paint  (juce::Graphics&) override;
    void setVisibleRange(double startSec, double endSec);
    void setPlayheadPos (double sec);
    void setLoopRegion  (double s, double e, bool enabled);
    void mouseDown  (const juce::MouseEvent&) override;
    void mouseDrag  (const juce::MouseEvent&) override;
    void mouseUp    (const juce::MouseEvent&) override;

    std::function<void(double)>         onSeek;
    std::function<void(double,double)>  onLoopChanged;

private:
    double viewStart   { 0.0  };
    double viewEnd     { 30.0 };
    double playheadSec { 0.0  };
    double loopStart   { 0.0  };
    double loopEnd     { 8.0  };
    bool   loopEnabled { false };

    enum class DragMode { None, Seek, LoopStart, LoopEnd };
    DragMode dragMode { DragMode::None };

    double xToSec(int x) const;
    float  secToX(double sec) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineRuler)
};

// ============================================================
//  TrackHeader
// ============================================================
class TrackHeader : public juce::Component,
                    public juce::Slider::Listener,
                    public juce::Button::Listener
{
public:
    TrackHeader(Track& track, AudioEngine& engine, int index);
    void resized() override;
    void paint   (juce::Graphics&) override;
    void sliderValueChanged(juce::Slider*) override;
    void buttonClicked     (juce::Button*) override;
    void mouseDoubleClick  (const juce::MouseEvent&) override;

    std::function<void()>   onChanged;
    std::function<void(int)> onDeleteRequest;

private:
    Track&       track;
    AudioEngine& engine;
    int          trackIndex;

    juce::Label      nameLabel;
    juce::Slider     volSlider;
    juce::Slider     panSlider;
    juce::TextButton muteBtn { "M" };
    juce::TextButton soloBtn { "S" };
    juce::TextButton armBtn  { "R" };
    juce::TextButton deleteBtn { "X" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackHeader)
};

// ============================================================
//  TrackLane
// ============================================================
class TrackLane : public juce::Component
{
public:
    TrackLane(Track& track, double& viewStart, double& viewEnd);
    void paint      (juce::Graphics&) override;
    void resized    () override;
    void mouseDown  (const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    void setSelectedClip(AudioClip* c) { selectedClip = c; repaint(); }
    AudioClip* getSelectedClip() const { return selectedClip; }

    std::function<void(Track*, AudioClip*)> onClipSelected;
    std::function<void(double, bool)>       onZoom;  // (factor, ctrl held)

private:
    Track&     track;
    double&    viewStart;
    double&    viewEnd;
    AudioClip* selectedClip { nullptr };

    double  xToSec(int x) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackLane)
};

// ============================================================
//  TrackPanel
// ============================================================
class TrackPanel : public juce::Component,
                   public juce::ChangeListener,
                   private juce::Timer
{
public:
    explicit TrackPanel(AudioEngine& engine);
    ~TrackPanel() override;

    void resized() override;
    void paint   (juce::Graphics&) override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    void refreshTracks();

    // Clipboard operations (called from MainComponent)
    void cutSelectedClip();
    void copySelectedClip();
    void pasteClip();
    void deleteSelectedClip();

private:
    void timerCallback() override;
    void applyZoom(double factor, int mouseXInLane);

    AudioEngine& engine;
    double viewStart   { 0.0  };
    double viewEnd     { 30.0 };
    double playheadSec { 0.0  };

    Track*     selectedTrack { nullptr };
    AudioClip* selectedClip  { nullptr };
    ClipClipboard clipboard;

    static constexpr int HEADER_W = 160;
    static constexpr int TRACK_H  = 80;
    static constexpr int RULER_H  = 28;

    TimelineRuler    ruler;
    juce::Viewport   viewport;
    juce::Component  tracksContainer;

    std::vector<std::unique_ptr<TrackHeader>> headers;
    std::vector<std::unique_ptr<TrackLane>>   lanes;

    juce::TextButton addTrackBtn { "  + Add Track  " };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackPanel)
};

// ============================================================
//  MixerPanel
// ============================================================
class MixerPanel : public juce::Component
{
public:
    explicit MixerPanel(AudioEngine& engine);
    void resized() override;
    void paint   (juce::Graphics&) override;
    void refresh();

private:
    AudioEngine& engine;

    struct ChannelStrip : public juce::Component
    {
        ChannelStrip(Track& t, AudioEngine& eng);
        void resized() override;
        void paint   (juce::Graphics&) override;

        Track&       track;
        AudioEngine& engine;

        juce::Label      nameLabel;
        juce::Slider     volFader;
        juce::Slider     eqLowKnob, eqMidKnob, eqHighKnob;
        juce::Label      eqLowLabel, eqMidLabel, eqHighLabel;
        juce::TextButton muteBtn { "M" }, soloBtn { "S" };
    };

    std::vector<std::unique_ptr<ChannelStrip>> strips;
    juce::Viewport   scrollView;
    juce::Component  stripContainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};

// ============================================================
//  PluginsPanel  --  basic VST3 hosting
// ============================================================
class PluginsPanel : public juce::Component,
                     public juce::ListBoxModel
{
public:
    explicit PluginsPanel(AudioEngine& engine);
    ~PluginsPanel() override;

    void resized() override;
    void paint   (juce::Graphics&) override;

    // ListBoxModel
    int  getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

private:
    AudioEngine& engine;

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList          knownPlugins;

    juce::ListBox  pluginList;
    juce::TextButton scanBtn   { "Scan VST3 Folder" };
    juce::TextButton loadBtn   { "Load Selected" };
    juce::Label    statusLabel;

    int selectedRow { -1 };

    void scanForPlugins();
    void loadSelectedPlugin();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginsPanel)
};

// ============================================================
//  ExportPanel
// ============================================================
class ExportPanel : public juce::Component
{
public:
    explicit ExportPanel(AudioEngine& engine);
    void resized() override;
    void paint   (juce::Graphics&) override;

private:
    AudioEngine& engine;

    juce::TextButton exportWavBtn { "Export WAV" };
    juce::TextButton exportMp3Btn { "Export MP3 (requires LAME)" };
    juce::ComboBox   bitrateBox;
    juce::Label      bitrateLabel;
    juce::Label      statusLabel;
    juce::Label      infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportPanel)
};

// ============================================================
//  SettingsPanel
// ============================================================
class SettingsPanel : public juce::Component
{
public:
    explicit SettingsPanel(AudioEngine& engine);
    void resized() override;
    void paint   (juce::Graphics&) override;

private:
    AudioEngine& engine;
    juce::AudioDeviceSelectorComponent deviceSelector;
    juce::ComboBox bufferSizeBox;
    juce::Label    bufferLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};

// ============================================================
//  MainComponent
// ============================================================
class MainComponent : public juce::Component,
                      public juce::KeyListener,
                      public juce::ChangeListener,
                      public juce::MenuBarModel
{
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;
    void paint   (juce::Graphics&) override;

    bool keyPressed(const juce::KeyPress& key,
                    juce::Component* originatingComponent) override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    // MenuBarModel
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu   getMenuForIndex(int index, const juce::String& name) override;
    void              menuItemSelected(int itemId, int topLevelIndex) override;

private:
    void showDAW();
    void saveProject();
    void openProject();

    InUteroTheme theme;
    AudioEngine  engine;
    juce::TooltipWindow tooltipWindow { this, 500 };

    std::unique_ptr<SplashScreenComponent> splash;
    std::unique_ptr<juce::TabbedComponent> tabs;
    std::unique_ptr<juce::MenuBarComponent> menuBar;

    TrackPanel*    trackPanel    { nullptr };
    MixerPanel*    mixerPanel    { nullptr };
    TunerPanel*    tunerPanel    { nullptr };
    PluginsPanel*  pluginsPanel  { nullptr };
    ExportPanel*   exportPanel   { nullptr };
    SettingsPanel* settingsPanel { nullptr };
    TransportBar*  transport     { nullptr };

    juce::File currentProjectFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
