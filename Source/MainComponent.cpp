#include "MainComponent.h"
#include <cmath>

// ============================================================
//  Colour helper
// ============================================================
static inline juce::Colour iu(juce::uint32 h) { return juce::Colour(h); }
using namespace InUtero;

// ============================================================
//  SplashScreenComponent
// ============================================================
SplashScreenComponent::SplashScreenComponent(std::function<void()> onFinished)
    : finishedCallback(std::move(onFinished))
{
    startTimeMs = juce::Time::getCurrentTime().toMilliseconds();
    startTimerHz(60);
    setInterceptsMouseClicks(false, false);
}

void SplashScreenComponent::paint(juce::Graphics& g)
{
    g.setColour(iu(Negro).withAlpha(alpha));
    g.fillAll();

    float cx = (float)getWidth()  * 0.5f;
    float cy = (float)getHeight() * 0.5f;

    // Grain texture overlay
    juce::Random rng(42);
    g.setColour(iu(CremaOscuro).withAlpha(alpha * 0.03f));
    for (int i = 0; i < 3000; ++i)
    {
        float gx = rng.nextFloat() * (float)getWidth();
        float gy = rng.nextFloat() * (float)getHeight();
        g.fillRect(gx, gy, 1.0f, 1.0f);
    }

    // Vignette
    juce::ColourGradient vgr(juce::Colour(0x00000000), cx, cy,
                              juce::Colour(0xCC000000), 0.0f, 0.0f, true);
    g.setGradientFill(vgr);
    g.setOpacity(alpha);
    g.fillAll();

    // Horizontal rules
    g.setColour(iu(VerdeMosgo).withAlpha(alpha * 0.6f));
    g.drawHorizontalLine((int)(cy - 54), cx - 140.0f, cx + 140.0f);
    g.drawHorizontalLine((int)(cy + 56), cx - 140.0f, cx + 140.0f);

    // Title
    g.setColour(iu(Crema).withAlpha(alpha));
    g.setFont(juce::Font(juce::FontOptions().withHeight(58.0f).withStyle("Bold")));
    g.drawText("Pennyroyal Audio", 0, (int)(cy - 50), getWidth(), 70,
               juce::Justification::centred);

    // Subtitle
    g.setFont(juce::Font(juce::FontOptions().withHeight(15.0f)));
    g.setColour(iu(CremaOscuro).withAlpha(alpha));
    g.drawText("Professional Digital Audio Workstation",
               0, (int)(cy + 28), getWidth(), 26, juce::Justification::centred);

    // Version
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    g.setColour(iu(GrisClaro).withAlpha(alpha));
    g.drawText("v1.1.0  --  JUCE 8  --  In Utero Edition",
               0, (int)(cy + 70), getWidth(), 18, juce::Justification::centred);
}

void SplashScreenComponent::timerCallback()
{
    juce::int64 elapsed = juce::Time::getCurrentTime().toMilliseconds() - startTimeMs;

    if (!fadingOut && elapsed >= HOLD_MS)
    {
        fadingOut   = true;
        startTimeMs = juce::Time::getCurrentTime().toMilliseconds();
    }

    if (fadingOut)
    {
        juce::int64 fe = juce::Time::getCurrentTime().toMilliseconds() - startTimeMs;
        alpha = 1.0f - (float)fe / (float)FADE_MS;
        if (alpha <= 0.0f)
        {
            alpha = 0.0f;
            stopTimer();
            if (finishedCallback) finishedCallback();
            return;
        }
    }
    repaint();
}

// ============================================================
//  TransportBar
// ============================================================
TransportBar::TransportBar(AudioEngine& eng) : engine(eng)
{
    for (auto* btn : { &playBtn, &stopBtn, &recBtn, &loopBtn })
        addAndMakeVisible(btn);

    addAndMakeVisible(monitorBtn);
    addAndMakeVisible(metroBtn);
    addAndMakeVisible(masterVolSlider);
    addAndMakeVisible(posLabel);
    addAndMakeVisible(bpmLabel);
    addAndMakeVisible(bpmSlider);
    addAndMakeVisible(recordTrackBox);
    addAndMakeVisible(recordTrackLabel);
    addAndMakeVisible(volLabel);

    for (auto* btn : { &monitorBtn, &metroBtn })
    {
        btn->setClickingTogglesState(true);
        btn->setColour(juce::TextButton::buttonOnColourId,  iu(VerdeMosgo));
        btn->setColour(juce::TextButton::buttonColourId,    iu(GrisOscuro));
        btn->setColour(juce::TextButton::textColourOffId,   iu(GrisClaro));
    }

    playBtn.onClick = [this] {
        auto state = engine.getTransportState();
        if (state == AudioEngine::TransportState::Playing)
            engine.pause();
        else if (state == AudioEngine::TransportState::Stopped)
            engine.play();
        updateButtonStates();
    };

    stopBtn.onClick = [this] {
        if (engine.getTransportState() == AudioEngine::TransportState::Recording)
        {
            engine.stopRecording();
            engine.stop();
            engine.setInputMonitorEnabled(false);
        }
        else
        {
            engine.stop();
        }
        updateButtonStates();
    };

    recBtn.onClick = [this] {
        if (engine.getTransportState() == AudioEngine::TransportState::Recording)
        {
            engine.stopRecording();
            engine.setInputMonitorEnabled(false);
        }
        else if (engine.getTransportState() != AudioEngine::TransportState::Playing)
        {
            int selectedTrack = recordTrackBox.getSelectedItemIndex();
            if (selectedTrack < 0) selectedTrack = 0;

            auto& tracks = engine.getProject().tracks;
            if (!tracks.empty() && selectedTrack < (int)tracks.size())
            {
                for (auto& t : tracks) t->armed = false;
                tracks[selectedTrack]->armed = true;
            }
            engine.setInputMonitorEnabled(true);
            engine.startRecording(selectedTrack);
        }
        updateButtonStates();
    };

    loopBtn.setClickingTogglesState(true);
    loopBtn.onClick = [this] {
        engine.setLoopEnabled(loopBtn.getToggleState());
        updateButtonStates();
    };
    loopBtn.setTooltip("Loop region on/off");

    monitorBtn.onClick = [this] {
        engine.setInputMonitorEnabled(monitorBtn.getToggleState());
    };
    monitorBtn.setTooltip("Monitor input (hear mic/guitar live)");

    metroBtn.onClick = [this] {
        engine.setMetronomeEnabled(metroBtn.getToggleState());
    };
    metroBtn.setTooltip("Metronome click");

    recordTrackLabel.setText("REC TO:", juce::dontSendNotification);
    recordTrackLabel.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    recordTrackLabel.setColour(juce::Label::textColourId, iu(GrisClaro));
    refreshTrackList();

    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    bpmLabel.setColour(juce::Label::textColourId, iu(GrisClaro));
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setRange(40.0, 240.0, 0.5);
    bpmSlider.setValue(120.0, juce::dontSendNotification);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 44, 18);
    bpmSlider.addListener(this);
    bpmSlider.setTooltip("Tempo in BPM");

    volLabel.setText("VOL", juce::dontSendNotification);
    volLabel.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    volLabel.setColour(juce::Label::textColourId, iu(GrisClaro));
    masterVolSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterVolSlider.setRange(0.0, 1.5, 0.01);
    masterVolSlider.setValue(0.85, juce::dontSendNotification);
    masterVolSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterVolSlider.addListener(this);
    masterVolSlider.setTooltip("Master output volume");

    posLabel.setJustificationType(juce::Justification::centred);
    posLabel.setFont(juce::Font(juce::FontOptions().withHeight(16.0f).withStyle("Bold")));
    posLabel.setColour(juce::Label::textColourId, iu(Crema));

    playBtn.setTooltip("Play / Pause [Space]");
    stopBtn.setTooltip("Stop and return to start");
    recBtn.setTooltip("Record on selected track");

    startTimerHz(20);
}

