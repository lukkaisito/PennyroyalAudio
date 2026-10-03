#pragma once
#include <JuceHeader.h>
#include "AudioEngine.h"
#include "InUteroTheme.h"
#include "UIComponents.h"
#include "TrackModels.h"
#include "TunerPanel.h"

class TrackPanel;

// ============================================================
//  SplashScreenComponent  --  animated intro, fades out with
//  juce::Desktop::getInstance().getAnimator()
// ============================================================
class SplashScreenComponent : public juce::Component,
                               private juce::Timer
{
public:
    explicit SplashScreenComponent(std::function<void()> onFinished);
    void paint  (juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override { finish(); }

private:
    void timerCallback() override;
    void finish();

    std::function<void()> finishedCallback;
    double startMs  { 0.0 };
    float  t        { 0.0f };   // seconds since start
    bool   finished { false };
    juce::Image grain;

    static constexpr float DURATION = 2.8f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashScreenComponent)
};

// ============================================================
//  TopBar  --  logo, file / undo, transport, time display, BPM,
//              record target and master volume + meter
// ============================================================
class TopBar : public juce::Component,
               private juce::Timer
{
public:
    explicit TopBar(AudioEngine& engine);
    void resized() override;
    void paint   (juce::Graphics&) override;
    void refreshTrackList();
    void updateButtonStates();

    void togglePlay();
    void stopTransport();
    void toggleRecord();
    void toggleLoop();

    std::function<void()> onOpen, onSave, onUndo, onRedo, onHelp;

private:
    void timerCallback() override;

    AudioEngine& engine;

    IconButton openBtn  { "Open",  IconButton::Icon::Open };
    IconButton saveBtn  { "Save",  IconButton::Icon::Save };
    IconButton undoBtn  { "Undo",  IconButton::Icon::Undo };
    IconButton redoBtn  { "Redo",  IconButton::Icon::Redo };
    IconButton helpBtn  { "Help",  IconButton::Icon::Help };

    IconButton stopBtn  { "Stop",   IconButton::Icon::Stop };
    IconButton playBtn  { "Play",   IconButton::Icon::Play };
    IconButton recBtn   { "Record", IconButton::Icon::Record };
    IconButton loopBtn  { "Loop",   IconButton::Icon::Loop };
    IconButton metroBtn { "Click",  IconButton::Icon::Metronome };
    IconButton monBtn   { "Monitor",IconButton::Icon::Monitor };

    juce::Slider   bpmSlider;
    juce::Slider   masterVol;

    juce::Rectangle<int> logoArea, lcdArea, bpmCaption, recCaption, recArea, meterArea, volCaption;
    int lastTarget { -2 };
    float levelL { 0.0f }, levelR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBar)
};

// ============================================================
//  QuickStartTip  --  floating, dismissable help card
// ============================================================
class QuickStartTip : public juce::Component
{
public:
    QuickStartTip();
    void paint  (juce::Graphics&) override;
    void resized() override;

    void setDontShowAgain(bool b) { dontShow.setToggleState(b, juce::dontSendNotification); }
    std::function<void(bool dontShowAgain)> onClose;

    static constexpr int WIDTH = 430, HEIGHT = 262, SHADOW = 14;

private:
    IconButton       closeBtn { "Close", IconButton::Icon::Close };
    juce::ToggleButton dontShow { "Don't show this again" };
    juce::TextButton gotIt { "GOT IT" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickStartTip)
};