void TransportBar::refreshTrackList()
{
    recordTrackBox.clear(juce::dontSendNotification);
    auto& tracks = engine.getProject().tracks;
    for (int i = 0; i < (int)tracks.size(); ++i)
        recordTrackBox.addItem(tracks[i]->name, i + 1);
    if (!tracks.empty())
        recordTrackBox.setSelectedItemIndex(0, juce::dontSendNotification);
}

void TransportBar::resized()
{
    auto area = getLocalBounds().reduced(8, 5);

    const int bigBtnW = 60, bigBtnH = 28;
    for (auto* btn : { &playBtn, &stopBtn, &recBtn })
    {
        btn->setBounds(area.removeFromLeft(bigBtnW).withSizeKeepingCentre(bigBtnW, bigBtnH));
        area.removeFromLeft(4);
    }

    area.removeFromLeft(6);

    loopBtn.setBounds(area.removeFromLeft(48).withSizeKeepingCentre(48, bigBtnH));
    area.removeFromLeft(4);

    const int smallH = 24;
    monitorBtn.setBounds(area.removeFromLeft(38).withSizeKeepingCentre(38, smallH));
    area.removeFromLeft(3);
    metroBtn.setBounds(area.removeFromLeft(44).withSizeKeepingCentre(44, smallH));
    area.removeFromLeft(10);

    recordTrackLabel.setBounds(area.removeFromLeft(42).withSizeKeepingCentre(42, 13));
    recordTrackBox.setBounds(area.removeFromLeft(108).withSizeKeepingCentre(108, 24));
    area.removeFromLeft(12);

    posLabel.setBounds(area.removeFromLeft(100).withSizeKeepingCentre(100, area.getHeight()));
    area.removeFromLeft(8);

    bpmLabel.setBounds(area.removeFromLeft(28).withSizeKeepingCentre(28, 13));
    bpmSlider.setBounds(area.removeFromLeft(130).withSizeKeepingCentre(130, 22));
    area.removeFromLeft(12);

    volLabel.setBounds(area.removeFromLeft(24).withSizeKeepingCentre(24, 13));
    masterVolSlider.setBounds(area.removeFromLeft(90).withSizeKeepingCentre(90, 22));
}

void TransportBar::paint(juce::Graphics& g)
{
    g.setColour(iu(NegroSuave));
    g.fillAll();
    g.setColour(iu(GrisMedio));
    g.drawHorizontalLine(0, 0.0f, (float)getWidth());

    // Level meters
    const int mX = getWidth() - 26;
    const int mH = getHeight() - 10;
    auto drawMeter = [&](int x, float level, juce::Colour col)
    {
        int filled = (int)(juce::jlimit(0.0f, 1.0f, level) * mH);
        g.setColour(iu(GrisOscuro));
        g.fillRect(x, 5, 8, mH);
        g.setColour(col.withAlpha(0.9f));
        g.fillRect(x, 5 + mH - filled, 8, filled);
    };
    drawMeter(mX,      levelL, iu(VerdeClaro));
    drawMeter(mX + 10, levelR, iu(VerdeClaro));
}

void TransportBar::sliderValueChanged(juce::Slider* s)
{
    if (s == &masterVolSlider)
        engine.setMasterVolume((float)masterVolSlider.getValue());
    if (s == &bpmSlider)
        engine.getProject().bpm = bpmSlider.getValue();
}

void TransportBar::timerCallback()
{
    double pos = engine.getPlayheadPositionSec();
    int m  = (int)(pos / 60.0);
    int s  = (int)pos % 60;
    int ms = (int)((pos - std::floor(pos)) * 100.0);

    posLabel.setText(
        juce::String(m) + ":" +
        (s  < 10 ? "0" : "") + juce::String(s)  + "." +
        (ms < 10 ? "0" : "") + juce::String(ms),
        juce::dontSendNotification);

    levelL = levelL * 0.85f + engine.getOutputLevelL() * 0.15f;
    levelR = levelR * 0.85f + engine.getOutputLevelR() * 0.15f;
    repaint();
    updateButtonStates();
}

void TransportBar::updateButtonStates()
{
    auto state      = engine.getTransportState();
    bool isRecording = (state == AudioEngine::TransportState::Recording);
    bool isPlaying   = (state == AudioEngine::TransportState::Playing);

    playBtn.setButtonText(isPlaying ? "PAUSE" : "PLAY");
    playBtn.setEnabled(!isRecording);

    recBtn.setColour(juce::TextButton::buttonColourId,
        isRecording ? iu(Rojo) : iu(GrisOscuro));
    recBtn.setColour(juce::TextButton::textColourOffId,
        isRecording ? iu(CremaClaro) : iu(CremaOscuro));
    recBtn.setEnabled(!isPlaying);

    stopBtn.setEnabled(isRecording || isPlaying);

    loopBtn.setToggleState(engine.isLoopEnabled(), juce::dontSendNotification);
    monitorBtn.setToggleState(engine.isInputMonitorEnabled(), juce::dontSendNotification);
    metroBtn.setToggleState(engine.isMetronomeEnabled(), juce::dontSendNotification);
}

// ============================================================
//  TimelineRuler
// ============================================================
TimelineRuler::TimelineRuler()
{
    setInterceptsMouseClicks(true, false);
}

void TimelineRuler::setVisibleRange(double s, double e) { viewStart=s; viewEnd=e; repaint(); }
void TimelineRuler::setPlayheadPos (double s)            { playheadSec=s;          repaint(); }
void TimelineRuler::setLoopRegion  (double s, double e, bool en)
{
    loopStart = s; loopEnd = e; loopEnabled = en; repaint();
}

double TimelineRuler::xToSec(int x) const
{
    double range = viewEnd - viewStart;
    if (range <= 0.0) return viewStart;
    return viewStart + ((double)x / (double)getWidth()) * range;
}

float TimelineRuler::secToX(double sec) const
{
    double range = viewEnd - viewStart;
    if (range <= 0.0) return 0.0f;
    return (float)((sec - viewStart) / range * getWidth());
}

void TimelineRuler::paint(juce::Graphics& g)
{
    const float w = (float)getWidth();
    const float h = (float)getHeight();

    g.setColour(iu(NegroSuave));
    g.fillAll();

    const double range = viewEnd - viewStart;
    if (range <= 0.0) return;

    // Loop region shading
    if (loopEnabled && loopEnd > loopStart)
    {
        float lx1 = secToX(loopStart);
        float lx2 = secToX(loopEnd);
        g.setColour(iu(VerdeMosgo).withAlpha(0.15f));
        g.fillRect(lx1, 0.0f, lx2 - lx1, h);

        // Loop bracket lines
        g.setColour(iu(VerdeMosgo).withAlpha(0.7f));
        g.drawVerticalLine((int)lx1, 0.0f, h);
        g.drawVerticalLine((int)lx2, 0.0f, h);

        // Handle markers
        g.setColour(iu(VerdeClaro));
        g.fillRect(lx1 - 1.0f, 0.0f, 3.0f, 8.0f);
        g.fillRect(lx2 - 1.0f, 0.0f, 3.0f, 8.0f);
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        g.drawText("IN",  (int)lx1 + 3, 0,  18, (int)h, juce::Justification::centredLeft);
        g.drawText("OUT", (int)lx2 + 3, 0,  24, (int)h, juce::Justification::centredLeft);
    }

    // Time grid
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
    for (double t = std::floor(viewStart); t <= viewEnd; t += 1.0)
    {
        float x     = secToX(t);
        bool isBar  = ((int)t % 4 == 0);
        g.setColour(isBar ? iu(GrisClaro) : iu(GrisMedio));
        g.drawVerticalLine((int)x, isBar ? 0.0f : h * 0.5f, h);
        if (isBar)
        {
            g.setColour(iu(CremaOscuro));
            g.drawText(juce::String((int)t) + "s",
                       (int)x + 2, 0, 40, (int)h,
                       juce::Justification::centredLeft);
        }
    }

    // Playhead
    float phX = secToX(playheadSec);
    if (phX >= 0.0f && phX <= w)
    {
        g.setColour(iu(RojoClaro));
        g.drawVerticalLine((int)phX, 0.0f, h);
        juce::Path tri;
        tri.addTriangle(phX - 5.0f, 0.0f, phX + 5.0f, 0.0f, phX, 8.0f);
        g.fillPath(tri);
    }

    // Border
    g.setColour(iu(GrisMedio));
    g.drawHorizontalLine((int)h - 1, 0.0f, w);
}

void TimelineRuler::mouseDown(const juce::MouseEvent& e)
{
    const float HANDLE_TOL = 6.0f;

    if (loopEnabled)
    {
        float lsX = secToX(loopStart);
        float leX = secToX(loopEnd);

        if (std::abs((float)e.x - lsX) < HANDLE_TOL)
        {
            dragMode = DragMode::LoopStart;
            return;
        }
        if (std::abs((float)e.x - leX) < HANDLE_TOL)
        {
            dragMode = DragMode::LoopEnd;
            return;
        }
    }

    dragMode = DragMode::Seek;
    double t = xToSec(e.x);
    if (onSeek) onSeek(t);
}

void TimelineRuler::mouseDrag(const juce::MouseEvent& e)
{
    double t = xToSec(e.x);
    if (dragMode == DragMode::Seek)
    {
        if (onSeek) onSeek(t);
    }
    else if (dragMode == DragMode::LoopStart)
    {
        loopStart = std::max(0.0, std::min(t, loopEnd - 0.5));
        if (onLoopChanged) onLoopChanged(loopStart, loopEnd);
        repaint();
    }
    else if (dragMode == DragMode::LoopEnd)
    {
        loopEnd = std::max(loopStart + 0.5, t);
        if (onLoopChanged) onLoopChanged(loopStart, loopEnd);
        repaint();
    }
}

void TimelineRuler::mouseUp(const juce::MouseEvent&)
{
    dragMode = DragMode::None;
}

// ============================================================
//  TrackHeader
// ============================================================
TrackHeader::TrackHeader(Track& t, AudioEngine& eng, int idx)
    : track(t), engine(eng), trackIndex(idx)
{
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(volSlider);
    addAndMakeVisible(panSlider);
    addAndMakeVisible(muteBtn);
    addAndMakeVisible(soloBtn);
    addAndMakeVisible(armBtn);
    addAndMakeVisible(deleteBtn);

    nameLabel.setText(track.name, juce::dontSendNotification);
    nameLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
    nameLabel.setColour(juce::Label::textColourId, iu(Crema));

    volSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volSlider.setRange(0.0, 1.5, 0.01);
    volSlider.setValue(track.volume, juce::dontSendNotification);
    volSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volSlider.addListener(this);
    volSlider.setTooltip("Track Volume");

    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.setValue(track.pan, juce::dontSendNotification);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.addListener(this);
    panSlider.setTooltip("Pan (L <-> R)");

    muteBtn.setClickingTogglesState(true);
    soloBtn.setClickingTogglesState(true);
    armBtn.setClickingTogglesState(true);

    muteBtn.setToggleState(track.muted,  juce::dontSendNotification);
    soloBtn.setToggleState(track.soloed, juce::dontSendNotification);
    armBtn.setToggleState (track.armed,  juce::dontSendNotification);

    muteBtn.addListener(this);
    soloBtn.addListener(this);
    armBtn.addListener(this);
    deleteBtn.addListener(this);

    muteBtn.setTooltip("Mute track (M)");
    soloBtn.setTooltip("Solo track (S)");
    armBtn.setTooltip ("Arm for recording (R)");
    deleteBtn.setTooltip("Delete this track");

    // Delete button danger style
    deleteBtn.setColour(juce::TextButton::buttonColourId,  iu(GrisOscuro));
    deleteBtn.setColour(juce::TextButton::textColourOffId, iu(Rojo));
}

void TrackHeader::resized()
{
    auto a = getLocalBounds().reduced(5, 3);

    // Row 1: name + X delete button
    auto topRow = a.removeFromTop(18);
    deleteBtn.setBounds(topRow.removeFromRight(16).reduced(1));
    nameLabel.setBounds(topRow);
    a.removeFromTop(3);

    // Row 2: M S R buttons + PAN slider
    auto midRow = a.removeFromTop(22);
    muteBtn.setBounds(midRow.removeFromLeft(20).reduced(1));
    soloBtn.setBounds(midRow.removeFromLeft(20).reduced(1));
    armBtn.setBounds (midRow.removeFromLeft(20).reduced(1));
    midRow.removeFromLeft(22); // space for "PAN" label drawn in paint()
    panSlider.setBounds(midRow.reduced(0, 3));

    a.removeFromTop(3);

    // Row 3: VOL slider
    auto botRow = a.removeFromTop(18);
    botRow.removeFromLeft(22); // space for "VOL" label drawn in paint()
    volSlider.setBounds(botRow);
}

void TrackHeader::paint(juce::Graphics& g)
{
    g.setColour(iu(NegroSuave));
    g.fillAll();

    // Color accent strip on left
    g.setColour(track.colour.withAlpha(0.7f));
    g.fillRect(0, 0, 3, getHeight());

    // Border
    g.setColour(iu(GrisMedio));
    g.drawRect(getLocalBounds(), 1);

    // Draw PAN / VOL micro-labels
    g.setColour(iu(GrisClaro));
    g.setFont(juce::FontOptions().withHeight(9.0f).withStyle("Bold"));

    auto bounds = getLocalBounds().reduced(5, 3);
    bounds.removeFromTop(21); // skip name row

    auto midRow = bounds.removeFromTop(22);
    midRow.removeFromLeft(60);
    g.drawText("PAN", midRow.removeFromLeft(22), juce::Justification::centred);

    bounds.removeFromTop(3);
    auto botRow = bounds.removeFromTop(18);
    g.drawText("VOL", botRow.removeFromLeft(22), juce::Justification::centred);
}

void TrackHeader::mouseDoubleClick(const juce::MouseEvent&)
{
    // Double-click: edit track name inline
    // setEditable must be called before showEditor(); createEditorComponent() is protected in JUCE 8
    nameLabel.setEditable(true, true, false);
    nameLabel.showEditor();
    nameLabel.onEditorHide = [this]
    {
        track.name = nameLabel.getText();
        nameLabel.setEditable(false, false, false);
    };
}