// ============================================================
//  TimelineRuler  --  bars/beats, loop strip, playhead
// ============================================================
class TimelineRuler : public juce::Component
{
public:
    TimelineRuler(AudioEngine& engine, TrackPanel& panel);
    void paint   (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    AudioEngine& engine;
    TrackPanel&  panel;

    enum class DragMode { None, Seek, LoopStart, LoopEnd, LoopMove, LoopCreate };
    DragMode dragMode { DragMode::None };
    double   dragAnchor { 0.0 }, origLoopStart { 0.0 }, origLoopEnd { 0.0 };

    bool inLoopStrip(int y) const { return y < getHeight() / 2; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineRuler)
};

// ============================================================
//  TrackHeader  --  name, M / S / R, volume, pan, meter
// ============================================================
class TrackHeader : public juce::Component
{
public:
    TrackHeader(Track& track, AudioEngine& engine, TrackPanel& panel, int index);
    void resized() override;
    void paint   (juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void refreshFromModel();

    juce::Rectangle<int> getMeterArea() const { return meterArea; }

private:
    void startRename();

    Track&       track;
    AudioEngine& engine;
    TrackPanel&  panel;
    int          trackIndex;

    juce::Label      nameLabel;
    juce::TextButton muteBtn { "M" }, soloBtn { "S" }, armBtn { "R" };
    juce::Slider     volSlider, panKnob;
    juce::Rectangle<int> meterArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackHeader)
};

// ============================================================
//  ArrangeCanvas  --  all track lanes: draw clips, move / trim /
//  fade with the mouse, drag & drop audio files
// ============================================================
class ArrangeCanvas : public juce::Component,
                      public juce::FileDragAndDropTarget
{
public:
    ArrangeCanvas(AudioEngine& engine, TrackPanel& panel);

    void paint(juce::Graphics&) override;
    void mouseMove       (const juce::MouseEvent&) override;
    void mouseDown       (const juce::MouseEvent&) override;
    void mouseDrag       (const juce::MouseEvent&) override;
    void mouseUp         (const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove  (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray&, int, int) override { dropHighlight = true;  repaint(); }
    void fileDragExit (const juce::StringArray&) override           { dropHighlight = false; repaint(); }

private:
    enum class Zone { None, Body, TrimStart, TrimEnd, FadeIn, FadeOut };
    struct Hit { int track = -1; AudioClip* clip = nullptr; Zone zone = Zone::None; };

    Hit  hitTest(juce::Point<int> p) const;
    int  trackAtY(int y) const;
    juce::Rectangle<float> clipRect(int trackIndex, const AudioClip& clip) const;
    void drawClip(juce::Graphics&, juce::Rectangle<float> r, const AudioClip&, juce::Colour, bool selected);
    void showClipMenu(int trackIndex, AudioClip* clip);
    void showEmptyMenu(int trackIndex, double sec);
    void renameClip(AudioClip* clip);

    AudioEngine& engine;
    TrackPanel&  panel;

    // drag state
    Zone   dragZone { Zone::None };
    int    dragTrack { -1 };
    AudioClip* dragClip { nullptr };
    AudioClip  dragOrig;
    double dragMouseSec { 0.0 };
    bool   dragChanged { false };
    bool   undoPushed  { false };
    bool   dropHighlight { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangeCanvas)
};

// ============================================================
//  TrackPanel  --  arrangement view (ruler + headers + canvas)
// ============================================================
class TrackPanel : public juce::Component,
                   public juce::ChangeListener,
                   public juce::ScrollBar::Listener,
                   private juce::Timer
{
public:
    static constexpr int HEADER_W = 210;
    static constexpr int TRACK_H  = 86;
    static constexpr int RULER_H  = 34;

    explicit TrackPanel(AudioEngine& engine);
    ~TrackPanel() override;

    void resized() override;
    void paint   (juce::Graphics&) override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void scrollBarMoved(juce::ScrollBar*, double newRangeStart) override;

    void refreshTracks();

    // ---- view (time <-> pixels) ----
    double viewStart { 0.0 };
    double viewEnd   { 24.0 };
    double secToX(double sec, int width) const { return (sec - viewStart) / (viewEnd - viewStart) * width; }
    double xToSec(double x, int width)   const { return viewStart + x / (double)width * (viewEnd - viewStart); }
    void   zoomAround(double sec, double factor);
    void   scrollBy(double fractionOfView);
    void   updateScrollBar();
    double gridStep(int widthPx) const;              // musical grid in seconds
    double snap(double sec, int widthPx, bool enabled) const;

    // ---- selection ----
    int        selectedTrack { 0 };
    AudioClip* selectedClip  { nullptr };
    void selectTrack(int index);
    void selectClip(int trackIndex, AudioClip* clip);

    // ---- editing (all undoable) ----
    void cutSelectedClip();
    void copySelectedClip();
    void pasteClip();
    void deleteSelectedClip();
    void splitSelectedClipAtPlayhead();
    void duplicateSelectedClip();
    void importAudio(int trackIndex, double sec);
    bool importFile(const juce::File& f, int trackIndex, double sec);

    AudioEngine& getEngine() { return engine; }

private:
    void timerCallback() override;

    AudioEngine& engine;
    std::unique_ptr<AudioClip> clipboard;

    TimelineRuler   ruler;
    juce::Viewport  viewport;
    juce::Component container;
    ArrangeCanvas   canvas;
    juce::ScrollBar hScroll { false };
    IconButton      addTrackBtn { "Add track", IconButton::Icon::Plus };

    std::vector<std::unique_ptr<TrackHeader>> headers;
    size_t lastTrackCount { 0 };
    int    lastTransportState { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackPanel)
};

// ============================================================
//  FxCard  --  one device of the effect rack (Ableton style)
// ============================================================
class FxCard : public juce::Component
{
public:
    struct Param
    {
        juce::String name;
        std::atomic<float>* value;
        double min, max, interval;
        juce::String suffix;
    };

    FxCard(const juce::String& title, std::atomic<bool>& enabled, std::vector<Param> params,
           juce::Colour accent);
    void resized() override;
    void paint   (juce::Graphics&) override;
    void syncFromModel();

private:
    juce::String title;
    std::atomic<bool>& enabled;
    juce::Colour accent;
    juce::ToggleButton power;
    std::vector<Param> params;
    juce::OwnedArray<juce::Slider> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxCard)
};

// ============================================================
//  MixerPanel  --  channel strips + master + FX rack
// ============================================================
class MixerPanel : public juce::Component,
                   private juce::Timer
{
public:
    explicit MixerPanel(AudioEngine& engine);
    void resized() override;
    void paint   (juce::Graphics&) override;
    void refresh();
    void selectStrip(int index);

private:
    void timerCallback() override;
    void buildRack();

    AudioEngine& engine;

    struct ChannelStrip : public juce::Component
    {
        ChannelStrip(Track& t, int index, MixerPanel& owner);
        void resized() override;
        void paint   (juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override { owner.selectStrip(index); }
        void refreshFxButtons();

        Track& track;
        int index;
        MixerPanel& owner;
        bool selected { false };

        juce::Slider     eqHigh, eqMid, eqLow, panKnob, fader;
        juce::TextButton drvBtn { "DRV" }, cmpBtn { "CMP" }, dlyBtn { "DLY" }, revBtn { "REV" };
        juce::TextButton muteBtn { "M" }, soloBtn { "S" };
        juce::Rectangle<int> meterArea, eqLabels, dbArea, panArea;
    };

    struct MasterStrip : public juce::Component
    {
        explicit MasterStrip(AudioEngine& e);
        void resized() override;
        void paint   (juce::Graphics&) override;
        AudioEngine& engine;
        juce::Slider fader;
        juce::Rectangle<int> meterArea;
    };

    std::vector<std::unique_ptr<ChannelStrip>> strips;
    MasterStrip     master;
    juce::Viewport  scrollView;
    juce::Component stripContainer;

    int selected { 0 };
    juce::OwnedArray<FxCard> rack;
    juce::Rectangle<int> rackArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};

// ============================================================
//  PluginsPanel  --  basic VST3 scanning
// ============================================================
class PluginsPanel : public juce::Component,
                     public juce::ListBoxModel
{
public:
    explicit PluginsPanel(AudioEngine& engine);
    ~PluginsPanel() override;

    void resized() override;
    void paint   (juce::Graphics&) override;

    int  getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

private:
    AudioEngine& engine;
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList          knownPlugins;

    juce::ListBox    pluginList;
    juce::TextButton scanBtn { "SCAN VST3 FOLDER" };
    juce::TextButton loadBtn { "LOAD SELECTED" };
    juce::Label      statusLabel;

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
    juce::TextButton exportWavBtn { "EXPORT WAV" };
    juce::TextButton exportMp3Btn { "EXPORT MP3" };
    juce::ComboBox   bitrateBox;
    juce::Label      statusLabel;
    juce::Rectangle<int> card;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportPanel)
};

// ============================================================
//  SettingsPanel
// ============================================================
class SettingsPanel : public juce::Component
{
public:
    SettingsPanel(AudioEngine& engine, juce::PropertiesFile& settings);
    void resized() override;
    void paint   (juce::Graphics&) override;
    void visibilityChanged() override;