void TrackHeader::sliderValueChanged(juce::Slider* s)
{
    if (s == &volSlider) track.volume = (float)volSlider.getValue();
    if (s == &panSlider) track.pan    = (float)panSlider.getValue();
}

void TrackHeader::buttonClicked(juce::Button* b)
{
    if (b == &muteBtn)   track.muted  = muteBtn.getToggleState();
    if (b == &soloBtn)   track.soloed = soloBtn.getToggleState();
    if (b == &armBtn)    track.armed  = armBtn.getToggleState();
    if (b == &deleteBtn)
    {
        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::WarningIcon,
            "Delete Track",
            "Delete track \"" + track.name + "\"? This cannot be undone.",
            "Delete", "Cancel",
            nullptr,
            juce::ModalCallbackFunction::create([this](int result)
            {
                if (result == 1 && onDeleteRequest)
                    onDeleteRequest(trackIndex);
            }));
    }
}

// ============================================================
//  TrackLane
// ============================================================
TrackLane::TrackLane(Track& t, double& vs, double& ve)
    : track(t), viewStart(vs), viewEnd(ve)
{
    setInterceptsMouseClicks(true, false);
}

double TrackLane::xToSec(int x) const
{
    double range = viewEnd - viewStart;
    return viewStart + ((double)x / (double)getWidth()) * range;
}

void TrackLane::paint(juce::Graphics& g)
{
    g.setColour(iu(Negro));
    g.fillAll();

    const float w     = (float)getWidth();
    const float h     = (float)getHeight();
    const double range = viewEnd - viewStart;

    // Subtle grid lines
    g.setColour(iu(GrisOscuro).withAlpha(0.5f));
    for (double t = std::floor(viewStart); t <= viewEnd; t += 4.0)
    {
        float x = (float)((t - viewStart) / range * w);
        g.drawVerticalLine((int)x, 0.0f, h);
    }

    // Clips
    for (auto& clipPtr : track.clips)
    {
        if (clipPtr->buffer.getNumSamples() == 0) continue;
        float cxStart = (float)((clipPtr->startTimeSec - viewStart) / range * w);
        float cxEnd   = (float)((clipPtr->startTimeSec + clipPtr->lengthSec - viewStart) / range * w);
        float cw      = std::max(2.0f, cxEnd - cxStart);

        bool isSelected = (clipPtr.get() == selectedClip);
        juce::Colour col = track.colour;

        // Clip fill
        g.setColour(col.withAlpha(isSelected ? 0.35f : 0.20f));
        g.fillRoundedRectangle(cxStart, 2.0f, cw, h - 4.0f, 3.0f);

        // Clip border (brighter when selected)
        g.setColour(isSelected ? col.withAlpha(0.95f) : col.withAlpha(0.55f));
        g.drawRoundedRectangle(cxStart, 2.0f, cw, h - 4.0f, 3.0f,
                               isSelected ? 1.8f : 1.0f);

        // Selection highlight top bar
        if (isSelected)
        {
            g.setColour(iu(VerdeClaro).withAlpha(0.6f));
            g.fillRect(cxStart, 2.0f, cw, 2.0f);
        }

        // Waveform
        const int numSamp = clipPtr->buffer.getNumSamples();
        if (numSamp > 0 && clipPtr->buffer.getNumChannels() > 0)
        {
            const float* data = clipPtr->buffer.getReadPointer(0);
            float mid   = h * 0.5f;
            int   step  = std::max(1, numSamp / (int)cw);
            juce::Path wv;
            bool first = true;
            for (int px = 0; px < (int)cw; ++px)
            {
                int idx = px * step;
                if (idx >= numSamp) break;
                float val   = data[idx] * (mid - 4.0f) * 0.8f;
                float pxAbs = cxStart + (float)px;
                if (first) { wv.startNewSubPath(pxAbs, mid - val); first = false; }
                else        wv.lineTo           (pxAbs, mid - val);
            }
            g.setColour(col.withAlpha(isSelected ? 0.9f : 0.75f));
            g.strokePath(wv, juce::PathStrokeType(1.0f));
        }

        // Clip name
        g.setColour(iu(CremaOscuro).withAlpha(0.8f));
        g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
        g.drawText(clipPtr->name,
                   (int)cxStart + 4, 4, (int)cw - 8, 12,
                   juce::Justification::centredLeft, true);
    }

    // Bottom separator
    g.setColour(iu(GrisMedio).withAlpha(0.4f));
    g.drawHorizontalLine((int)h - 1, 0.0f, w);
}

void TrackLane::resized() {}

void TrackLane::mouseDown(const juce::MouseEvent& e)
{
    double sec = xToSec(e.x);
    for (auto& c : track.clips)
    {
        if (sec >= c->startTimeSec && sec <= c->startTimeSec + c->lengthSec)
        {
            selectedClip = c.get();
            repaint();
            if (onClipSelected) onClipSelected(&track, c.get());
            return;
        }
    }
    selectedClip = nullptr;
    repaint();
    if (onClipSelected) onClipSelected(nullptr, nullptr);
}

void TrackLane::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Double-click on a clip: edit its name inline via popup
    double sec = xToSec(e.x);
    for (auto& c : track.clips)
    {
        if (sec >= c->startTimeSec && sec <= c->startTimeSec + c->lengthSec)
        {
            // Show a simple AlertWindow to rename
            auto* dlg = new juce::AlertWindow("Rename Clip", "Enter new name:", juce::AlertWindow::NoIcon);
            dlg->addTextEditor("name", c->name, "Name:");
            dlg->addButton("OK",     1, juce::KeyPress(juce::KeyPress::returnKey));
            dlg->addButton("Cancel", 0);

            auto* clipRaw = c.get();
            dlg->enterModalState(true, juce::ModalCallbackFunction::create([dlg, clipRaw, this](int res)
            {
                if (res == 1)
                {
                    juce::String newName = dlg->getTextEditorContents("name");
                    if (newName.isNotEmpty())
                        clipRaw->name = newName;
                    repaint();
                }
            }), true);
            return;
        }
    }

    // Double-click on empty: import audio file
    double targetSec = sec;
    auto chooser = std::make_shared<juce::FileChooser>(
        "Import Audio",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aiff;*.mp3;*.flac;*.ogg");

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this, targetSec, chooser](const juce::FileChooser& fc)
        {
            auto results = fc.getResults();
            if (results.isEmpty()) return;

            juce::AudioFormatManager fmt;
            fmt.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader(
                fmt.createReaderFor(results[0]));
            if (!reader) return;

            auto* clip = track.addClip();
            clip->buffer.setSize((int)reader->numChannels,
                                 (int)reader->lengthInSamples);
            reader->read(&clip->buffer, 0,
                         (int)reader->lengthInSamples, 0, true, true);
            clip->sampleRate     = reader->sampleRate;
            clip->startTimeSec   = std::max(0.0, targetSec);
            clip->name           = results[0].getFileNameWithoutExtension();
            clip->sourceFilePath = results[0].getFullPathName();
            clip->refreshLength();
            repaint();
        });
}

void TrackLane::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (onZoom)
        onZoom((double)w.deltaY, e.mods.isCommandDown());
}

// ============================================================
//  TrackPanel
// ============================================================
TrackPanel::TrackPanel(AudioEngine& eng) : engine(eng)
{
    engine.addChangeListener(this);
    addAndMakeVisible(ruler);
    addAndMakeVisible(viewport);
    addAndMakeVisible(addTrackBtn);

    viewport.setViewedComponent(&tracksContainer, false);
    viewport.setScrollBarsShown(true, true);

    ruler.onSeek = [this](double t) { engine.setPlayheadPositionSec(t); };

    ruler.onLoopChanged = [this](double s, double e)
    {
        engine.setLoopPoints(s, e);
        engine.getProject().loopStart = s;
        engine.getProject().loopEnd   = e;
    };

    addTrackBtn.onClick = [this] {
        engine.getProject().addTrack("Audio");
        refreshTracks();
        engine.sendChangeMessage();
    };

    refreshTracks();
    startTimerHz(20);
}

TrackPanel::~TrackPanel()
{
    engine.removeChangeListener(this);
    stopTimer();
}

void TrackPanel::refreshTracks()
{
    headers.clear();
    lanes.clear();
    tracksContainer.removeAllChildren();
    selectedTrack = nullptr;
    selectedClip  = nullptr;

    auto& tracks = engine.getProject().tracks;
    for (int i = 0; i < (int)tracks.size(); ++i)
    {
        auto* t    = tracks[i].get();
        auto  hdr  = std::make_unique<TrackHeader>(*t, engine, i);
        auto  lane = std::make_unique<TrackLane>  (*t, viewStart, viewEnd);

        hdr->onDeleteRequest = [this](int idx)
        {
            engine.removeTrackEQ(idx);
            engine.getProject().removeTrack(idx);
            refreshTracks();
            engine.sendChangeMessage();
        };

        lane->onClipSelected = [this, t](Track* tr, AudioClip* cl)
        {
            selectedTrack = tr;
            selectedClip  = cl;
            // Deselect other lanes
            for (auto& l : lanes)
                if (l->getSelectedClip() != cl)
                    l->setSelectedClip(nullptr);
        };

        lane->onZoom = [this](double delta, bool isCtrlHeld)
        {
            if (isCtrlHeld)
            {
                double factor = (delta > 0.0) ? 0.8 : 1.25;
                double centre = (viewStart + viewEnd) * 0.5;
                double range  = (viewEnd - viewStart) * factor;
                viewStart = centre - range * 0.5;
                viewEnd   = centre + range * 0.5;
                viewStart = std::max(0.0, viewStart);
                viewEnd   = std::max(viewStart + 1.0, viewEnd);
            }
            else
            {
                double shift = (viewEnd - viewStart) * 0.1 * (delta > 0.0 ? -1.0 : 1.0);
                viewStart = std::max(0.0, viewStart + shift);
                viewEnd   = viewEnd + shift;
            }
            resized();
        };

        tracksContainer.addAndMakeVisible(hdr.get());
        tracksContainer.addAndMakeVisible(lane.get());
        headers.push_back(std::move(hdr));
        lanes.push_back  (std::move(lane));
    }
    resized();
}

void TrackPanel::resized()
{
    auto area    = getLocalBounds();
    auto topArea = area.removeFromTop(RULER_H);
    addTrackBtn.setBounds(area.removeFromBottom(30));

    topArea.removeFromLeft(HEADER_W);
    ruler.setBounds(topArea);

    int laneW  = std::max(viewport.getWidth() - HEADER_W, (int)(viewEnd * 30));
    int totalH = TRACK_H * (int)headers.size();
    tracksContainer.setBounds(0, 0, HEADER_W + laneW, totalH);
    viewport.setBounds(area);

    for (int i = 0; i < (int)headers.size(); ++i)
    {
        headers[i]->setBounds(0,        i * TRACK_H, HEADER_W, TRACK_H);
        lanes[i]  ->setBounds(HEADER_W, i * TRACK_H, laneW,    TRACK_H);
    }
}

void TrackPanel::paint(juce::Graphics& g)
{
    g.setColour(iu(Negro));
    g.fillAll();
}

void TrackPanel::timerCallback()
{
    playheadSec = engine.getPlayheadPositionSec();
    if (playheadSec > viewEnd - 2.0)
        viewEnd = playheadSec + 10.0;

    auto& proj = engine.getProject();
    ruler.setPlayheadPos (playheadSec);
    ruler.setVisibleRange(viewStart, viewEnd);
    ruler.setLoopRegion  (proj.loopStart, proj.loopEnd, proj.loopEnabled);

    for (auto& l : lanes) l->repaint();
}

void TrackPanel::changeListenerCallback(juce::ChangeBroadcaster*)
{
    refreshTracks();
}

void TrackPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (e.mods.isCommandDown())
    {
        double factor = (w.deltaY > 0.0f) ? 0.8 : 1.25;
        double centre = (viewStart + viewEnd) * 0.5;
        double range  = (viewEnd - viewStart) * factor;
        viewStart = std::max(0.0, centre - range * 0.5);
        viewEnd   = std::max(viewStart + 1.0, centre + range * 0.5);
        resized();
    }
}

// Clipboard operations
void TrackPanel::cutSelectedClip()
{
    if (!selectedClip || !selectedTrack) return;
    clipboard.clip = std::make_unique<AudioClip>(*selectedClip);
    // Find and remove
    auto& clips = selectedTrack->clips;
    for (int i = 0; i < (int)clips.size(); ++i)
    {
        if (clips[i].get() == selectedClip)
        {
            clips.erase(clips.begin() + i);
            break;
        }
    }
    selectedClip = nullptr;
    for (auto& l : lanes) l->setSelectedClip(nullptr);
    repaint();
}

void TrackPanel::copySelectedClip()
{
    if (!selectedClip) return;
    clipboard.clip = std::make_unique<AudioClip>(*selectedClip);
}

void TrackPanel::pasteClip()
{
    if (!clipboard.hasData() || !selectedTrack) return;
    auto* newClip = selectedTrack->addClip();
    *newClip = *clipboard.clip;
    newClip->startTimeSec = engine.getPlayheadPositionSec();
    repaint();
}

void TrackPanel::deleteSelectedClip()
{
    if (!selectedClip || !selectedTrack) return;
    auto& clips = selectedTrack->clips;
    for (int i = 0; i < (int)clips.size(); ++i)
    {
        if (clips[i].get() == selectedClip)
        {
            clips.erase(clips.begin() + i);
            break;
        }
    }
    selectedClip = nullptr;
    for (auto& l : lanes) l->setSelectedClip(nullptr);
    repaint();
}