    std::function<void(bool)> onShowTipsChanged;
    std::function<void(int themeId, juce::uint32 accent)> onThemeChanged;
    std::function<void(int languageId)> onLanguageChanged;

private:
    void updateAccentVisibility();
    void openCustomColour();

    AudioEngine& engine;
    juce::PropertiesFile& settings;
    juce::AudioDeviceSelectorComponent deviceSelector;
    juce::ComboBox bufferSizeBox;
    juce::ComboBox themeBox, languageBox;
    juce::OwnedArray<ColourSwatch> swatches;
    juce::TextButton customColourBtn { "CUSTOM..." };
    juce::ToggleButton showTipsToggle { "Show quick-start tips when the program opens" };
    juce::Rectangle<int> accentRow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};

// ============================================================
//  MainComponent
// ============================================================
class MainComponent : public juce::Component,
                      public juce::KeyListener,
                      public juce::ChangeListener,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;
    void paint   (juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;

private:
    void timerCallback() override;
    void showDAW();
    void saveProject(bool saveAs);
    void openProject();
    void writeProject(const juce::File& f);
    void setStatus(const juce::String& msg);
    void showTip(bool show);
    void buildUI();
    void applyTheme(int themeId, juce::uint32 accent, bool rebuild);
    void applyLanguage(int languageId);
    void rebuildUI();
    juce::PropertiesFile& settings() { return *appProps.getUserSettings(); }

    InUteroTheme theme;
    juce::ApplicationProperties appProps;
    AudioEngine  engine;
    juce::TooltipWindow tooltipWindow { this, 600 };

    std::unique_ptr<SplashScreenComponent> splash;
    std::unique_ptr<TopBar>                topBar;
    std::unique_ptr<juce::TabbedComponent> tabs;
    std::unique_ptr<QuickStartTip>         tip;

    TrackPanel*    trackPanel { nullptr };
    MixerPanel*    mixerPanel { nullptr };

    juce::File   currentProjectFile;
    juce::String statusMessage;
    double       statusTime { 0.0 };

    static constexpr int TOPBAR_H = 64;
    static constexpr int STATUS_H = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