// ============================================================
//  MixerPanel::ChannelStrip
// ============================================================
MixerPanel::ChannelStrip::ChannelStrip(Track& t, AudioEngine& eng)
    : track(t), engine(eng)
{
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(volFader);
    addAndMakeVisible(eqLowKnob);
    addAndMakeVisible(eqMidKnob);
    addAndMakeVisible(eqHighKnob);
    addAndMakeVisible(eqLowLabel);
    addAndMakeVisible(eqMidLabel);
    addAndMakeVisible(eqHighLabel);
    addAndMakeVisible(muteBtn);
    addAndMakeVisible(soloBtn);

    nameLabel.setText(track.name, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    nameLabel.setColour(juce::Label::textColourId, iu(CremaOscuro));

    auto setupKnob = [](juce::Slider& s, double initVal)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRange(-12.0, 12.0, 0.1);
        s.setValue(initVal, juce::dontSendNotification);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    };
    setupKnob(eqLowKnob,  (double)track.eqLow.gainDb);
    setupKnob(eqMidKnob,  (double)track.eqMid.gainDb);
    setupKnob(eqHighKnob, (double)track.eqHigh.gainDb);

    eqLowLabel.setText ("LO",  juce::dontSendNotification);
    eqMidLabel.setText ("MID", juce::dontSendNotification);
    eqHighLabel.setText("HI",  juce::dontSendNotification);
    for (auto* l : { &eqLowLabel, &eqMidLabel, &eqHighLabel })
    {
        l->setJustificationType(juce::Justification::centred);
        l->setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        l->setColour(juce::Label::textColourId, iu(GrisClaro));
    }

    eqLowKnob.onValueChange = [this] {
        track.eqLow.gainDb = (float)eqLowKnob.getValue();
        track.eqLow.dirty.store(true);
    };
    eqMidKnob.onValueChange = [this] {
        track.eqMid.gainDb = (float)eqMidKnob.getValue();
        track.eqMid.dirty.store(true);
    };
    eqHighKnob.onValueChange = [this] {
        track.eqHigh.gainDb = (float)eqHighKnob.getValue();
        track.eqHigh.dirty.store(true);
    };

    volFader.setSliderStyle(juce::Slider::LinearVertical);
    volFader.setRange(0.0, 1.5, 0.01);
    volFader.setValue(track.volume, juce::dontSendNotification);
    volFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volFader.onValueChange = [this] { track.volume = (float)volFader.getValue(); };

    muteBtn.setClickingTogglesState(true);
    soloBtn.setClickingTogglesState(true);
    muteBtn.onClick = [this] { track.muted  = muteBtn.getToggleState(); };
    soloBtn.onClick = [this] { track.soloed = soloBtn.getToggleState(); };
}

void MixerPanel::ChannelStrip::resized()
{
    auto a = getLocalBounds().reduced(4);
    nameLabel.setBounds(a.removeFromTop(18));

    auto ms = a.removeFromTop(22);
    muteBtn.setBounds(ms.removeFromLeft(24).reduced(1));
    soloBtn.setBounds(ms.removeFromLeft(24).reduced(1));

    auto eqArea = a.removeFromTop(90);
    int kw = eqArea.getWidth() / 3;
    auto kRow = eqArea.removeFromTop(62);
    eqLowKnob .setBounds(kRow.removeFromLeft(kw).reduced(2));
    eqMidKnob .setBounds(kRow.removeFromLeft(kw).reduced(2));
    eqHighKnob.setBounds(kRow.reduced(2));
    eqLowLabel .setBounds(eqArea.removeFromLeft(kw));
    eqMidLabel .setBounds(eqArea.removeFromLeft(kw));
    eqHighLabel.setBounds(eqArea);

    volFader.setBounds(a);
}

void MixerPanel::ChannelStrip::paint(juce::Graphics& g)
{
    g.setColour(iu(NegroSuave));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    g.setColour(track.colour.withAlpha(0.5f));
    g.fillRoundedRectangle(getLocalBounds().toFloat().removeFromTop(5.0f), 3.0f);
    g.setColour(iu(GrisMedio));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 5.0f, 1.0f);
}

// ============================================================
//  MixerPanel
// ============================================================
MixerPanel::MixerPanel(AudioEngine& eng) : engine(eng)
{
    addAndMakeVisible(scrollView);
    scrollView.setViewedComponent(&stripContainer, false);
    scrollView.setScrollBarsShown(false, true);
    refresh();
}

void MixerPanel::refresh()
{
    strips.clear();
    stripContainer.removeAllChildren();
    for (auto& t : engine.getProject().tracks)
    {
        auto s = std::make_unique<ChannelStrip>(*t, engine);
        stripContainer.addAndMakeVisible(s.get());
        strips.push_back(std::move(s));
    }
    resized();
}

void MixerPanel::resized()
{
    scrollView.setBounds(getLocalBounds().reduced(8));
    const int sw = 90;
    stripContainer.setBounds(0, 0,
        sw * (int)strips.size(),
        scrollView.getHeight() - 16);
    for (int i = 0; i < (int)strips.size(); ++i)
        strips[i]->setBounds(i * sw, 0, sw - 4, stripContainer.getHeight());
}

void MixerPanel::paint(juce::Graphics& g)
{
    g.setColour(iu(Negro));
    g.fillAll();
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
    g.setColour(iu(GrisClaro));
    g.drawText("MIXER", 16, 6, 80, 18, juce::Justification::centredLeft);
}

// ============================================================
//  PluginsPanel
// ============================================================
PluginsPanel::PluginsPanel(AudioEngine& eng) : engine(eng)
{
    // addDefaultFormats() was deleted in JUCE 8 - add formats explicitly
    formatManager.addFormat(new juce::VST3PluginFormat());

    addAndMakeVisible(pluginList);
    addAndMakeVisible(scanBtn);
    addAndMakeVisible(loadBtn);
    addAndMakeVisible(statusLabel);

    pluginList.setModel(this);
    pluginList.setRowHeight(24);

    statusLabel.setText("No plugins loaded. Click 'Scan VST3 Folder' to find plugins.", juce::dontSendNotification);
    statusLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    statusLabel.setColour(juce::Label::textColourId, iu(GrisClaro));
    statusLabel.setJustificationType(juce::Justification::centredLeft);

    scanBtn.onClick = [this] { scanForPlugins(); };
    loadBtn.onClick = [this] { loadSelectedPlugin(); };
    loadBtn.setEnabled(false);
}

PluginsPanel::~PluginsPanel() = default;

int PluginsPanel::getNumRows() { return knownPlugins.getNumTypes(); }

void PluginsPanel::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected)
{
    g.setColour(selected ? iu(VerdeMosgo) : iu(NegroSuave));
    g.fillRect(0, 0, w, h);

    if (row < knownPlugins.getNumTypes())
    {
        auto types = knownPlugins.getTypes();
        if (row >= types.size()) return;
        auto desc = types[row];
        g.setColour(iu(Crema));
        g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
        g.drawText(desc.name + " - " + desc.manufacturerName,
                   6, 0, w - 12, h, juce::Justification::centredLeft);
    }
}

void PluginsPanel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    selectedRow = row;
    loadSelectedPlugin();
}

void PluginsPanel::resized()
{
    auto a = getLocalBounds().reduced(12);
    a.removeFromTop(30);

    auto btnRow = a.removeFromTop(32);
    scanBtn.setBounds(btnRow.removeFromLeft(160).reduced(2));
    loadBtn.setBounds(btnRow.removeFromLeft(140).reduced(2));
    a.removeFromTop(6);

    statusLabel.setBounds(a.removeFromTop(22));
    a.removeFromTop(4);

    pluginList.setBounds(a);
}

void PluginsPanel::paint(juce::Graphics& g)
{
    g.setColour(iu(Negro));
    g.fillAll();
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f).withStyle("Bold")));
    g.setColour(iu(GrisClaro));
    g.drawText("VST3 PLUGINS", 12, 8, 200, 18, juce::Justification::centredLeft);
    g.setColour(iu(GrisMedio));
    g.drawHorizontalLine(28, 0.0f, (float)getWidth());
}

void PluginsPanel::scanForPlugins()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select VST3 Folder",
        juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory),
        "");

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectDirectories,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto results = fc.getResults();
            if (results.isEmpty()) return;

            statusLabel.setText("Scanning for VST3 plugins...", juce::dontSendNotification);

            // PluginDirectoryScanner requires FileSearchPath, not a raw File
            juce::FileSearchPath searchPath(results[0].getFullPathName());
            juce::PluginDirectoryScanner scanner(knownPlugins,
                *formatManager.getFormat(0),
                searchPath, true, juce::File());

            juce::String currentPlugin;
            while (scanner.scanNextFile(true, currentPlugin)) {}

            statusLabel.setText(
                "Found " + juce::String(knownPlugins.getNumTypes()) + " plugin(s).",
                juce::dontSendNotification);
            pluginList.updateContent();
            loadBtn.setEnabled(knownPlugins.getNumTypes() > 0);
        });
}

void PluginsPanel::loadSelectedPlugin()
{
    int row = pluginList.getSelectedRow();
    if (row < 0 || row >= knownPlugins.getNumTypes())
    {
        statusLabel.setText("Select a plugin first.", juce::dontSendNotification);
        return;
    }

    auto types = knownPlugins.getTypes();
    if (row >= (int)types.size()) return;
    auto desc = types[row];
    statusLabel.setText(
        "Plugin \"" + desc.name + "\" selected. (Full hosting coming in next build.)",
        juce::dontSendNotification);
}

// ============================================================
//  ExportPanel
// ============================================================
ExportPanel::ExportPanel(AudioEngine& eng) : engine(eng)
{
    addAndMakeVisible(exportWavBtn);
    addAndMakeVisible(exportMp3Btn);
    addAndMakeVisible(bitrateBox);
    addAndMakeVisible(bitrateLabel);
    addAndMakeVisible(statusLabel);
    addAndMakeVisible(infoLabel);

    bitrateLabel.setText("MP3 Bitrate:", juce::dontSendNotification);
    bitrateLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    bitrateLabel.setColour(juce::Label::textColourId, iu(GrisClaro));

    bitrateBox.addItem("128 kbps", 128);
    bitrateBox.addItem("192 kbps", 192);
    bitrateBox.addItem("320 kbps", 320);
    bitrateBox.setSelectedId(192);

    statusLabel.setColour(juce::Label::textColourId, iu(VerdeClaro));
    statusLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));

    infoLabel.setText(
        "MP3 export requires LAME to be linked.\n"
        "See AudioEngine.cpp for instructions on enabling it.\n"
        "WAV export is always available.",
        juce::dontSendNotification);
    infoLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    infoLabel.setColour(juce::Label::textColourId, iu(GrisClaro));
    infoLabel.setJustificationType(juce::Justification::topLeft);

    exportWavBtn.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Export WAV",
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                .getChildFile("export.wav"),
            "*.wav");

        chooser->launchAsync(
            juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f == juce::File()) return;
                juce::File out = f.withFileExtension("wav");
                if (engine.exportToWav(out))
                    statusLabel.setText("Exported: " + out.getFileName(), juce::dontSendNotification);
                else
                    statusLabel.setText("Export failed.", juce::dontSendNotification);
            });
    };

    exportMp3Btn.onClick = [this]
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "MP3 Export",
            "MP3 export is not yet enabled in this build.\n\n"
            "To enable it, link LAME and see the comment\n"
            "in AudioEngine.cpp > exportToMp3().",
            "OK");
    };
}

void ExportPanel::resized()
{
    auto a = getLocalBounds().reduced(20);
    a.removeFromTop(40);

    exportWavBtn.setBounds(a.removeFromTop(36).removeFromLeft(180).reduced(2));
    a.removeFromTop(10);

    auto mp3Row = a.removeFromTop(36);
    exportMp3Btn.setBounds(mp3Row.removeFromLeft(200).reduced(2));
    mp3Row.removeFromLeft(16);
    bitrateLabel.setBounds(mp3Row.removeFromLeft(80).withSizeKeepingCentre(80, 20));
    bitrateBox.setBounds(mp3Row.removeFromLeft(120).withSizeKeepingCentre(120, 28));

    a.removeFromTop(12);
    statusLabel.setBounds(a.removeFromTop(22));
    a.removeFromTop(12);
    infoLabel.setBounds(a.removeFromTop(80));
}

void ExportPanel::paint(juce::Graphics& g)
{
    g.setColour(iu(Negro));
    g.fillAll();
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f).withStyle("Bold")));
    g.setColour(iu(GrisClaro));
    g.drawText("EXPORT", 20, 12, 160, 20, juce::Justification::centredLeft);
}

// ============================================================
//  SettingsPanel
// ============================================================
SettingsPanel::SettingsPanel(AudioEngine& eng)
    : engine(eng),
      deviceSelector(eng.getDeviceManager(), 0, 2, 0, 2,
                     false, false, false, false)
{
    addAndMakeVisible(deviceSelector);
    addAndMakeVisible(bufferSizeBox);
    addAndMakeVisible(bufferLabel);

    bufferLabel.setText("Buffer Size:", juce::dontSendNotification);
    bufferLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    bufferLabel.setColour(juce::Label::textColourId, iu(GrisClaro));

    for (int s : { 64, 128, 256, 512, 1024, 2048 })
        bufferSizeBox.addItem(juce::String(s) + " samples",
                              bufferSizeBox.getNumItems() + 1);
    bufferSizeBox.setSelectedItemIndex(3);
    bufferSizeBox.onChange = [this] {
        int sizes[] = { 64, 128, 256, 512, 1024, 2048 };
        int idx = bufferSizeBox.getSelectedItemIndex();
        if (idx >= 0 && idx < 6) engine.setBufferSize(sizes[idx]);
    };
}

void SettingsPanel::resized()
{
    auto a = getLocalBounds().reduced(20);
    a.removeFromTop(40);
    deviceSelector.setBounds(a.removeFromTop(300));
    a.removeFromTop(12);
    auto row = a.removeFromTop(30);
    bufferLabel.setBounds(row.removeFromLeft(100));
    bufferSizeBox.setBounds(row.removeFromLeft(160));
}

void SettingsPanel::paint(juce::Graphics& g)
{
    g.setColour(iu(Negro));
    g.fillAll();
    g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f).withStyle("Bold")));
    g.setColour(iu(GrisClaro));
    g.drawText("AUDIO SETTINGS", 20, 12, 200, 20, juce::Justification::centredLeft);
}

// ============================================================
//  MainComponent
// ============================================================
MainComponent::MainComponent()
{
    juce::LookAndFeel::setDefaultLookAndFeel(&theme);
    setSize(1280, 820);

    splash = std::make_unique<SplashScreenComponent>([this] { showDAW(); });
    addAndMakeVisible(*splash);
    splash->setBounds(getLocalBounds());

    engine.initialise();
    engine.addChangeListener(this);

    engine.getProject().addTrack("Audio");
    engine.getProject().addTrack("Audio");

    addKeyListener(this);
    setWantsKeyboardFocus(true);
}

MainComponent::~MainComponent()
{
    engine.removeChangeListener(this);
    removeKeyListener(this);
    engine.shutdown();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void MainComponent::showDAW()
{
    if (splash) { removeChildComponent(splash.get()); splash.reset(); }

    auto* tb  = new TransportBar(engine);
    auto* tp  = new TrackPanel(engine);
    auto* mp  = new MixerPanel(engine);
    auto* tup = new TunerPanel(engine);
    auto* pp  = new PluginsPanel(engine);
    auto* ep  = new ExportPanel(engine);
    auto* sp  = new SettingsPanel(engine);

    transport     = tb;
    trackPanel    = tp;
    mixerPanel    = mp;
    tunerPanel    = tup;
    pluginsPanel  = pp;
    exportPanel   = ep;
    settingsPanel = sp;

    tabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    tabs->setTabBarDepth(32);
    tabs->addTab("ARRANGE",  iu(Negro), tp,  true);
    tabs->addTab("MIXER",    iu(Negro), mp,  true);
    tabs->addTab("TUNER",    iu(Negro), tup, true);
    tabs->addTab("PLUGINS",  iu(Negro), pp,  true);
    tabs->addTab("EXPORT",   iu(Negro), ep,  true);
    tabs->addTab("SETTINGS", iu(Negro), sp,  true);

    // Menu bar
    menuBar = std::make_unique<juce::MenuBarComponent>(this);
    addAndMakeVisible(*menuBar);

    addAndMakeVisible(*transport);
    addAndMakeVisible(*tabs);
    resized();
    repaint();
}

void MainComponent::resized()
{
    if (splash) { splash->setBounds(getLocalBounds()); return; }
    auto area = getLocalBounds();

    const int menuH      = 22;
    const int transportH = 46;

    if (menuBar)
        menuBar->setBounds(area.removeFromTop(menuH));

    if (tabs)
        tabs->setBounds(area.removeFromTop(area.getHeight() - transportH));

    if (transport)
        transport->setBounds(area);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.setColour(iu(Negro));
    g.fillAll();
}

// ============================================================
//  Key commands
// ============================================================
bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // Space: play/pause
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        if (transport)
        {
            auto state = engine.getTransportState();
            if (state == AudioEngine::TransportState::Recording)
                transport->triggerStop();
            else
                transport->triggerPlay();
        }
        return true;
    }

    // Ctrl+Z: undo last recording
    if (key.getModifiers().isCommandDown() &&
        (key.getKeyCode() == 'Z' || key.getKeyCode() == 'z'))
    {
        engine.undoLastRecording();
        return true;
    }

    // Ctrl+S: save project
    if (key.getModifiers().isCommandDown() &&
        (key.getKeyCode() == 'S' || key.getKeyCode() == 's'))
    {
        saveProject();
        return true;
    }

    // Ctrl+O: open project
    if (key.getModifiers().isCommandDown() &&
        (key.getKeyCode() == 'O' || key.getKeyCode() == 'o'))
    {
        openProject();
        return true;
    }

    // Ctrl+X: cut clip
    if (key.getModifiers().isCommandDown() &&
        (key.getKeyCode() == 'X' || key.getKeyCode() == 'x'))
    {
        if (trackPanel) trackPanel->cutSelectedClip();
        return true;
    }

    // Ctrl+C: copy clip
    if (key.getModifiers().isCommandDown() &&
        (key.getKeyCode() == 'C' || key.getKeyCode() == 'c'))
    {
        if (trackPanel) trackPanel->copySelectedClip();
        return true;
    }

    // Ctrl+V: paste clip
    if (key.getModifiers().isCommandDown() &&
        (key.getKeyCode() == 'V' || key.getKeyCode() == 'v'))
    {
        if (trackPanel) trackPanel->pasteClip();
        return true;
    }

    // Delete / Backspace: delete clip
    if (key.getKeyCode() == juce::KeyPress::deleteKey ||
        key.getKeyCode() == juce::KeyPress::backspaceKey)
    {
        if (trackPanel) trackPanel->deleteSelectedClip();
        return true;
    }

    // R: record
    if (key.getTextCharacter() == 'r' || key.getTextCharacter() == 'R')
    {
        if (transport) transport->triggerRecord();
        return true;
    }

    return false;
}

// ============================================================
//  Change listener
// ============================================================
void MainComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    if (mixerPanel) mixerPanel->refresh();
    if (transport)  transport->refreshTrackList();
}

// ============================================================
//  Menu Bar
// ============================================================
juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "Edit" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int index, const juce::String&)
{
    juce::PopupMenu menu;
    if (index == 0) // File
    {
        menu.addItem(1, "New Project");
        menu.addSeparator();
        menu.addItem(2, "Open Project...  Ctrl+O");
        menu.addItem(3, "Save Project     Ctrl+S");
        menu.addItem(4, "Save Project As...");
        menu.addSeparator();
        menu.addItem(5, "Quit");
    }
    else if (index == 1) // Edit
    {
        menu.addItem(10, "Undo Last Recording  Ctrl+Z");
        menu.addSeparator();
        menu.addItem(11, "Cut Clip    Ctrl+X");
        menu.addItem(12, "Copy Clip   Ctrl+C");
        menu.addItem(13, "Paste Clip  Ctrl+V");
        menu.addItem(14, "Delete Clip  Del");
    }
    return menu;
}

void MainComponent::menuItemSelected(int itemId, int /*topLevelIndex*/)
{
    switch (itemId)
    {
        case 1: // New Project
            engine.stop();
            engine.getProject().tracks.clear();
            engine.getProject().addTrack("Audio");
            engine.getProject().addTrack("Audio");
            engine.sendChangeMessage();
            currentProjectFile = juce::File();
            break;
        case 2: openProject(); break;
        case 3: saveProject(); break;
        case 4: // Save As
        {
            auto chooser = std::make_shared<juce::FileChooser>(
                "Save Project As",
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("project.pennyr"),
                "*.pennyr");
            chooser->launchAsync(
                juce::FileBrowserComponent::saveMode |
                juce::FileBrowserComponent::canSelectFiles,
                [this, chooser](const juce::FileChooser& fc)
                {
                    auto f = fc.getResult();
                    if (f == juce::File()) return;
                    currentProjectFile = f.withFileExtension("pennyr");
                    saveProject();
                });
            break;
        }
        case 5: juce::JUCEApplication::getInstance()->systemRequestedQuit(); break;
        case 10: engine.undoLastRecording(); break;
        case 11: if (trackPanel) trackPanel->cutSelectedClip(); break;
        case 12: if (trackPanel) trackPanel->copySelectedClip(); break;
        case 13: if (trackPanel) trackPanel->pasteClip(); break;
        case 14: if (trackPanel) trackPanel->deleteSelectedClip(); break;
        default: break;
    }
}

// ============================================================
//  Project Save / Load
// ============================================================
void MainComponent::saveProject()
{
    if (currentProjectFile == juce::File())
    {
        // No file yet - trigger Save As
        auto chooser = std::make_shared<juce::FileChooser>(
            "Save Project",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                .getChildFile("project.pennyr"),
            "*.pennyr");

        chooser->launchAsync(
            juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f == juce::File()) return;
                currentProjectFile = f.withFileExtension("pennyr");
                saveProject();
            });
        return;
    }

    auto xml = engine.saveProjectToXml();
    if (xml)
    {
        if (!currentProjectFile.replaceWithText(xml->toString()))
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Save Error", "Could not write project file.");
    }
}

void MainComponent::openProject()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Open Project",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.pennyr");

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto results = fc.getResults();
            if (results.isEmpty()) return;

            juce::String xmlText = results[0].loadFileAsString();
            auto xml = juce::XmlDocument::parse(xmlText);
            if (!xml)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Open Error", "Could not parse project file.");
                return;
            }

            if (!engine.loadProjectFromXml(*xml))
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Open Error", "Invalid project file format.");
                return;
            }

            currentProjectFile = results[0];
            engine.sendChangeMessage();
        });
}