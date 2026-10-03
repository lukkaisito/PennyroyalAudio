#include "MainComponent.h"
#include <cmath>

using namespace InUtero;

namespace
{
    float easeOut(float x)   { x = juce::jlimit(0.0f, 1.0f, x); return 1.0f - (1.0f - x) * (1.0f - x) * (1.0f - x); }
    float easeInOut(float x) { x = juce::jlimit(0.0f, 1.0f, x); return x * x * (3.0f - 2.0f * x); }
    float phase(float t, float a, float b) { return juce::jlimit(0.0f, 1.0f, (t - a) / (b - a)); }

    // Pennyroyal logo mark: two rings + "P" (same as the app icon)
    void drawLogo(juce::Graphics& g, juce::Point<float> c0, float r, float sweep, float alpha)
    {
        juce::Path ring;
        ring.addCentredArc(c0.x, c0.y, r, r, 0.0f, 0.0f,
                           juce::MathConstants<float>::twoPi * juce::jlimit(0.001f, 1.0f, sweep), true);
        g.setColour(c(Acento).withAlpha(alpha));
        g.strokePath(ring, juce::PathStrokeType(juce::jmax(1.5f, r * 0.07f), juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
        const float r2 = r * 0.80f;
        g.setColour(c(Rojo).withAlpha(alpha * 0.9f));
        g.drawEllipse(c0.x - r2, c0.y - r2, r2 * 2.0f, r2 * 2.0f, juce::jmax(1.0f, r * 0.03f));
        g.setColour(c(Crema).withAlpha(alpha));
        g.setFont(PRFonts::title(r * 1.25f));
        g.drawText("P", juce::Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(c0.translated(0.0f, r * 0.05f)),
                   juce::Justification::centred);
    }
}

// ============================================================
//  SplashScreenComponent
// ============================================================
SplashScreenComponent::SplashScreenComponent(std::function<void()> onFinished)
    : finishedCallback(std::move(onFinished))
{
    setOpaque(true);
    startTimerHz(60);
}

void SplashScreenComponent::resized()
{
    // Pre-render film grain once
    const int w = juce::jmax(1, getWidth()), h = juce::jmax(1, getHeight());
    grain = juce::Image(juce::Image::ARGB, w, h, true);
    juce::Image::BitmapData bd(grain, juce::Image::BitmapData::writeOnly);
    juce::Random rng(1993);
    for (int y = 0; y < h; y += 2)
        for (int x = 0; x < w; x += 2)
        {
            const auto v = (juce::uint8)rng.nextInt(256);
            if (v > 200)
                bd.setPixelColour(x, y, juce::Colour(v, v, (juce::uint8)(v * 0.9f), (juce::uint8)rng.nextInt(28)));
        }
}

void SplashScreenComponent::paint(juce::Graphics& g)
{
    const float w = (float)getWidth(), h = (float)getHeight();
    const float cx = w * 0.5f, cy = h * 0.44f;

    g.fillAll(c(Negro));

    // warm glow behind the logo
    juce::ColourGradient glow(c(Acento).withAlpha(0.10f * easeOut(t / 1.2f)), cx, cy,
                              juce::Colours::transparentBlack, cx, cy + h * 0.55f, true);
    g.setGradientFill(glow);
    g.fillAll();

    g.setOpacity(1.0f);
    g.drawImageAt(grain, 0, 0);

    // vignette
    juce::ColourGradient vig(juce::Colours::transparentBlack, cx, cy,
                             juce::Colour(0xDD000000), 0.0f, 0.0f, true);
    g.setGradientFill(vig);
    g.fillAll();

    // ---- logo mark ----
    const float logoA = easeOut(phase(t, 0.0f, 0.6f));
    drawLogo(g, { cx, cy - 92.0f }, 46.0f, easeOut(phase(t, 0.05f, 1.1f)), logoA);

    // ---- title ----
    const float titleA = easeOut(phase(t, 0.25f, 0.9f));
    const float rise   = (1.0f - titleA) * 14.0f;
    g.setColour(c(CremaClaro).withAlpha(titleA));
    g.setFont(juce::Font(juce::FontOptions(PRFonts::bebas()).withHeight(96.0f).withKerningFactor(0.06f)));
    g.drawText("PENNYROYAL", juce::Rectangle<float>(0.0f, cy - 40.0f + rise, w, 96.0f),
               juce::Justification::centred);

    // "AUDIO" typewriter stamp
    const float audioA = easeOut(phase(t, 0.6f, 1.1f));
    {
        juce::Graphics::ScopedSaveState ss(g);
        g.addTransform(juce::AffineTransform::rotation(-0.035f, cx, cy + 66.0f));
        g.setColour(c(Acento).withAlpha(audioA));
        g.setFont(juce::Font(juce::FontOptions(PRFonts::grunge()).withHeight(30.0f).withKerningFactor(0.35f)));
        g.drawText("AUDIO", juce::Rectangle<float>(0.0f, cy + 50.0f, w, 34.0f), juce::Justification::centred);
    }

    // tagline
    const float tagA = easeOut(phase(t, 0.9f, 1.4f));
    g.setColour(c(CremaOscuro).withAlpha(tagA * 0.75f));
    g.setFont(PRFonts::typewriter(16.0f));
    g.drawText(TR("record  /  mix  /  distort"), juce::Rectangle<float>(0.0f, cy + 96.0f, w, 22.0f),
               juce::Justification::centred);

    // ---- loading bar ----
    const float prog = easeInOut(phase(t, 0.3f, DURATION - 0.25f));
    const float barW = 280.0f, barY = h * 0.80f;
    auto barBg = juce::Rectangle<float>(barW, 2.0f).withCentre({ cx, barY });
    g.setColour(c(GrisOscuro));
    g.fillRect(barBg);
    g.setColour(c(Acento));
    g.fillRect(barBg.withWidth(barW * prog));

    const char* step = prog < 0.25f ? "STARTING AUDIO ENGINE"
                     : prog < 0.50f ? "LOADING TYPEFACES"
                     : prog < 0.80f ? "BUILDING MIXER"
                     :                "READY";
    g.setColour(c(GrisClaro).withAlpha(easeOut(phase(t, 0.3f, 0.7f))));
    g.setFont(PRFonts::num(11.0f));
    g.drawText(TR(step), barBg.translated(0.0f, -22.0f).withHeight(16.0f).expanded(80.0f, 0.0f),
               juce::Justification::centred);

    // ---- footer ----
    g.setColour(c(GrisClaro).withAlpha(0.8f));
    g.setFont(PRFonts::num(11.0f));
    g.drawText("v1.4.0  /  JUCE 8  /  C++17", 28, (int)h - 34, 300, 16, juce::Justification::centredLeft);
    g.drawText(TR("EET N24 SIMON DE IRIONDO  /  OPEN SOURCE  /  GPLv3"), (int)w - 428, (int)h - 34, 400, 16,
               juce::Justification::centredRight);
}

void SplashScreenComponent::timerCallback()
{
    // start the clock only once the window is really on screen
    if (startMs <= 0.0)
    {
        if (!isShowing()) return;
        startMs = juce::Time::getMillisecondCounterHiRes();
    }
    t = (float)((juce::Time::getMillisecondCounterHiRes() - startMs) / 1000.0);
    repaint();
    if (t >= DURATION) finish();
}

void SplashScreenComponent::finish()
{
    if (finished) return;
    finished = true;
    stopTimer();
    if (finishedCallback) finishedCallback();
}

// ============================================================
//  TopBar
// ============================================================
TopBar::TopBar(AudioEngine& eng) : engine(eng)
{
    for (auto* b : { &openBtn, &saveBtn, &undoBtn, &redoBtn, &helpBtn })
    {
        b->setFlat(true);
        addAndMakeVisible(b);
    }
    for (auto* b : { &stopBtn, &playBtn, &recBtn, &loopBtn, &metroBtn, &monBtn })
        addAndMakeVisible(b);

    playBtn.setOnColour(c(Medidor));
    recBtn.setOnColour(c(RojoClaro));

    openBtn.setTooltip(TR("Open project  (Ctrl+O)"));
    saveBtn.setTooltip(TR("Save project  (Ctrl+S)"));
    undoBtn.setTooltip(TR("Undo  (Ctrl+Z)"));
    redoBtn.setTooltip(TR("Redo  (Ctrl+Y)"));
    helpBtn.setTooltip(TR("Quick-start tips"));
    stopBtn.setTooltip(TR("Stop  (back to start)"));
    playBtn.setTooltip(TR("Play / Pause  (Space)"));
    recBtn .setTooltip(TR("Record on the armed track, or the selected one  (R)"));
    loopBtn.setTooltip("Loop  (L)");
    metroBtn.setTooltip(TR("Metronome"));
    monBtn .setTooltip(TR("Input monitor: hear your mic / guitar live"));

    openBtn.onClick = [this] { if (onOpen) onOpen(); };
    saveBtn.onClick = [this] { if (onSave) onSave(); };
    undoBtn.onClick = [this] { if (onUndo) onUndo(); };
    redoBtn.onClick = [this] { if (onRedo) onRedo(); };
    helpBtn.onClick = [this] { if (onHelp) onHelp(); };
    stopBtn.onClick = [this] { stopTransport(); };
    playBtn.onClick = [this] { togglePlay(); };
    recBtn .onClick = [this] { toggleRecord(); };
    loopBtn.onClick = [this] { toggleLoop(); };
    metroBtn.onClick = [this] { engine.setMetronomeEnabled(!engine.isMetronomeEnabled()); updateButtonStates(); };
    monBtn .onClick = [this] { engine.setInputMonitorEnabled(!engine.isInputMonitorEnabled()); updateButtonStates(); };

    bpmSlider.setSliderStyle(juce::Slider::LinearBarVertical);
    bpmSlider.setRange(40.0, 240.0, 1.0);
    bpmSlider.setValue(engine.getProject().bpm, juce::dontSendNotification);
    bpmSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    bpmSlider.setMouseDragSensitivity(300);
    bpmSlider.setTooltip(TR("Tempo: drag up / down"));
    bpmSlider.onValueChange = [this] { engine.getProject().bpm = bpmSlider.getValue(); };
    addAndMakeVisible(bpmSlider);


    masterVol.setSliderStyle(juce::Slider::LinearHorizontal);
    masterVol.setRange(0.0, 1.5, 0.01);
    masterVol.setValue(engine.getMasterVolume(), juce::dontSendNotification);
    masterVol.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterVol.setDoubleClickReturnValue(true, 0.85);
    masterVol.setTooltip(TR("Master volume"));
    masterVol.onValueChange = [this] { engine.setMasterVolume((float)masterVol.getValue()); };
    addAndMakeVisible(masterVol);

    UI::noFocus({ &bpmSlider, &masterVol });
    refreshTrackList();
    updateButtonStates();
    startTimerHz(30);
}

void TopBar::refreshTrackList()
{
    lastTarget = -2;   // force the "records on" box to redraw
    repaint(recArea.expanded(4));
}

void TopBar::resized()
{
    auto a = getLocalBounds().reduced(14, 0);
    const int cy = getHeight() / 2;

    logoArea = a.removeFromLeft(172);

    auto place = [&](juce::Component& comp, int w, int h, int gapAfter)
    {
        comp.setBounds(a.removeFromLeft(w).withSizeKeepingCentre(w, h));
        a.removeFromLeft(gapAfter);
    };

    place(openBtn, 32, 32, 2);
    place(saveBtn, 32, 32, 10);
    place(undoBtn, 32, 32, 2);
    place(redoBtn, 32, 32, 2);
    place(helpBtn, 32, 32, 22);

    place(stopBtn, 40, 40, 5);
    place(playBtn, 40, 40, 5);
    place(recBtn,  40, 40, 14);
    place(loopBtn, 34, 34, 5);
    place(metroBtn,34, 34, 5);
    place(monBtn,  34, 34, 20);

    lcdArea = a.removeFromLeft(196).withSizeKeepingCentre(196, 46);
    a.removeFromLeft(18);

    auto bpmCol = a.removeFromLeft(64);
    bpmCaption = bpmCol.withHeight(14).withY(cy - 23);
    bpmSlider.setBounds(bpmCol.withHeight(26).withY(cy - 7));
    a.removeFromLeft(14);

    auto recCol = a.removeFromLeft(150);
    recCaption = recCol.withHeight(14).withY(cy - 23);
    recArea    = recCol.withHeight(26).withY(cy - 7);

    // right side: master
    auto right = getLocalBounds().reduced(14, 0).removeFromRight(190);
    meterArea  = right.removeFromRight(18).withSizeKeepingCentre(14, 40);
    right.removeFromRight(10);
    volCaption = right.withHeight(14).withY(cy - 23);
    masterVol.setBounds(right.withHeight(22).withY(cy - 6));
}

void TopBar::paint(juce::Graphics& g)
{
    juce::ColourGradient bg(c(NegroSuave), 0.0f, 0.0f, c(Panel), 0.0f, (float)getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();
    g.setColour(c(Linea));
    g.fillRect(0, getHeight() - 1, getWidth(), 1);

    // logo
    drawLogo(g, { (float)logoArea.getX() + 18.0f, (float)logoArea.getCentreY() }, 15.0f, 1.0f, 1.0f);
    g.setColour(c(CremaClaro));
    g.setFont(juce::Font(juce::FontOptions(PRFonts::bebas()).withHeight(25.0f).withKerningFactor(0.05f)));
    g.drawText("PENNYROYAL", logoArea.withTrimmedLeft(42).withHeight(26).withY(logoArea.getCentreY() - 15),
               juce::Justification::centredLeft);
    g.setColour(c(Acento));
    g.setFont(PRFonts::typewriter(11.0f));
    g.drawText("audio", logoArea.withTrimmedLeft(43).withHeight(12).withY(logoArea.getCentreY() + 8),
               juce::Justification::centredLeft);

    // separators
    g.setColour(c(Linea));
    for (auto* comp : { (juce::Component*)&helpBtn, (juce::Component*)&monBtn })
        g.fillRect(comp->getRight() + 10, 14, 1, getHeight() - 28);

    // LCD time display
    auto lcd = lcdArea.toFloat();
    g.setColour(c(Negro));
    g.fillRoundedRectangle(lcd, 6.0f);
    g.setColour(c(GrisOscuro));
    g.drawRoundedRectangle(lcd.reduced(0.5f), 6.0f, 1.0f);

    auto& proj = engine.getProject();
    const double pos   = engine.getPlayheadPositionSec();
    const double beats = pos / proj.secondsPerBeat();
    const int bar   = (int)(beats / proj.timeSignatureN) + 1;
    const int beat  = (int)std::fmod(beats, (double)proj.timeSignatureN) + 1;
    const int six   = (int)((beats - std::floor(beats)) * 4.0) + 1;

    const bool rec = engine.getTransportState() == AudioEngine::TransportState::Recording;
    g.setColour(rec ? c(RojoClaro).brighter(0.3f) : c(CremaClaro));
    g.setFont(PRFonts::numBold(23.0f));
    g.drawText(juce::String(bar).paddedLeft('0', 3) + "." + juce::String(beat) + "." + juce::String(six),
               lcd.reduced(12.0f, 3.0f).withTrimmedBottom(16.0f), juce::Justification::centredLeft);

    const int mins = (int)(pos / 60.0);
    const double secs = std::fmod(pos, 60.0);
    juce::String tstr = juce::String(mins).paddedLeft('0', 2) + ":" +
                        juce::String((int)secs).paddedLeft('0', 2) + "." +
                        juce::String((int)((secs - std::floor(secs)) * 1000.0)).paddedLeft('0', 3);
    g.setColour(c(CremaOscuro).withAlpha(0.75f));
    g.setFont(PRFonts::num(12.0f));
    g.drawText(tstr, lcd.reduced(12.0f, 5.0f).withTrimmedTop(25.0f), juce::Justification::centredLeft);
    g.drawText(juce::String(proj.timeSignatureN) + "/" + juce::String(proj.timeSignatureD),
               lcd.reduced(12.0f, 5.0f).withTrimmedTop(25.0f), juce::Justification::centredRight);
    if (rec)
    {
        g.setColour(c(RojoClaro));
        g.fillEllipse(lcd.getRight() - 18.0f, lcd.getY() + 9.0f, 7.0f, 7.0f);
    }

    UI::drawCaption(g, bpmCaption, "BPM", juce::Justification::centredLeft);
    UI::drawCaption(g, recCaption, TR("RECORDS ON"), juce::Justification::centredLeft);
    {
        auto r = recArea.toFloat();
        g.setColour(c(Negro));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(c(GrisMedio));
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
        const int ti = engine.getRecordTargetTrack();
        auto& trs = engine.getProject().tracks;
        if (ti >= 0 && ti < (int)trs.size())
        {
            auto& tr = *trs[(size_t)ti];
            const bool recNow = engine.getTransportState() == AudioEngine::TransportState::Recording;
            g.setColour(recNow ? c(RojoClaro) : tr.colour);
            g.fillEllipse(r.getX() + 9.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);
            g.setColour(c(CremaClaro));
            g.setFont(PRFonts::bodyBold(14.0f));
            g.drawText(tr.name + (tr.armed ? TR("  (armed)") : ""), r.withTrimmedLeft(24.0f).withTrimmedRight(6.0f),
                       juce::Justification::centredLeft, true);
        }
    }
    UI::drawCaption(g, volCaption, "MASTER", juce::Justification::centredLeft);

    // master stereo meter
    auto m = meterArea.toFloat();
    UI::drawMeter(g, m.withWidth(6.0f), levelL);
    UI::drawMeter(g, m.withTrimmedLeft(8.0f), levelR);
}

void TopBar::timerCallback()
{
    levelL = engine.getOutputLevelL();
    levelR = engine.getOutputLevelR();

    if (!bpmSlider.isMouseButtonDown() &&
        std::abs(bpmSlider.getValue() - engine.getProject().bpm) > 0.001)
        bpmSlider.setValue(engine.getProject().bpm, juce::dontSendNotification);
    if (!masterVol.isMouseButtonDown() &&
        std::abs(masterVol.getValue() - engine.getMasterVolume()) > 0.001)
        masterVol.setValue(engine.getMasterVolume(), juce::dontSendNotification);

    repaint(lcdArea);
    repaint(meterArea.expanded(2));
    const int target = engine.getRecordTargetTrack();
    if (target != lastTarget || engine.getTransportState() == AudioEngine::TransportState::Recording)
    {
        lastTarget = target;
        repaint(recArea.expanded(2));
    }
    updateButtonStates();
}

void TopBar::togglePlay()
{
    switch (engine.getTransportState())
    {
        case AudioEngine::TransportState::Playing:   engine.pause(); break;
        case AudioEngine::TransportState::Stopped:   engine.play();  break;
        case AudioEngine::TransportState::Recording: engine.stopRecording(); break;
    }
    updateButtonStates();
}

void TopBar::stopTransport()
{
    engine.stop();
    updateButtonStates();
}

void TopBar::toggleRecord()
{
    if (engine.getTransportState() == AudioEngine::TransportState::Recording)
    {
        engine.stopRecording();
    }
    else
    {
        const int idx = engine.getRecordTargetTrack();   // armed track, or the selected one
        if (idx < 0) return;
        engine.startRecording(idx);
    }
    updateButtonStates();
}

void TopBar::toggleLoop()
{
    engine.setLoopEnabled(!engine.isLoopEnabled());
    updateButtonStates();
}

void TopBar::updateButtonStates()
{
    const auto st = engine.getTransportState();
    const bool playing = st == AudioEngine::TransportState::Playing;
    const bool rec     = st == AudioEngine::TransportState::Recording;

    playBtn.setIcon(playing ? IconButton::Icon::Pause : IconButton::Icon::Play);
    playBtn.setToggleState(playing, juce::dontSendNotification);
    recBtn .setToggleState(rec, juce::dontSendNotification);
    loopBtn.setToggleState(engine.isLoopEnabled(), juce::dontSendNotification);
    metroBtn.setToggleState(engine.isMetronomeEnabled(), juce::dontSendNotification);
    monBtn .setToggleState(engine.isInputMonitorEnabled(), juce::dontSendNotification);
    undoBtn.setEnabled(engine.canUndo());
    redoBtn.setEnabled(engine.canRedo());
}

// ============================================================
//  QuickStartTip
// ============================================================
namespace
{
    const char* tipLines[] = {
        "Drop audio files on a track, or double-click a lane",
        "Press R to record on the selected (or armed) track",
        "Drag clip edges to trim, top corners to fade",
        "S split   /   Ctrl+D duplicate   /   Ctrl+Z undo",
    };
}

QuickStartTip::QuickStartTip()
{
    dontShow.setButtonText(TR("Don't show this again"));
    gotIt.setButtonText(TR("GOT IT"));
    closeBtn.setFlat(true);
    closeBtn.setTooltip(TR("Close"));
    closeBtn.onClick = [this] { if (onClose) onClose(dontShow.getToggleState()); };
    addAndMakeVisible(closeBtn);

    dontShow.setColour(juce::ToggleButton::textColourId, c(GrisClaro));
    addAndMakeVisible(dontShow);

    gotIt.setColour(juce::TextButton::buttonColourId, c(Acento));
    gotIt.setColour(juce::TextButton::textColourOffId, c(Negro));
    gotIt.onClick = [this] { if (onClose) onClose(dontShow.getToggleState()); };
    addAndMakeVisible(gotIt);

    UI::noFocus({ &closeBtn, &dontShow, &gotIt });
}

void QuickStartTip::resized()
{
    auto card = getLocalBounds().reduced(SHADOW);
    closeBtn.setBounds(card.getRight() - 34, card.getY() + 12, 24, 24);
    auto bottom = card.reduced(20, 16).removeFromBottom(30);
    gotIt.setBounds(bottom.removeFromRight(96));
    dontShow.setBounds(bottom.withTrimmedLeft(-4));
}

void QuickStartTip::paint(juce::Graphics& g)
{
    auto card = getLocalBounds().reduced(SHADOW);
    juce::DropShadow(juce::Colours::black.withAlpha(0.6f), SHADOW, { 0, 4 }).drawForRectangle(g, card);

    auto b = card.toFloat();
    g.setColour(c(NegroSuave));
    g.fillRoundedRectangle(b, 10.0f);
    g.setColour(c(GrisMedio));
    g.drawRoundedRectangle(b.reduced(0.5f), 10.0f, 1.0f);
    g.setColour(c(Acento));
    g.fillRoundedRectangle(b.withHeight(3.0f).reduced(12.0f, 0.0f), 1.5f);

    auto in = card.reduced(20, 16);
    g.setColour(c(CremaClaro));
    g.setFont(PRFonts::title(27.0f));
    g.drawText(TR("QUICK START"), in.removeFromTop(30), juce::Justification::centredLeft);
    in.removeFromTop(8);

    g.setFont(PRFonts::body(15.0f));
    for (auto* line : tipLines)
    {
        auto row = in.removeFromTop(26);
        g.setColour(c(Acento));
        g.fillRect(juce::Rectangle<float>(5.0f, 5.0f).withCentre({ (float)row.getX() + 3.0f, (float)row.getCentreY() }));
        g.setColour(c(Crema));
        g.drawFittedText(TR(line), row.withTrimmedLeft(16), juce::Justification::centredLeft, 1, 0.85f);
    }
}

// ============================================================
//  TimelineRuler
// ============================================================
TimelineRuler::TimelineRuler(AudioEngine& e, TrackPanel& p) : engine(e), panel(p) {}

void TimelineRuler::paint(juce::Graphics& g)
{
    const int w = getWidth(), h = getHeight();
    const float half = (float)h * 0.5f;
    auto& proj = engine.getProject();

    g.fillAll(c(Panel));
    g.setColour(c(Negro).withAlpha(0.5f));
    g.fillRect(0.0f, 0.0f, (float)w, half);

    // loop strip
    {
        const float x1 = (float)panel.secToX(proj.loopStart, w);
        const float x2 = (float)panel.secToX(proj.loopEnd, w);
        auto lr = juce::Rectangle<float>(x1, 2.0f, x2 - x1, half - 4.0f);
        const auto col = proj.loopEnabled ? c(Acento) : c(GrisMedio);
        g.setColour(col.withAlpha(proj.loopEnabled ? 0.35f : 0.35f));
        g.fillRoundedRectangle(lr, 3.0f);
        g.setColour(col);
        g.fillRoundedRectangle(lr.withWidth(4.0f), 2.0f);
        g.fillRoundedRectangle(lr.withLeft(lr.getRight() - 4.0f), 2.0f);
    }

    // bars / beats
    const double spb   = proj.secondsPerBeat();
    const double bar   = proj.secondsPerBar();
    const double pxBar = bar / (panel.viewEnd - panel.viewStart) * w;
    int labelEvery = 1;
    while (pxBar * labelEvery < 44.0) labelEvery *= 2;

    const double step = panel.gridStep(w) < spb ? spb : panel.gridStep(w);
    const double first = std::floor(panel.viewStart / step) * step;
    for (double t = first; t <= panel.viewEnd + step; t += step)
    {
        const float x = (float)panel.secToX(t, w);
        const double barIdx = t / bar;
        const bool isBar = std::abs(barIdx - std::round(barIdx)) < 1e-6;
        if (isBar)
        {
            const int barNum = (int)std::round(barIdx) + 1;
            g.setColour(c(GrisClaro));
            g.fillRect(x, half, 1.0f, half);
            if ((barNum - 1) % labelEvery == 0)
            {
                g.setColour(c(CremaOscuro));
                g.setFont(PRFonts::num(11.0f));
                g.drawText(juce::String(barNum), (int)x + 4, (int)half, 40, (int)half,
                           juce::Justification::centredLeft);
            }
        }
        else if (pxBar > 60.0)
        {
            g.setColour(c(GrisMedio));
            g.fillRect(x, (float)h - 6.0f, 1.0f, 6.0f);
        }
    }

    g.setColour(c(Linea));
    g.fillRect(0, h - 1, w, 1);

    // playhead
    const float px = (float)panel.secToX(engine.getPlayheadPositionSec(), w);
    if (px >= -6.0f && px <= (float)w + 6.0f)
    {
        juce::Path tri;
        tri.addTriangle(px - 6.0f, half, px + 6.0f, half, px, half + 8.0f);
        const bool rec = engine.getTransportState() == AudioEngine::TransportState::Recording;
        g.setColour(rec ? c(RojoClaro) : c(CremaClaro));
        g.fillPath(tri);
        g.fillRect(px - 0.5f, half, 1.0f, half);
    }
}

void TimelineRuler::mouseMove(const juce::MouseEvent& e)
{
    auto& proj = engine.getProject();
    if (inLoopStrip(e.y))
    {
        const double x1 = panel.secToX(proj.loopStart, getWidth());
        const double x2 = panel.secToX(proj.loopEnd, getWidth());
        if (std::abs(e.x - x1) < 6 || std::abs(e.x - x2) < 6)
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        else if (e.x > x1 && e.x < x2)
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        else
            setMouseCursor(juce::MouseCursor::IBeamCursor);
    }
    else
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void TimelineRuler::mouseDown(const juce::MouseEvent& e)
{
    auto& proj = engine.getProject();
    const double sec = panel.xToSec(e.x, getWidth());

    if (inLoopStrip(e.y))
    {
        const double x1 = panel.secToX(proj.loopStart, getWidth());
        const double x2 = panel.secToX(proj.loopEnd, getWidth());
        origLoopStart = proj.loopStart;
        origLoopEnd   = proj.loopEnd;
        dragAnchor    = sec;
        if      (std::abs(e.x - x1) < 6) dragMode = DragMode::LoopStart;
        else if (std::abs(e.x - x2) < 6) dragMode = DragMode::LoopEnd;
        else if (e.x > x1 && e.x < x2)   dragMode = DragMode::LoopMove;
        else
        {
            dragMode   = DragMode::LoopCreate;
            dragAnchor = panel.snap(sec, getWidth(), !e.mods.isAltDown());
        }
        return;
    }

    dragMode = DragMode::Seek;
    engine.setPlayheadPositionSec(std::max(0.0, sec));
    repaint();
}

void TimelineRuler::mouseDrag(const juce::MouseEvent& e)
{
    auto& proj = engine.getProject();
    const double sec  = std::max(0.0, panel.xToSec(e.x, getWidth()));
    const bool   snap = !e.mods.isAltDown();
    const double s    = panel.snap(sec, getWidth(), snap);

    switch (dragMode)
    {
        case DragMode::Seek:
            engine.setPlayheadPositionSec(sec);
            break;
        case DragMode::LoopStart:
            engine.setLoopPoints(std::min(s, proj.loopEnd - 0.1), proj.loopEnd);
            break;
        case DragMode::LoopEnd:
            engine.setLoopPoints(proj.loopStart, std::max(s, proj.loopStart + 0.1));
            break;
        case DragMode::LoopMove:
        {
            const double len = origLoopEnd - origLoopStart;
            double ns = panel.snap(origLoopStart + (sec - dragAnchor), getWidth(), snap);
            ns = std::max(0.0, ns);
            engine.setLoopPoints(ns, ns + len);
            break;
        }
        case DragMode::LoopCreate:
            if (std::abs(s - dragAnchor) > 0.05)
            {
                engine.setLoopPoints(std::min(s, dragAnchor), std::max(s, dragAnchor));
                engine.setLoopEnabled(true);
            }
            break;
        case DragMode::None: break;
    }
    if (auto* p = getParentComponent()) p->repaint();
}

void TimelineRuler::mouseUp(const juce::MouseEvent&) { dragMode = DragMode::None; }

void TimelineRuler::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const double sec = panel.xToSec(e.x, getWidth());
    if (e.mods.isCommandDown() || std::abs(w.deltaX) < std::abs(w.deltaY))
        panel.zoomAround(sec, w.deltaY > 0 ? 0.8 : 1.25);
    else
        panel.scrollBy(-w.deltaX * 0.5);
}

// ============================================================
//  TrackHeader
// ============================================================
TrackHeader::TrackHeader(Track& t, AudioEngine& eng, TrackPanel& p, int idx)
    : track(t), engine(eng), panel(p), trackIndex(idx)
{
    nameLabel.setText(track.name, juce::dontSendNotification);
    nameLabel.setFont(PRFonts::bodyBold(15.0f));
    nameLabel.setColour(juce::Label::textColourId, c(CremaClaro));
    nameLabel.setEditable(false, true, false);
    nameLabel.setInterceptsMouseClicks(false, false);
    nameLabel.onTextChange = [this]
    {
        auto n = nameLabel.getText().trim();
        if (n.isNotEmpty()) track.name = n;
        nameLabel.setText(track.name, juce::dontSendNotification);
        engine.sendChangeMessage();
    };
    addAndMakeVisible(nameLabel);

    auto setupToggle = [this](juce::TextButton& b, juce::Colour on, const juce::String& tip)
    {
        b.setClickingTogglesState(true);
        b.setColour(juce::TextButton::buttonColourId,   c(NegroSuave));
        b.setColour(juce::TextButton::buttonOnColourId, on);
        b.setColour(juce::TextButton::textColourOffId,  c(GrisClaro));
        b.setColour(juce::TextButton::textColourOnId,   c(Negro));
        b.setTooltip(tip);
        addAndMakeVisible(b);
    };
    setupToggle(muteBtn, c(0xFFD1B45A), TR("Mute"));
    setupToggle(soloBtn, c(0xFF7FA4C9), TR("Solo"));
    setupToggle(armBtn,  c(RojoClaro),  TR("Arm: record on this track"));

    muteBtn.onClick = [this] { track.muted  = muteBtn.getToggleState(); panel.repaint(); };
    soloBtn.onClick = [this] { track.soloed = soloBtn.getToggleState(); panel.repaint(); };
    armBtn.onClick  = [this]
    {
        for (auto& tr : engine.getProject().tracks) tr->armed = false;
        track.armed = armBtn.getToggleState();
        panel.selectTrack(trackIndex);
        engine.sendChangeMessage();
    };

    volSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volSlider.setRange(0.0, 1.5, 0.01);
    volSlider.setValue(track.volume, juce::dontSendNotification);
    volSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volSlider.setDoubleClickReturnValue(true, 1.0);
    volSlider.setColour(juce::Slider::trackColourId, track.colour);
    volSlider.onValueChange = [this] { track.volume = (float)volSlider.getValue(); };
    volSlider.setTooltip(TR("Volume (double-click: reset)"));
    addAndMakeVisible(volSlider);

    panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob.setRange(-1.0, 1.0, 0.01);
    panKnob.setValue(track.pan, juce::dontSendNotification);
    panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panKnob.setDoubleClickReturnValue(true, 0.0);
    panKnob.setColour(juce::Slider::rotarySliderFillColourId, track.colour);
    panKnob.onValueChange = [this] { track.pan = (float)panKnob.getValue(); };
    panKnob.setTooltip(TR("Pan (double-click: centre)"));
    addAndMakeVisible(panKnob);

    UI::noFocus({ &muteBtn, &soloBtn, &armBtn, &volSlider, &panKnob });
    refreshFromModel();
}

void TrackHeader::refreshFromModel()
{
    muteBtn.setToggleState(track.muted,  juce::dontSendNotification);
    soloBtn.setToggleState(track.soloed, juce::dontSendNotification);
    armBtn .setToggleState(track.armed,  juce::dontSendNotification);
    if (!nameLabel.isBeingEdited())
        nameLabel.setText(track.name, juce::dontSendNotification);
    repaint();
}

void TrackHeader::resized()
{
    auto a = getLocalBounds().withTrimmedLeft(12).withTrimmedRight(8).reduced(0, 8);
    meterArea = getLocalBounds().removeFromRight(9).reduced(2, 8);
    a.removeFromRight(6);

    auto top = a.removeFromTop(22);
    nameLabel.setBounds(top.withTrimmedLeft(-3).withTrimmedRight(16));
    a.removeFromTop(6);

    auto row = a.removeFromTop(22);
    panKnob.setBounds(row.removeFromRight(34).withSizeKeepingCentre(34, 34).translated(0, 6));
    muteBtn.setBounds(row.removeFromLeft(26).reduced(1));
    row.removeFromLeft(3);
    soloBtn.setBounds(row.removeFromLeft(26).reduced(1));
    row.removeFromLeft(3);
    armBtn .setBounds(row.removeFromLeft(26).reduced(1));
    a.removeFromTop(6);

    volSlider.setBounds(a.removeFromTop(18).withTrimmedRight(40).withTrimmedLeft(-2));
}

void TrackHeader::paint(juce::Graphics& g)
{
    const bool sel = panel.selectedTrack == trackIndex;
    g.fillAll(sel ? c(Seleccion) : c(Panel));

    g.setColour(track.colour.withAlpha(track.muted ? 0.35f : 1.0f));
    g.fillRect(0, 0, sel ? 5 : 3, getHeight());

    g.setColour(c(Linea));
    g.fillRect(0, getHeight() - 1, getWidth(), 1);
    g.fillRect(getWidth() - 1, 0, 1, getHeight());

    if (engine.getRecordTargetTrack() == trackIndex)
    {
        // small "this track records / monitors" marker next to the name
        const bool rec = engine.getTransportState() == AudioEngine::TransportState::Recording;
        auto dot = juce::Rectangle<float>(8.0f, 8.0f).withCentre({ (float)meterArea.getX() - 12.0f, 19.0f });
        g.setColour(c(RojoClaro));
        if (rec || track.armed) g.fillEllipse(dot);
        else                    g.drawEllipse(dot, 1.5f);
    }

    auto m = meterArea.toFloat();
    UI::drawMeter(g, m.withWidth(2.5f), track.meterL.load());
    UI::drawMeter(g, m.withTrimmedLeft(3.0f), track.meterR.load());
}

void TrackHeader::mouseDown(const juce::MouseEvent& e)
{
    panel.selectTrack(trackIndex);
    if (!e.mods.isPopupMenu()) return;

    juce::PopupMenu menu;
    menu.addSectionHeader(track.name);
    menu.addItem(1, TR("Rename"));
    menu.addItem(2, TR("Import audio..."));
    menu.addSeparator();
    menu.addItem(3, TR("Delete track"));

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [safe = juce::Component::SafePointer<TrackHeader>(this)](int r)
        {
            if (safe == nullptr) return;
            auto* self = safe.getComponent();
            if (r == 1) self->startRename();
            if (r == 2) self->panel.importAudio(self->trackIndex, self->engine.getPlayheadPositionSec());
            if (r == 3)
            {
                const int idx = self->trackIndex;
                auto& eng = self->engine;
                juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon, TR("Delete track"),
                    TR("Delete \"%1\" and all its clips?").replace("%1", self->track.name),
                    TR("Delete"), TR("Cancel"), nullptr,
                    juce::ModalCallbackFunction::create([&eng, idx](int res)
                    {
                        if (res == 1) { eng.removeTrack(idx); eng.sendChangeMessage(); }
                    }));
            }
        });
}

void TrackHeader::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (nameLabel.getBounds().contains(e.getPosition()))
        startRename();
}

void TrackHeader::startRename()
{
    nameLabel.showEditor();
}

// ============================================================
//  ArrangeCanvas
// ============================================================
ArrangeCanvas::ArrangeCanvas(AudioEngine& e, TrackPanel& p) : engine(e), panel(p)
{
    setWantsKeyboardFocus(false);
}

int ArrangeCanvas::trackAtY(int y) const
{
    const int n = (int)engine.getProject().tracks.size();
    return juce::jlimit(0, juce::jmax(0, n - 1), y / TrackPanel::TRACK_H);
}

juce::Rectangle<float> ArrangeCanvas::clipRect(int ti, const AudioClip& clip) const
{
    const float x1 = (float)panel.secToX(clip.startTimeSec, getWidth());
    const float x2 = (float)panel.secToX(clip.endTimeSec(), getWidth());
    return { x1, (float)(ti * TrackPanel::TRACK_H) + 3.0f, std::max(3.0f, x2 - x1),
             (float)TrackPanel::TRACK_H - 6.0f };
}

ArrangeCanvas::Hit ArrangeCanvas::hitTest(juce::Point<int> p) const
{
    Hit h;
    auto& tracks = engine.getProject().tracks;
    const int ti = p.y / TrackPanel::TRACK_H;
    if (ti < 0 || ti >= (int)tracks.size()) return h;
    h.track = ti;

    auto& clips = tracks[(size_t)ti]->clips;
    for (int i = (int)clips.size() - 1; i >= 0; --i)
    {
        auto* clip = clips[(size_t)i].get();
        auto r = clipRect(ti, *clip);
        if (!r.contains(p.toFloat())) continue;

        h.clip = clip;
        const float x = (float)p.x, y = (float)p.y;
        const float edge = juce::jmin(7.0f, r.getWidth() * 0.25f);
        if (y < r.getY() + 16.0f && x < r.getX() + 12.0f)          h.zone = Zone::FadeIn;
        else if (y < r.getY() + 16.0f && x > r.getRight() - 12.0f) h.zone = Zone::FadeOut;
        else if (x < r.getX() + edge)                               h.zone = Zone::TrimStart;
        else if (x > r.getRight() - edge)                           h.zone = Zone::TrimEnd;
        else                                                        h.zone = Zone::Body;
        return h;
    }
    return h;
}

void ArrangeCanvas::paint(juce::Graphics& g)
{
    auto& proj   = engine.getProject();
    auto& tracks = proj.tracks;
    const int w  = getWidth();
    const int TH = TrackPanel::TRACK_H;

    g.fillAll(c(Negro));

    // lanes
    for (int ti = 0; ti < (int)tracks.size(); ++ti)
    {
        auto lane = juce::Rectangle<int>(0, ti * TH, w, TH);
        g.setColour(ti % 2 == 0 ? c(LaneA) : c(LaneB));
        g.fillRect(lane);
        if (panel.selectedTrack == ti)
        {
            g.setColour(tracks[(size_t)ti]->colour.withAlpha(0.05f));
            g.fillRect(lane);
        }
    }

    // grid
    const double bar  = proj.secondsPerBar();
    const double step = panel.gridStep(w);
    const int totalH  = juce::jmax(getHeight(), (int)tracks.size() * TH);
    for (double t = std::floor(panel.viewStart / step) * step; t <= panel.viewEnd + step; t += step)
    {
        const float x = (float)panel.secToX(t, w);
        const double bi = t / bar;
        const bool isBar = std::abs(bi - std::round(bi)) < 1e-6;
        g.setColour(isBar ? c(GridBar) : c(GridBeat));
        g.fillRect(x, 0.0f, 1.0f, (float)totalH);
    }
    g.setColour(c(Linea));
    for (int ti = 1; ti <= (int)tracks.size(); ++ti)
        g.fillRect(0, ti * TH - 1, w, 1);

    // loop region
    if (proj.loopEnabled)
    {
        const float x1 = (float)panel.secToX(proj.loopStart, w);
        const float x2 = (float)panel.secToX(proj.loopEnd, w);
        g.setColour(c(Acento).withAlpha(0.045f));
        g.fillRect(x1, 0.0f, x2 - x1, (float)totalH);
        g.setColour(c(Acento).withAlpha(0.35f));
        g.fillRect(x1, 0.0f, 1.0f, (float)totalH);
        g.fillRect(x2, 0.0f, 1.0f, (float)totalH);
    }

    // clips
    for (int ti = 0; ti < (int)tracks.size(); ++ti)
    {
        auto& tr = *tracks[(size_t)ti];
        for (auto& clip : tr.clips)
        {
            auto r = clipRect(ti, *clip);
            if (r.getRight() < 0.0f || r.getX() > (float)w) continue;
            drawClip(g, r, *clip, tr.colour, clip.get() == panel.selectedClip);
        }
    }

    // live recording region with the waveform growing as you play
    int recTrack = -1; double recStart = 0.0, recLen = 0.0;
    if (engine.getRecordingInfo(recTrack, recStart, recLen))
    {
        const float x1 = (float)panel.secToX(recStart, w);
        const float x2 = (float)panel.secToX(recStart + recLen, w);
        auto r = juce::Rectangle<float>(x1, (float)(recTrack * TH) + 3.0f, std::max(2.0f, x2 - x1), (float)TH - 6.0f);

        juce::Graphics::ScopedSaveState ss(g);
        juce::Path shape;
        shape.addRoundedRectangle(r, 4.0f);
        g.reduceClipRegion(shape);

        g.setColour(c(Rojo).withAlpha(0.30f));
        g.fillRect(r);
        auto head = r.withHeight(16.0f);
        g.setColour(c(RojoClaro));
        g.fillRect(head);
        g.setColour(c(CremaClaro));
        g.setFont(PRFonts::bodyBold(12.5f));
        g.drawText(TR("RECORDING"), head.reduced(7.0f, 0.0f), juce::Justification::centredLeft, true);

        const float* mins = nullptr;
        const float* maxs = nullptr;
        const int count = engine.getLivePeaks(mins, maxs);
        if (count > 0 && mins != nullptr && maxs != nullptr)
        {
            auto body = r.withTrimmedTop(16.0f).reduced(0.0f, 3.0f);
            const float mid = body.getCentreY();
            const float hh  = body.getHeight() * 0.5f;
            const double secPerPx    = (panel.viewEnd - panel.viewStart) / (double)w;
            const double peaksPerSec = engine.getDeviceSampleRate() / (double)WaveformPeaks::SAMPLES_PER_PEAK;

            g.setColour(c(CremaClaro).withAlpha(0.85f));
            const int px0 = (int)std::max(0.0f, x1);
            const int px1 = (int)std::min((float)w, x2);
            for (int px = px0; px < px1; ++px)
            {
                const double tIn = panel.xToSec(px, w) - recStart;
                int i0 = (int)(tIn * peaksPerSec);
                if (i0 >= count) break;
                int i1 = (int)((tIn + secPerPx) * peaksPerSec);
                i0 = juce::jlimit(0, count - 1, i0);
                i1 = juce::jlimit(i0 + 1, count, i1);
                float lo = 0.0f, hi = 0.0f;
                for (int k = i0; k < i1; ++k) { lo = std::min(lo, mins[k]); hi = std::max(hi, maxs[k]); }
                const float y0 = mid - juce::jmin(1.0f, hi) * hh;
                const float y1 = mid - juce::jmax(-1.0f, lo) * hh;
                g.fillRect((float)px, y0, 1.0f, std::max(1.0f, y1 - y0));
            }
        }
        g.setColour(c(RojoClaro));
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.5f);
    }

    if (dropHighlight)
    {
        g.setColour(c(Acento));
        g.drawRect(getLocalBounds(), 2);
    }

    // playhead
    const float px = (float)panel.secToX(engine.getPlayheadPositionSec(), w);
    if (px >= 0.0f && px <= (float)w)
    {
        const bool rec = engine.getTransportState() == AudioEngine::TransportState::Recording;
        g.setColour(rec ? c(RojoClaro) : c(CremaClaro).withAlpha(0.85f));
        g.fillRect(px - 0.5f, 0.0f, 1.0f, (float)totalH);
    }
}

void ArrangeCanvas::drawClip(juce::Graphics& g, juce::Rectangle<float> r, const AudioClip& clip,
                             juce::Colour col, bool selected)
{
    const int w = getWidth();
    const float alpha = clip.muted ? 0.35f : 1.0f;
    const float headH = juce::jmin(16.0f, r.getHeight() * 0.25f);

    juce::Graphics::ScopedSaveState ss(g);
    juce::Path shape;
    shape.addRoundedRectangle(r, 4.0f);
    g.reduceClipRegion(shape);

    // body + header
    g.setColour(col.withMultipliedSaturation(0.7f).darker(1.7f).withAlpha(alpha));
    g.fillRect(r);
    auto head = r.withHeight(headH);
    g.setColour(col.withAlpha(alpha * (selected ? 1.0f : 0.85f)));
    g.fillRect(head);

    // waveform from cached peaks
    auto body = r.withTrimmedTop(headH).reduced(0.0f, 3.0f);
    if (clip.peaks && !clip.peaks->maxs.empty())
    {
        const auto& pk  = *clip.peaks;
        const float mid = body.getCentreY();
        const float hh  = body.getHeight() * 0.5f;
        const double secPerPx = (panel.viewEnd - panel.viewStart) / (double)w;
        const double peaksPerSec = clip.sampleRate / (double)WaveformPeaks::SAMPLES_PER_PEAK;
        const int numPeaks = (int)pk.maxs.size();

        g.setColour(col.interpolatedWith(c(CremaClaro), 0.45f).withAlpha(alpha * 0.9f));
        const int px0 = (int)std::max(0.0f, r.getX());
        const int px1 = (int)std::min((float)w, r.getRight());
        for (int px = px0; px < px1; ++px)
        {
            const double tIn = panel.xToSec(px, w) - clip.startTimeSec;
            const double src = tIn + clip.offsetSec;
            int i0 = (int)(src * peaksPerSec);
            int i1 = (int)((src + secPerPx) * peaksPerSec);
            i0 = juce::jlimit(0, numPeaks - 1, i0);
            i1 = juce::jlimit(i0 + 1, numPeaks, i1);
            float lo = 0.0f, hi = 0.0f;
            for (int k = i0; k < i1; ++k) { lo = std::min(lo, pk.mins[(size_t)k]); hi = std::max(hi, pk.maxs[(size_t)k]); }
            const float gfac = clip.gain * clip.fadeGainAt(juce::jlimit(0.0, clip.lengthSec, tIn));
            const float y0 = mid - juce::jmin(1.0f, hi * gfac) * hh;
            const float y1 = mid - juce::jmax(-1.0f, lo * gfac) * hh;
            g.fillRect((float)px, y0, 1.0f, std::max(1.0f, y1 - y0));
        }
        g.setColour(col.withAlpha(0.25f * alpha));
        g.fillRect(body.getX(), mid, body.getWidth(), 1.0f);
    }

    // fades
    const float fiPx = (float)(clip.fadeInSec  / (panel.viewEnd - panel.viewStart) * w);
    const float foPx = (float)(clip.fadeOutSec / (panel.viewEnd - panel.viewStart) * w);
    auto fb = r.withTrimmedTop(headH);
    if (fiPx > 0.5f)
    {
        juce::Path p;
        p.startNewSubPath(fb.getX(), fb.getY());
        p.lineTo(fb.getX() + fiPx, fb.getY());
        p.quadraticTo(fb.getX() + fiPx * 0.3f, fb.getY() + fb.getHeight() * 0.3f, fb.getX(), fb.getBottom());
        p.closeSubPath();
        g.setColour(c(Negro).withAlpha(0.55f));
        g.fillPath(p);
        g.setColour(c(CremaClaro).withAlpha(0.6f));
        juce::Path line;
        line.startNewSubPath(fb.getX(), fb.getBottom());
        line.quadraticTo(fb.getX() + fiPx * 0.3f, fb.getY() + fb.getHeight() * 0.3f, fb.getX() + fiPx, fb.getY());
        g.strokePath(line, juce::PathStrokeType(1.2f));
    }
    if (foPx > 0.5f)
    {
        juce::Path p;
        p.startNewSubPath(fb.getRight(), fb.getY());
        p.lineTo(fb.getRight() - foPx, fb.getY());
        p.quadraticTo(fb.getRight() - foPx * 0.3f, fb.getY() + fb.getHeight() * 0.3f, fb.getRight(), fb.getBottom());
        p.closeSubPath();
        g.setColour(c(Negro).withAlpha(0.55f));
        g.fillPath(p);
        g.setColour(c(CremaClaro).withAlpha(0.6f));
        juce::Path line;
        line.startNewSubPath(fb.getRight(), fb.getBottom());
        line.quadraticTo(fb.getRight() - foPx * 0.3f, fb.getY() + fb.getHeight() * 0.3f, fb.getRight() - foPx, fb.getY());
        g.strokePath(line, juce::PathStrokeType(1.2f));
    }

    // name
    g.setColour(c(Negro).withAlpha(0.9f * alpha));
    g.setFont(PRFonts::bodyBold(12.5f));
    g.drawText(clip.name + (clip.muted ? TR("  [muted]") : ""),
               head.reduced(7.0f, 0.0f).withTrimmedLeft(fiPx > 0.0f ? 6.0f : 0.0f),
               juce::Justification::centredLeft, true);

    // fade handles
    if (selected)
    {
        g.setColour(c(CremaClaro));
        g.fillRect(juce::Rectangle<float>(6.0f, 6.0f).withCentre({ r.getX() + fiPx + 3.0f, r.getY() + headH + 3.0f }));
        g.fillRect(juce::Rectangle<float>(6.0f, 6.0f).withCentre({ r.getRight() - foPx - 3.0f, r.getY() + headH + 3.0f }));
    }

    // outline
    g.setColour(selected ? c(CremaClaro) : col.withAlpha(0.5f * alpha));
    g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, selected ? 1.6f : 1.0f);
}

void ArrangeCanvas::mouseMove(const juce::MouseEvent& e)
{
    switch (hitTest(e.getPosition()).zone)
    {
        case Zone::TrimStart:
        case Zone::TrimEnd:   setMouseCursor(juce::MouseCursor::LeftRightResizeCursor); break;
        case Zone::FadeIn:    setMouseCursor(juce::MouseCursor::TopLeftCornerResizeCursor); break;
        case Zone::FadeOut:   setMouseCursor(juce::MouseCursor::TopRightCornerResizeCursor); break;
        case Zone::Body:      setMouseCursor(juce::MouseCursor::DraggingHandCursor); break;
        case Zone::None:      setMouseCursor(juce::MouseCursor::NormalCursor); break;
    }
}

void ArrangeCanvas::mouseDown(const juce::MouseEvent& e)
{
    if (auto* top = findParentComponentOfClass<MainComponent>())
        top->grabKeyboardFocus();

    const auto hit = hitTest(e.getPosition());
    const int  ti  = hit.track >= 0 ? hit.track : trackAtY(e.y);
    const double sec = panel.xToSec(e.x, getWidth());

    if (e.mods.isPopupMenu())
    {
        if (hit.clip) { panel.selectClip(hit.track, hit.clip); showClipMenu(hit.track, hit.clip); }
        else          { panel.selectTrack(ti); showEmptyMenu(ti, sec); }
        return;
    }

    if (hit.clip)
    {
        panel.selectClip(hit.track, hit.clip);
        dragZone     = hit.zone;
        dragTrack    = hit.track;
        dragClip     = hit.clip;
        dragOrig     = *hit.clip;
        dragMouseSec = sec;
        dragChanged  = false;
        undoPushed   = false;
    }
    else
    {
        dragClip = nullptr;
        dragZone = Zone::None;
        panel.selectClip(ti, nullptr);
        engine.setPlayheadPositionSec(std::max(0.0, panel.snap(sec, getWidth(), !e.mods.isAltDown())));
    }
    repaint();
}

void ArrangeCanvas::mouseDrag(const juce::MouseEvent& e)
{
    if (dragClip == nullptr || dragZone == Zone::None) return;
    if (!dragChanged && e.getDistanceFromDragStart() < 3) return;

    if (!undoPushed)
    {
        // nothing has changed yet, so this snapshot is the "before" state
        engine.pushUndoState();
        undoPushed = true;
    }
    dragChanged = true;

    const int    w    = getWidth();
    const double sec  = panel.xToSec(e.x, w);
    const double dt   = sec - dragMouseSec;
    const bool   snap = !e.mods.isAltDown();
    const double minLen = 0.02;

    const juce::ScopedLock sl(engine.getLock());
    auto& c0 = *dragClip;

    switch (dragZone)
    {
        case Zone::Body:
        {
            c0.startTimeSec = std::max(0.0, panel.snap(dragOrig.startTimeSec + dt, w, snap));

            // move between tracks
            auto& tracks = engine.getProject().tracks;
            const int newTrack = trackAtY(e.y);
            if (newTrack != dragTrack && newTrack >= 0 && newTrack < (int)tracks.size())
            {
                auto& src = tracks[(size_t)dragTrack]->clips;
                const int idx = tracks[(size_t)dragTrack]->indexOf(dragClip);
                if (idx >= 0)
                {
                    auto moved = std::move(src[(size_t)idx]);
                    src.erase(src.begin() + idx);
                    tracks[(size_t)newTrack]->clips.push_back(std::move(moved));
                    dragTrack = newTrack;
                    panel.selectedTrack = newTrack;
                    engine.setSelectedTrack(newTrack);
                }
            }
            break;
        }
        case Zone::TrimStart:
        {
            const double minStart = dragOrig.startTimeSec - dragOrig.offsetSec;
            double ns = panel.snap(dragOrig.startTimeSec + dt, w, snap);
            ns = juce::jlimit(std::max(0.0, minStart), dragOrig.endTimeSec() - minLen, ns);
            const double delta = ns - dragOrig.startTimeSec;
            c0.startTimeSec = ns;
            c0.offsetSec    = dragOrig.offsetSec + delta;
            c0.lengthSec    = dragOrig.lengthSec - delta;
            c0.fadeInSec    = juce::jmin(c0.fadeInSec, c0.lengthSec * 0.5);
            c0.fadeOutSec   = juce::jmin(c0.fadeOutSec, c0.lengthSec - c0.fadeInSec);
            break;
        }
        case Zone::TrimEnd:
        {
            const double maxEnd = dragOrig.startTimeSec + (dragOrig.sourceLengthSec() - dragOrig.offsetSec);
            double ne = panel.snap(dragOrig.endTimeSec() + dt, w, snap);
            ne = juce::jlimit(dragOrig.startTimeSec + minLen, maxEnd, ne);
            c0.lengthSec  = ne - c0.startTimeSec;
            c0.fadeOutSec = juce::jmin(c0.fadeOutSec, c0.lengthSec * 0.5);
            c0.fadeInSec  = juce::jmin(c0.fadeInSec, c0.lengthSec - c0.fadeOutSec);
            break;
        }
        case Zone::FadeIn:
            c0.fadeInSec = juce::jlimit(0.0, c0.lengthSec - c0.fadeOutSec, sec - c0.startTimeSec);
            break;
        case Zone::FadeOut:
            c0.fadeOutSec = juce::jlimit(0.0, c0.lengthSec - c0.fadeInSec, c0.endTimeSec() - sec);
            break;
        case Zone::None: break;
    }
    repaint();
}

void ArrangeCanvas::mouseUp(const juce::MouseEvent&)
{
    dragClip = nullptr;
    dragZone = Zone::None;
    if (dragChanged) engine.sendChangeMessage();
    dragChanged = false;
}

void ArrangeCanvas::mouseDoubleClick(const juce::MouseEvent& e)
{
    const auto hit = hitTest(e.getPosition());
    if (hit.clip) { renameClip(hit.clip); return; }
    panel.importAudio(trackAtY(e.y), std::max(0.0, panel.snap(panel.xToSec(e.x, getWidth()), getWidth(), true)));
}

void ArrangeCanvas::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (e.mods.isCommandDown())
    {
        panel.zoomAround(panel.xToSec(e.x, getWidth()), w.deltaY > 0 ? 0.8 : 1.25);
        return;
    }
    if (e.mods.isShiftDown() || std::abs(w.deltaX) > std::abs(w.deltaY))
    {
        const float d = std::abs(w.deltaX) > std::abs(w.deltaY) ? w.deltaX : w.deltaY;
        panel.scrollBy(-d * 0.5);
        return;
    }
    juce::Component::mouseWheelMove(e, w);   // vertical scroll -> viewport
}

void ArrangeCanvas::renameClip(AudioClip* clip)
{
    auto* dlg = new juce::AlertWindow(TR("Rename clip"), {}, juce::AlertWindow::NoIcon);
    dlg->addTextEditor("name", clip->name);
    dlg->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dlg->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    dlg->enterModalState(true, juce::ModalCallbackFunction::create(
        [dlg, clip, safe = juce::Component::SafePointer<ArrangeCanvas>(this)](int res)
        {
            if (res == 1 && safe != nullptr)
            {
                auto n = dlg->getTextEditorContents("name").trim();
                auto& tracks = safe->engine.getProject().tracks;
                for (auto& t : tracks)
                    if (t->indexOf(clip) >= 0 && n.isNotEmpty())
                        clip->name = n;
                safe->repaint();
            }
        }), true);
}

void ArrangeCanvas::showClipMenu(int, AudioClip* clip)
{
    juce::PopupMenu m;
    auto add = [&m](int id, const juce::String& text, const juce::String& shortcut = {})
    {
        juce::PopupMenu::Item it(text);
        it.itemID = id;
        it.shortcutKeyDescription = shortcut;
        m.addItem(it);
    };
    m.addSectionHeader(clip->name);
    add(1, TR("Rename"));
    add(2, TR("Split at playhead"), "S");
    add(3, TR("Duplicate"), "Ctrl+D");
    add(4, clip->muted ? TR("Unmute clip") : TR("Mute clip"));
    add(5, TR("Remove fades"));
    m.addSeparator();
    add(6, TR("Cut"), "Ctrl+X");
    add(7, TR("Copy"), "Ctrl+C");
    add(8, TR("Delete"), TR("Del"));

    m.showMenuAsync(juce::PopupMenu::Options(),
        [clip, safe = juce::Component::SafePointer<ArrangeCanvas>(this)](int r)
        {
            if (safe == nullptr || r == 0) return;
            auto& p = safe->panel;
            auto& eng = safe->engine;
            switch (r)
            {
                case 1: safe->renameClip(clip); break;
                case 2: p.splitSelectedClipAtPlayhead(); break;
                case 3: p.duplicateSelectedClip(); break;
                case 4: { eng.pushUndoState(); const juce::ScopedLock sl(eng.getLock()); clip->muted = !clip->muted; break; }
                case 5: { eng.pushUndoState(); const juce::ScopedLock sl(eng.getLock()); clip->fadeInSec = clip->fadeOutSec = 0.0; break; }
                case 6: p.cutSelectedClip(); break;
                case 7: p.copySelectedClip(); break;
                case 8: p.deleteSelectedClip(); break;
                default: break;
            }
            safe->repaint();
        });
}

void ArrangeCanvas::showEmptyMenu(int ti, double sec)
{
    juce::PopupMenu m;
    m.addItem(1, TR("Import audio here..."));
    juce::PopupMenu::Item paste(TR("Paste at playhead"));
    paste.itemID = 2;
    paste.shortcutKeyDescription = "Ctrl+V";
    m.addItem(paste);
    m.addItem(3, TR("Set loop to this bar"));
    m.showMenuAsync(juce::PopupMenu::Options(),
        [ti, sec, safe = juce::Component::SafePointer<ArrangeCanvas>(this)](int r)
        {
            if (safe == nullptr) return;
            auto& p = safe->panel;
            auto& eng = safe->engine;
            if (r == 1) p.importAudio(ti, std::max(0.0, sec));
            if (r == 2) p.pasteClip();
            if (r == 3)
            {
                const double bar = eng.getProject().secondsPerBar();
                const double s = std::floor(sec / bar) * bar;
                eng.setLoopPoints(s, s + bar);
                eng.setLoopEnabled(true);
            }
            safe->repaint();
        });
}

bool ArrangeCanvas::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
        if (juce::File(f).hasFileExtension("wav;aif;aiff;mp3;flac;ogg"))
            return true;
    return false;
}

void ArrangeCanvas::filesDropped(const juce::StringArray& files, int x, int y)
{
    dropHighlight = false;
    int ti = y / TrackPanel::TRACK_H;
    if (ti >= (int)engine.getProject().tracks.size())
    {
        engine.addTrack("Audio");
        panel.refreshTracks();
        ti = (int)engine.getProject().tracks.size() - 1;
    }
    double sec = std::max(0.0, panel.snap(panel.xToSec(x, getWidth()), getWidth(), true));

    engine.pushUndoState();
    for (auto& f : files)
    {
        juce::File file(f);
        if (!file.hasFileExtension("wav;aif;aiff;mp3;flac;ogg")) continue;
        if (panel.importFile(file, ti, sec))
        {
            auto& clips = engine.getProject().tracks[(size_t)ti]->clips;
            sec = clips.back()->endTimeSec();
        }
    }
    engine.sendChangeMessage();
    repaint();
}

// ============================================================
//  TrackPanel
// ============================================================
TrackPanel::TrackPanel(AudioEngine& eng)
    : engine(eng), ruler(eng, *this), canvas(eng, *this)
{
    engine.addChangeListener(this);

    addAndMakeVisible(ruler);
    addAndMakeVisible(viewport);
    addAndMakeVisible(hScroll);
    viewport.setViewedComponent(&container, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(10);
    container.addAndMakeVisible(canvas);
    container.addAndMakeVisible(addTrackBtn);

    hScroll.addListener(this);
    hScroll.setAutoHide(false);

    addTrackBtn.setTooltip(TR("Add audio track"));
    addTrackBtn.onClick = [this]
    {
        engine.addTrack("Audio");
        refreshTracks();
        selectTrack((int)engine.getProject().tracks.size() - 1);
        engine.sendChangeMessage();
    };

    refreshTracks();
    startTimerHz(30);
}

TrackPanel::~TrackPanel()
{
    engine.removeChangeListener(this);
    hScroll.removeListener(this);
}

void TrackPanel::refreshTracks()
{
    for (auto& h : headers) container.removeChildComponent(h.get());
    headers.clear();

    auto& tracks = engine.getProject().tracks;
    for (int i = 0; i < (int)tracks.size(); ++i)
    {
        auto h = std::make_unique<TrackHeader>(*tracks[(size_t)i], engine, *this, i);
        container.addAndMakeVisible(h.get());
        headers.push_back(std::move(h));
    }
    lastTrackCount = tracks.size();
    selectedTrack  = juce::jlimit(0, juce::jmax(0, (int)tracks.size() - 1), selectedTrack);
    engine.setSelectedTrack(selectedTrack);

    // drop a dangling clip selection
    bool found = false;
    for (auto& t : tracks) if (t->indexOf(selectedClip) >= 0) found = true;
    if (!found) selectedClip = nullptr;

    resized();
    repaint();
}

void TrackPanel::resized()
{
    auto a = getLocalBounds();
    auto top = a.removeFromTop(RULER_H);
    ruler.setBounds(top.withTrimmedLeft(HEADER_W).withTrimmedRight(viewport.getScrollBarThickness()));
    auto bottom = a.removeFromBottom(12);
    hScroll.setBounds(bottom.withTrimmedLeft(HEADER_W).withTrimmedRight(viewport.getScrollBarThickness()));
    viewport.setBounds(a);

    const int n = (int)headers.size();
    const int contentW = viewport.getWidth() - viewport.getScrollBarThickness();
    const int contentH = juce::jmax(viewport.getHeight(), n * TRACK_H + 52);
    container.setSize(contentW, contentH);

    for (int i = 0; i < n; ++i)
        headers[(size_t)i]->setBounds(0, i * TRACK_H, HEADER_W, TRACK_H);
    addTrackBtn.setBounds(12, n * TRACK_H + 12, 30, 30);
    canvas.setBounds(HEADER_W, 0, contentW - HEADER_W, contentH);

    updateScrollBar();
}

void TrackPanel::paint(juce::Graphics& g)
{
    g.fillAll(c(Negro));
    auto corner = juce::Rectangle<int>(0, 0, HEADER_W, RULER_H);
    g.setColour(c(Panel));
    g.fillRect(corner);
    g.setColour(c(Linea));
    g.fillRect(corner.removeFromBottom(1));
    g.fillRect(HEADER_W - 1, 0, 1, RULER_H);

    g.setColour(c(GrisClaro));
    g.setFont(PRFonts::title(17.0f));
    g.drawText(TR("TRACKS  ") + juce::String((int)engine.getProject().tracks.size()),
               juce::Rectangle<int>(12, 0, HEADER_W - 20, RULER_H), juce::Justification::centredLeft);

    g.setColour(c(Panel));
    g.fillRect(0, getHeight() - 12, HEADER_W, 12);
}

void TrackPanel::timerCallback()
{
    auto& tracks = engine.getProject().tracks;
    if (tracks.size() != lastTrackCount) refreshTracks();

    // selection may have been changed from the mixer
    if (engine.getSelectedTrack() != selectedTrack)
    {
        selectedTrack = engine.getSelectedTrack();
        for (auto& h : headers) h->repaint();
        canvas.repaint();
    }

    const int stateNow = (int)engine.getTransportState();
    if (stateNow != lastTransportState)
    {
        lastTransportState = stateNow;
        for (auto& h : headers) h->repaint();
    }

    const bool running = engine.getTransportState() != AudioEngine::TransportState::Stopped;
    const double pos   = engine.getPlayheadPositionSec();
    const double range = viewEnd - viewStart;

    // follow the playhead while playing
    if (running && (pos > viewEnd - range * 0.03 || pos < viewStart))
    {
        viewStart = std::max(0.0, pos - range * 0.1);
        viewEnd   = viewStart + range;
        updateScrollBar();
    }

    ruler.repaint();
    if (running) canvas.repaint();
    for (auto& h : headers) h->repaint(h->getMeterArea().expanded(2));
}

void TrackPanel::changeListenerCallback(juce::ChangeBroadcaster*)
{
    if (engine.getProject().tracks.size() != lastTrackCount) { refreshTracks(); return; }

    bool found = selectedClip == nullptr;
    for (auto& t : engine.getProject().tracks) if (t->indexOf(selectedClip) >= 0) found = true;
    if (!found) selectedClip = nullptr;

    for (auto& h : headers) h->refreshFromModel();
    updateScrollBar();
    canvas.repaint();
}

void TrackPanel::zoomAround(double sec, double factor)
{
    const double oldRange = viewEnd - viewStart;
    const double range = juce::jlimit(0.5, 1800.0, oldRange * factor);
    const double p = (sec - viewStart) / oldRange;
    viewStart = std::max(0.0, sec - p * range);
    viewEnd   = viewStart + range;
    updateScrollBar();
    ruler.repaint();
    canvas.repaint();
}

void TrackPanel::scrollBy(double fraction)
{
    const double range = viewEnd - viewStart;
    viewStart = std::max(0.0, viewStart + fraction * range);
    viewEnd   = viewStart + range;
    updateScrollBar();
    ruler.repaint();
    canvas.repaint();
}

void TrackPanel::updateScrollBar()
{
    const double range = viewEnd - viewStart;
    const double total = std::max(engine.getProject().getTotalLengthSec() + range * 0.5, viewEnd);
    hScroll.setRangeLimits(0.0, total, juce::dontSendNotification);
    hScroll.setCurrentRange(viewStart, range, juce::dontSendNotification);
}

void TrackPanel::scrollBarMoved(juce::ScrollBar*, double newStart)
{
    const double range = viewEnd - viewStart;
    viewStart = std::max(0.0, newStart);
    viewEnd   = viewStart + range;
    ruler.repaint();
    canvas.repaint();
}

double TrackPanel::gridStep(int widthPx) const
{
    const auto& proj = engine.getProject();
    const double beat = proj.secondsPerBeat();
    const double bar  = proj.secondsPerBar();
    const double pxPerSec = (double)juce::jmax(1, widthPx) / (viewEnd - viewStart);
    const double steps[] = { beat / 4.0, beat / 2.0, beat, bar, bar * 2.0, bar * 4.0, bar * 8.0, bar * 16.0, bar * 32.0 };
    for (double s : steps)
        if (s * pxPerSec >= 18.0) return s;
    return bar * 64.0;
}

double TrackPanel::snap(double sec, int widthPx, bool enabled) const
{
    if (!enabled) return sec;
    const double step = gridStep(widthPx);
    return std::round(sec / step) * step;
}

void TrackPanel::selectTrack(int index)
{
    selectedTrack = index;
    engine.setSelectedTrack(index);
    for (auto& h : headers) h->repaint();
    canvas.repaint();
}

void TrackPanel::selectClip(int trackIndex, AudioClip* clip)
{
    if (trackIndex >= 0) { selectedTrack = trackIndex; engine.setSelectedTrack(trackIndex); }
    selectedClip = clip;
    for (auto& h : headers) h->repaint();
    canvas.repaint();
}

// ---- editing ------------------------------------------------
void TrackPanel::copySelectedClip()
{
    if (selectedClip) clipboard = std::make_unique<AudioClip>(*selectedClip);
}

void TrackPanel::cutSelectedClip()
{
    copySelectedClip();
    deleteSelectedClip();
}

void TrackPanel::deleteSelectedClip()
{
    if (!selectedClip) return;
    for (auto& t : engine.getProject().tracks)
    {
        const int idx = t->indexOf(selectedClip);
        if (idx < 0) continue;
        engine.pushUndoState();
        const juce::ScopedLock sl(engine.getLock());
        t->removeClip(idx);
        break;
    }
    selectedClip = nullptr;
    engine.sendChangeMessage();
}

void TrackPanel::pasteClip()
{
    auto& tracks = engine.getProject().tracks;
    if (!clipboard || tracks.empty()) return;
    const int ti = juce::jlimit(0, (int)tracks.size() - 1, selectedTrack);

    engine.pushUndoState();
    AudioClip* added = nullptr;
    {
        const juce::ScopedLock sl(engine.getLock());
        added = tracks[(size_t)ti]->addClip();
        *added = *clipboard;
        added->startTimeSec = engine.getPlayheadPositionSec();
    }
    selectClip(ti, added);
    engine.sendChangeMessage();
}

void TrackPanel::duplicateSelectedClip()
{
    if (!selectedClip) return;
    for (int ti = 0; ti < (int)engine.getProject().tracks.size(); ++ti)
    {
        auto& t = *engine.getProject().tracks[(size_t)ti];
        if (t.indexOf(selectedClip) < 0) continue;
        engine.pushUndoState();
        AudioClip* added = nullptr;
        {
            const juce::ScopedLock sl(engine.getLock());
            auto copy = std::make_unique<AudioClip>(*selectedClip);
            copy->startTimeSec = selectedClip->endTimeSec();
            added = copy.get();
            t.clips.push_back(std::move(copy));
        }
        selectClip(ti, added);
        engine.sendChangeMessage();
        return;
    }
}

void TrackPanel::splitSelectedClipAtPlayhead()
{
    auto& tracks = engine.getProject().tracks;
    const double p = engine.getPlayheadPositionSec();

    // the selected clip, or the clip under the playhead on the selected track
    AudioClip* target = nullptr;
    int ti = -1;
    for (int i = 0; i < (int)tracks.size(); ++i)
        if (selectedClip && tracks[(size_t)i]->indexOf(selectedClip) >= 0) { target = selectedClip; ti = i; }

    if (target == nullptr && selectedTrack >= 0 && selectedTrack < (int)tracks.size())
        for (auto& c0 : tracks[(size_t)selectedTrack]->clips)
            if (p > c0->startTimeSec && p < c0->endTimeSec()) { target = c0.get(); ti = selectedTrack; }

    if (target == nullptr || p <= target->startTimeSec + 0.005 || p >= target->endTimeSec() - 0.005)
        return;

    engine.pushUndoState();
    {
        const juce::ScopedLock sl(engine.getLock());
        auto right = std::make_unique<AudioClip>(*target);
        const double cut = p - target->startTimeSec;
        right->startTimeSec = p;
        right->offsetSec    = target->offsetSec + cut;
        right->lengthSec    = target->lengthSec - cut;
        right->fadeInSec    = 0.0;
        right->fadeOutSec   = juce::jmin(target->fadeOutSec, right->lengthSec);
        target->lengthSec   = cut;
        target->fadeOutSec  = 0.0;
        target->fadeInSec   = juce::jmin(target->fadeInSec, target->lengthSec);
        tracks[(size_t)ti]->clips.push_back(std::move(right));
    }
    engine.sendChangeMessage();
    canvas.repaint();
}

bool TrackPanel::importFile(const juce::File& f, int trackIndex, double sec)
{
    auto& tracks = engine.getProject().tracks;
    if (trackIndex < 0 || trackIndex >= (int)tracks.size()) return false;

    juce::AudioFormatManager fmt;
    fmt.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fmt.createReaderFor(f));
    if (!reader || reader->lengthInSamples <= 0) return false;

    auto buf = std::make_shared<juce::AudioBuffer<float>>((int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(buf.get(), 0, (int)reader->lengthInSamples, 0, true, true);

    auto clip = std::make_unique<AudioClip>();
    clip->setSource(buf, reader->sampleRate);
    clip->startTimeSec   = std::max(0.0, sec);
    clip->name           = f.getFileNameWithoutExtension();
    clip->sourceFilePath = f.getFullPathName();
    auto* raw = clip.get();
    {
        const juce::ScopedLock sl(engine.getLock());
        tracks[(size_t)trackIndex]->clips.push_back(std::move(clip));
    }
    selectClip(trackIndex, raw);
    return true;
}

void TrackPanel::importAudio(int trackIndex, double sec)
{
    auto chooser = std::make_shared<juce::FileChooser>(
        TR("Import audio"), juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe = juce::Component::SafePointer<TrackPanel>(this), chooser, trackIndex, sec](const juce::FileChooser& fc)
        {
            if (safe == nullptr || fc.getResults().isEmpty()) return;
            safe->engine.pushUndoState();
            if (safe->importFile(fc.getResult(), trackIndex, sec))
                safe->engine.sendChangeMessage();
        });
}

// ============================================================
//  FxCard
// ============================================================
FxCard::FxCard(const juce::String& t, std::atomic<bool>& en, std::vector<Param> ps, juce::Colour acc)
    : title(t), enabled(en), accent(acc), params(std::move(ps))
{
    power.setToggleState(enabled.load(), juce::dontSendNotification);
    power.setTooltip(TR("On / off"));
    power.onClick = [this] { enabled = power.getToggleState(); repaint(); };
    power.setWantsKeyboardFocus(false);
    addAndMakeVisible(power);

    for (auto& p : params)
    {
        auto* k = knobs.add(new juce::Slider());
        k->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k->setRange(p.min, p.max, p.interval);
        k->setValue(p.value->load(), juce::dontSendNotification);
        k->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
        k->setTextValueSuffix(p.suffix);
        k->setColour(juce::Slider::rotarySliderFillColourId, accent);
        k->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        k->setWantsKeyboardFocus(false);
        auto* target = p.value;
        k->onValueChange = [k, target] { target->store((float)k->getValue()); };
        addAndMakeVisible(k);
    }
}

void FxCard::syncFromModel()
{
    if (power.getToggleState() != enabled.load())
    {
        power.setToggleState(enabled.load(), juce::dontSendNotification);
        repaint();
    }
}

void FxCard::resized()
{
    auto a = getLocalBounds().reduced(10, 8);
    auto top = a.removeFromTop(24);
    power.setBounds(top.removeFromRight(28));
    a.removeFromTop(18);   // knob captions
    const int kw = a.getWidth() / juce::jmax(1, (int)knobs.size());
    for (auto* k : knobs)
        k->setBounds(a.removeFromLeft(kw).reduced(2, 0));
}

void FxCard::paint(juce::Graphics& g)
{
    const bool on = enabled.load();
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(c(Panel));
    g.fillRoundedRectangle(b, 6.0f);
    g.setColour(on ? accent : c(GrisOscuro));
    g.fillRoundedRectangle(b.withHeight(3.0f).reduced(6.0f, 0.0f), 1.5f);
    g.setColour(c(Linea));
    g.drawRoundedRectangle(b, 6.0f, 1.0f);

    auto a = getLocalBounds().reduced(10, 8);
    g.setColour(on ? c(CremaClaro) : c(GrisClaro));
    g.setFont(PRFonts::title(21.0f));
    g.drawText(title, a.removeFromTop(24), juce::Justification::centredLeft);

    auto caps = a.removeFromTop(18);
    const int kw = caps.getWidth() / juce::jmax(1, (int)params.size());
    for (auto& p : params)
        UI::drawCaption(g, caps.removeFromLeft(kw), UI::upper(p.name));

    if (!on)
    {
        g.setColour(c(Negro).withAlpha(0.35f));
        g.fillRoundedRectangle(b.withTrimmedTop(32.0f), 6.0f);
    }
}

// ============================================================
//  MixerPanel::ChannelStrip
// ============================================================
MixerPanel::ChannelStrip::ChannelStrip(Track& t, int idx, MixerPanel& o)
    : track(t), index(idx), owner(o)
{
    auto knob = [this](juce::Slider& s, double mn, double mx, double val, double def, juce::Colour col)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRange(mn, mx, 0.1);
        s.setValue(val, juce::dontSendNotification);
        s.setDoubleClickReturnValue(true, def);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setColour(juce::Slider::rotarySliderFillColourId, col);
        s.setWantsKeyboardFocus(false);
        addAndMakeVisible(s);
    };
    knob(eqHigh, -12, 12, track.eqHigh.gainDb, 0, track.colour);
    knob(eqMid,  -12, 12, track.eqMid.gainDb,  0, track.colour);
    knob(eqLow,  -12, 12, track.eqLow.gainDb,  0, track.colour);
    knob(panKnob, -1, 1, track.pan, 0, c(CremaOscuro));
    panKnob.setRange(-1.0, 1.0, 0.01);
    for (auto* s : { &eqHigh, &eqMid, &eqLow })
        s->setTooltip(TR("EQ gain in dB (double-click: 0)"));
    panKnob.setTooltip(TR("Pan"));

    eqHigh.onValueChange = [this] { track.eqHigh.gainDb = (float)eqHigh.getValue(); track.eqHigh.dirty = true; };
    eqMid .onValueChange = [this] { track.eqMid.gainDb  = (float)eqMid.getValue();  track.eqMid.dirty  = true; };
    eqLow .onValueChange = [this] { track.eqLow.gainDb  = (float)eqLow.getValue();  track.eqLow.dirty  = true; };
    panKnob.onValueChange = [this] { track.pan = (float)panKnob.getValue(); };

    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setRange(0.0, 1.5, 0.001);
    fader.setSkewFactorFromMidPoint(0.5);
    fader.setValue(track.volume, juce::dontSendNotification);
    fader.setDoubleClickReturnValue(true, 1.0);
    fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fader.setWantsKeyboardFocus(false);
    fader.onValueChange = [this] { track.volume = (float)fader.getValue(); repaint(dbArea); };
    addAndMakeVisible(fader);

    auto fxBtn = [this](juce::TextButton& b, std::atomic<bool>& flag, const juce::String& tip)
    {
        b.setClickingTogglesState(true);
        b.setColour(juce::TextButton::buttonColourId, c(NegroSuave));
        b.setColour(juce::TextButton::buttonOnColourId, track.colour);
        b.setColour(juce::TextButton::textColourOffId, c(GrisClaro));
        b.setColour(juce::TextButton::textColourOnId, c(Negro));
        b.setTooltip(tip);
        b.setWantsKeyboardFocus(false);
        b.onClick = [this, &b, &flag] { flag = b.getToggleState(); owner.selectStrip(index); };
        addAndMakeVisible(b);
    };
    fxBtn(drvBtn, track.fx.driveOn,  TR("Overdrive"));
    fxBtn(cmpBtn, track.fx.compOn,   TR("Compressor"));
    fxBtn(dlyBtn, track.fx.delayOn,  TR("Delay"));
    fxBtn(revBtn, track.fx.reverbOn, TR("Reverb"));

    for (auto* b : { &muteBtn, &soloBtn })
    {
        b->setClickingTogglesState(true);
        b->setColour(juce::TextButton::buttonColourId, c(NegroSuave));
        b->setColour(juce::TextButton::textColourOffId, c(GrisClaro));
        b->setColour(juce::TextButton::textColourOnId, c(Negro));
        b->setWantsKeyboardFocus(false);
        addAndMakeVisible(b);
    }
    muteBtn.setColour(juce::TextButton::buttonOnColourId, c(0xFFD1B45A));
    soloBtn.setColour(juce::TextButton::buttonOnColourId, c(0xFF7FA4C9));
    muteBtn.onClick = [this] { track.muted  = muteBtn.getToggleState(); };
    soloBtn.onClick = [this] { track.soloed = soloBtn.getToggleState(); };
    refreshFxButtons();
}

void MixerPanel::ChannelStrip::refreshFxButtons()
{
    drvBtn.setToggleState(track.fx.driveOn.load(),  juce::dontSendNotification);
    cmpBtn.setToggleState(track.fx.compOn.load(),   juce::dontSendNotification);
    dlyBtn.setToggleState(track.fx.delayOn.load(),  juce::dontSendNotification);
    revBtn.setToggleState(track.fx.reverbOn.load(), juce::dontSendNotification);
    muteBtn.setToggleState(track.muted,  juce::dontSendNotification);
    soloBtn.setToggleState(track.soloed, juce::dontSendNotification);
}

void MixerPanel::ChannelStrip::resized()
{
    auto a = getLocalBounds().reduced(8, 6);
    a.removeFromTop(30);   // name

    eqLabels = a.removeFromTop(126);
    auto eq = eqLabels;
    for (auto* k : { &eqHigh, &eqMid, &eqLow })
        k->setBounds(eq.removeFromTop(42).withTrimmedLeft(34).withSizeKeepingCentre(40, 40));

    a.removeFromTop(6);
    auto fx1 = a.removeFromTop(22);
    drvBtn.setBounds(fx1.removeFromLeft(fx1.getWidth() / 2).reduced(1));
    cmpBtn.setBounds(fx1.reduced(1));
    auto fx2 = a.removeFromTop(22);
    dlyBtn.setBounds(fx2.removeFromLeft(fx2.getWidth() / 2).reduced(1));
    revBtn.setBounds(fx2.reduced(1));

    a.removeFromTop(6);
    panArea = a.removeFromTop(40);
    panKnob.setBounds(panArea.withTrimmedLeft(34).withSizeKeepingCentre(38, 38));

    auto ms = a.removeFromBottom(24);
    muteBtn.setBounds(ms.removeFromLeft(ms.getWidth() / 2).reduced(1));
    soloBtn.setBounds(ms.reduced(1));
    dbArea = a.removeFromBottom(20);

    a.removeFromTop(6);
    meterArea = a.removeFromRight(14).withTrimmedTop(4).withTrimmedBottom(4);
    fader.setBounds(a);
}

void MixerPanel::ChannelStrip::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(selected ? c(Seleccion) : c(Panel));
    g.fillRoundedRectangle(b, 6.0f);
    g.setColour(track.colour);
    g.fillRoundedRectangle(b.withHeight(4.0f).reduced(4.0f, 0.0f), 2.0f);
    g.setColour(selected ? c(Acento) : c(Linea));
    g.drawRoundedRectangle(b, 6.0f, selected ? 1.5f : 1.0f);

    g.setColour(c(CremaClaro));
    g.setFont(PRFonts::title(19.0f));
    g.drawFittedText(UI::upper(track.name), getLocalBounds().reduced(8, 0).withHeight(36).withY(4),
                     juce::Justification::centred, 1);

    auto eq = eqLabels;
    for (auto* name : { "HI", "MID", "LO" })
        UI::drawCaption(g, eq.removeFromTop(42).withWidth(32), TR(name), juce::Justification::centredLeft);

    UI::drawCaption(g, panArea.withWidth(32), TR("PAN"), juce::Justification::centredLeft);

    auto m = meterArea.toFloat();
    UI::drawMeter(g, m.withWidth(6.0f), track.meterL.load());
    UI::drawMeter(g, m.withTrimmedLeft(8.0f), track.meterR.load());

    const float db = juce::Decibels::gainToDecibels(track.volume, -60.0f);
    g.setColour(c(CremaOscuro));
    g.setFont(PRFonts::num(11.0f));
    g.drawText(db <= -60.0f ? "-inf dB" : juce::String(db, 1) + " dB", dbArea, juce::Justification::centred);
}

// ============================================================
//  MixerPanel::MasterStrip
// ============================================================
MixerPanel::MasterStrip::MasterStrip(AudioEngine& e) : engine(e)
{
    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setRange(0.0, 1.5, 0.001);
    fader.setSkewFactorFromMidPoint(0.5);
    fader.setValue(engine.getMasterVolume(), juce::dontSendNotification);
    fader.setDoubleClickReturnValue(true, 0.85);
    fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fader.setWantsKeyboardFocus(false);
    fader.onValueChange = [this] { engine.setMasterVolume((float)fader.getValue()); repaint(); };
    addAndMakeVisible(fader);
}

void MixerPanel::MasterStrip::resized()
{
    auto a = getLocalBounds().reduced(10, 6);
    a.removeFromTop(40);
    a.removeFromBottom(30);
    meterArea = a.removeFromRight(18).reduced(0, 4);
    fader.setBounds(a);
}

void MixerPanel::MasterStrip::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(c(Panel));
    g.fillRoundedRectangle(b, 6.0f);
    g.setColour(c(Acento));
    g.fillRoundedRectangle(b.withHeight(4.0f).reduced(4.0f, 0.0f), 2.0f);
    g.setColour(c(Linea));
    g.drawRoundedRectangle(b, 6.0f, 1.0f);

    g.setColour(c(CremaClaro));
    g.setFont(PRFonts::title(21.0f));
    g.drawText("MASTER", getLocalBounds().withHeight(40).withY(4), juce::Justification::centred);

    auto m = meterArea.toFloat();
    UI::drawMeter(g, m.withWidth(8.0f), engine.getOutputLevelL());
    UI::drawMeter(g, m.withTrimmedLeft(10.0f), engine.getOutputLevelR());

    const float db = juce::Decibels::gainToDecibels(engine.getMasterVolume(), -60.0f);
    g.setColour(c(CremaOscuro));
    g.setFont(PRFonts::num(11.0f));
    g.drawText(juce::String(db, 1) + " dB", getLocalBounds().removeFromBottom(30), juce::Justification::centred);
}

// ============================================================
//  MixerPanel
// ============================================================
MixerPanel::MixerPanel(AudioEngine& eng) : engine(eng), master(eng)
{
    addAndMakeVisible(scrollView);
    addAndMakeVisible(master);
    scrollView.setViewedComponent(&stripContainer, false);
    scrollView.setScrollBarsShown(false, true);
    refresh();
    startTimerHz(30);
}

void MixerPanel::refresh()
{
    strips.clear();
    stripContainer.removeAllChildren();
    auto& tracks = engine.getProject().tracks;
    for (int i = 0; i < (int)tracks.size(); ++i)
    {
        auto s = std::make_unique<ChannelStrip>(*tracks[(size_t)i], i, *this);
        stripContainer.addAndMakeVisible(s.get());
        strips.push_back(std::move(s));
    }
    selected = juce::jlimit(0, juce::jmax(0, (int)strips.size() - 1), engine.getSelectedTrack());
    for (auto& s : strips) s->selected = (s->index == selected);
    buildRack();
    resized();
}

void MixerPanel::selectStrip(int index)
{
    engine.setSelectedTrack(index);
    if (index == selected && rack.size() > 0) return;
    selected = index;
    for (auto& s : strips) { s->selected = (s->index == selected); s->repaint(); }
    buildRack();
    resized();
}

void MixerPanel::buildRack()
{
    for (auto* card : rack) removeChildComponent(card);
    rack.clear();

    auto& tracks = engine.getProject().tracks;
    if (selected < 0 || selected >= (int)tracks.size()) return;
    auto& t  = *tracks[(size_t)selected];
    auto& fx = t.fx;
    const auto col = t.colour;

    rack.add(new FxCard(TR("OVERDRIVE"), fx.driveOn, {
        { TR("Drive"), &fx.driveAmount, 0.0, 1.0, 0.01, "" },
        { TR("Tone"),  &fx.driveTone,   0.0, 1.0, 0.01, "" },
        { TR("Level"), &fx.driveLevel,  0.0, 1.0, 0.01, "" } }, col));
    rack.add(new FxCard(TR("COMPRESSOR"), fx.compOn, {
        { TR("Thresh"), &fx.compThreshold, -48.0, 0.0, 0.5, " dB" },
        { TR("Ratio"),  &fx.compRatio,       1.0, 20.0, 0.1, ":1" },
        { TR("Makeup"), &fx.compMakeup,      0.0, 24.0, 0.5, " dB" } }, col));
    rack.add(new FxCard(TR("DELAY"), fx.delayOn, {
        { TR("Time"),     &fx.delayTimeMs,   20.0, 1500.0, 1.0, " ms" },
        { TR("Feedback"), &fx.delayFeedback,  0.0, 0.9, 0.01, "" },
        { TR("Mix"),      &fx.delayMix,       0.0, 1.0, 0.01, "" } }, col));
    rack.add(new FxCard(TR("REVERB"), fx.reverbOn, {
        { TR("Size"), &fx.reverbSize, 0.0, 1.0, 0.01, "" },
        { TR("Damp"), &fx.reverbDamp, 0.0, 1.0, 0.01, "" },
        { TR("Mix"),  &fx.reverbMix,  0.0, 1.0, 0.01, "" } }, col));

    for (auto* card : rack) addAndMakeVisible(card);
}

void MixerPanel::resized()
{
    auto a = getLocalBounds().reduced(16, 12);
    a.removeFromTop(40);   // title

    rackArea = a.removeFromBottom(196);
    a.removeFromBottom(12);

    master.setBounds(a.removeFromRight(128).withTrimmedBottom(12));
    a.removeFromRight(12);
    scrollView.setBounds(a);

    const int sw = 118;
    const int h = a.getHeight() - 12;   // leave room for the horizontal scrollbar
    stripContainer.setSize(juce::jmax(1, (int)strips.size() * (sw + 8)), juce::jmax(1, h));
    for (int i = 0; i < (int)strips.size(); ++i)
        strips[(size_t)i]->setBounds(i * (sw + 8), 0, sw, stripContainer.getHeight());

    auto r = rackArea.withTrimmedTop(30);
    const int n = juce::jmax(1, rack.size());
    const int cw = (r.getWidth() - (n - 1) * 10) / n;
    for (auto* card : rack)
    {
        card->setBounds(r.removeFromLeft(cw));
        r.removeFromLeft(10);
    }
}

void MixerPanel::paint(juce::Graphics& g)
{
    g.fillAll(c(Negro));
    auto a = getLocalBounds().reduced(16, 12);
    UI::drawSectionTitle(g, a.removeFromTop(34), TR("MIXER"), TR("EQ, effects and levels per track"));

    auto& tracks = engine.getProject().tracks;
    juce::String name = (selected >= 0 && selected < (int)tracks.size()) ? tracks[(size_t)selected]->name : "";
    g.setColour(c(GrisClaro));
    g.setFont(PRFonts::title(19.0f));
    g.drawText(TR("DEVICES  /  ") + UI::upper(name), rackArea.withHeight(24), juce::Justification::centredLeft);
}

void MixerPanel::timerCallback()
{
    if (strips.size() != engine.getProject().tracks.size()) { refresh(); return; }
    if (engine.getSelectedTrack() != selected && engine.getSelectedTrack() < (int)strips.size())
        selectStrip(engine.getSelectedTrack());
    for (auto& s : strips)
    {
        s->refreshFxButtons();
        s->repaint(s->meterArea.expanded(2));
    }
    for (auto* card : rack) card->syncFromModel();
    master.repaint(master.meterArea.expanded(2));
}

// ============================================================
//  PluginsPanel
// ============================================================
PluginsPanel::PluginsPanel(AudioEngine& eng) : engine(eng)
{
    scanBtn.setButtonText(TR("SCAN VST3 FOLDER"));
    loadBtn.setButtonText(TR("LOAD SELECTED"));
    formatManager.addFormat(std::make_unique<juce::VST3PluginFormat>());

    addAndMakeVisible(pluginList);
    addAndMakeVisible(scanBtn);
    addAndMakeVisible(loadBtn);
    addAndMakeVisible(statusLabel);

    pluginList.setModel(this);
    pluginList.setRowHeight(30);
    pluginList.setColour(juce::ListBox::backgroundColourId, c(Panel));

    statusLabel.setText(TR("No plugins scanned yet. Choose the folder where your VST3 plugins are installed."),
                        juce::dontSendNotification);
    statusLabel.setFont(PRFonts::body(14.0f));
    statusLabel.setColour(juce::Label::textColourId, c(GrisClaro));

    scanBtn.setColour(juce::TextButton::buttonColourId, c(Acento));
    scanBtn.setColour(juce::TextButton::textColourOffId, c(Negro));
    scanBtn.onClick = [this] { scanForPlugins(); };
    loadBtn.onClick = [this] { loadSelectedPlugin(); };
    loadBtn.setEnabled(false);
    UI::noFocus({ &scanBtn, &loadBtn });
}

PluginsPanel::~PluginsPanel() = default;

int PluginsPanel::getNumRows() { return knownPlugins.getNumTypes(); }

void PluginsPanel::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel)
{
    g.fillAll(sel ? c(Acento).withAlpha(0.25f) : (row % 2 ? c(Panel) : c(NegroSuave)));
    auto types = knownPlugins.getTypes();
    if (row < 0 || row >= types.size()) return;
    const auto& d = types.getReference(row);
    g.setColour(c(CremaClaro));
    g.setFont(PRFonts::bodyBold(15.0f));
    g.drawText(d.name, 12, 0, w / 2, h, juce::Justification::centredLeft);
    g.setColour(c(GrisClaro));
    g.setFont(PRFonts::body(14.0f));
    g.drawText(d.manufacturerName + "   " + d.version, w / 2, 0, w / 2 - 12, h, juce::Justification::centredRight);
}

void PluginsPanel::listBoxItemDoubleClicked(int, const juce::MouseEvent&) { loadSelectedPlugin(); }

void PluginsPanel::resized()
{
    auto a = getLocalBounds().reduced(16, 12);
    a.removeFromTop(50);
    auto row = a.removeFromTop(34);
    scanBtn.setBounds(row.removeFromLeft(190));
    row.removeFromLeft(8);
    loadBtn.setBounds(row.removeFromLeft(160));
    a.removeFromTop(10);
    statusLabel.setBounds(a.removeFromTop(24));
    a.removeFromTop(8);
    pluginList.setBounds(a);
}

void PluginsPanel::paint(juce::Graphics& g)
{
    g.fillAll(c(Negro));
    UI::drawSectionTitle(g, getLocalBounds().reduced(16, 12).removeFromTop(34), TR("PLUGINS"), TR("VST3 scanner"));
}

void PluginsPanel::scanForPlugins()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        TR("Select VST3 folder"), juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory), "");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this, chooser](const juce::FileChooser& fc)
        {
            if (fc.getResults().isEmpty()) return;
            statusLabel.setText(TR("Scanning..."), juce::dontSendNotification);
            juce::FileSearchPath path(fc.getResult().getFullPathName());
            juce::PluginDirectoryScanner scanner(knownPlugins, *formatManager.getFormat(0), path, true, juce::File());
            juce::String current;
            while (scanner.scanNextFile(true, current)) {}
            statusLabel.setText(TR("Found %1 plugin(s).").replace("%1", juce::String(knownPlugins.getNumTypes())),
                                juce::dontSendNotification);
            pluginList.updateContent();
            loadBtn.setEnabled(knownPlugins.getNumTypes() > 0);
        });
}

void PluginsPanel::loadSelectedPlugin()
{
    const int row = pluginList.getSelectedRow();
    auto types = knownPlugins.getTypes();
    if (row < 0 || row >= types.size())
    {
        statusLabel.setText(TR("Select a plugin first."), juce::dontSendNotification);
        return;
    }
    statusLabel.setText(TR("\"%1\" selected. Full plugin hosting is planned for a future version; "
                           "use the built-in effects in the MIXER meanwhile.").replace("%1", types.getReference(row).name),
                        juce::dontSendNotification);
}

// ============================================================
//  ExportPanel
// ============================================================
ExportPanel::ExportPanel(AudioEngine& eng) : engine(eng)
{
    exportWavBtn.setButtonText(TR("EXPORT WAV"));
    exportMp3Btn.setButtonText(TR("EXPORT MP3"));
    addAndMakeVisible(exportWavBtn);
    addAndMakeVisible(exportMp3Btn);
    addAndMakeVisible(bitrateBox);
    addAndMakeVisible(statusLabel);

    exportWavBtn.setColour(juce::TextButton::buttonColourId, c(Acento));
    exportWavBtn.setColour(juce::TextButton::textColourOffId, c(Negro));

    bitrateBox.addItem("128 kbps", 128);
    bitrateBox.addItem("192 kbps", 192);
    bitrateBox.addItem("320 kbps", 320);
    bitrateBox.setSelectedId(192);

    statusLabel.setFont(PRFonts::body(14.0f));
    statusLabel.setColour(juce::Label::textColourId, c(Medidor));
    UI::noFocus({ &exportWavBtn, &exportMp3Btn, &bitrateBox });

    exportWavBtn.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser>(
            TR("Export WAV"),
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                .getChildFile(engine.getProject().name + ".wav"),
            "*.wav");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f == juce::File()) return;
                auto out = f.withFileExtension("wav");
                statusLabel.setText(TR("Rendering..."), juce::dontSendNotification);
                const bool ok = engine.exportToWav(out);
                statusLabel.setColour(juce::Label::textColourId, ok ? c(Medidor) : c(RojoClaro));
                statusLabel.setText(ok ? TR("Exported: ") + out.getFullPathName() : TR("Export failed."),
                                    juce::dontSendNotification);
            });
    };

    exportMp3Btn.onClick = []
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, TR("MP3 export"),
            TR("MP3 export needs the LAME encoder, which is not included in this build.\n"
               "Export as WAV and convert it with any free converter."), "OK");
    };
}

void ExportPanel::resized()
{
    auto a = getLocalBounds().reduced(16, 12);
    a.removeFromTop(50);
    card = a.removeFromTop(250).withWidth(juce::jmin(620, a.getWidth()));
    auto in = card.reduced(22, 20);
    in.removeFromTop(86);
    auto row1 = in.removeFromTop(40);
    exportWavBtn.setBounds(row1.removeFromLeft(200));
    in.removeFromTop(12);
    auto row2 = in.removeFromTop(32);
    exportMp3Btn.setBounds(row2.removeFromLeft(200));
    row2.removeFromLeft(10);
    bitrateBox.setBounds(row2.removeFromLeft(120));
    in.removeFromTop(16);
    statusLabel.setBounds(in.removeFromTop(24));
}

void ExportPanel::paint(juce::Graphics& g)
{
    g.fillAll(c(Negro));
    UI::drawSectionTitle(g, getLocalBounds().reduced(16, 12).removeFromTop(34), TR("EXPORT"), TR("Render your project"));

    auto b = card.toFloat();
    g.setColour(c(Panel));
    g.fillRoundedRectangle(b, 8.0f);
    g.setColour(c(Linea));
    g.drawRoundedRectangle(b.reduced(0.5f), 8.0f, 1.0f);

    auto in = card.reduced(22, 20);
    g.setColour(c(CremaClaro));
    g.setFont(PRFonts::title(26.0f));
    g.drawText(TR("MIXDOWN"), in.removeFromTop(30), juce::Justification::centredLeft);
    g.setColour(c(GrisClaro));
    g.setFont(PRFonts::body(14.5f));
    g.drawFittedText(TR("Renders every track with its EQ, effects, volume and pan to a 24-bit stereo WAV. "
                        "Includes 2 seconds of tail for reverb and delay."),
                     in.removeFromTop(48), juce::Justification::topLeft, 3);
}

// ============================================================
//  SettingsPanel
// ============================================================
SettingsPanel::SettingsPanel(AudioEngine& eng, juce::PropertiesFile& s)
    : engine(eng), settings(s),
      deviceSelector(eng.getDeviceManager(), 0, 2, 0, 2, false, false, false, false)
{
    customColourBtn.setButtonText(TR("CUSTOM..."));
    showTipsToggle.setButtonText(TR("Show quick-start tips when the program opens"));

    // ---- language ----
    languageBox.addItemList(Lang::names(), 1);
    languageBox.setSelectedItemIndex(Lang::current, juce::dontSendNotification);
    languageBox.onChange = [this]
    {
        if (onLanguageChanged) onLanguageChanged(languageBox.getSelectedItemIndex());
    };
    addAndMakeVisible(languageBox);

    // ---- theme ----
    themeBox.addItemList(Themes::names(), 1);
    themeBox.setSelectedItemIndex(Themes::current, juce::dontSendNotification);
    themeBox.onChange = [this]
    {
        if (onThemeChanged) onThemeChanged(themeBox.getSelectedItemIndex(), Themes::oledAccent);
    };
    addAndMakeVisible(themeBox);

    for (auto col : Themes::oledPresets)
    {
        auto* sw = swatches.add(new ColourSwatch(col));
        sw->setToggleState(col == Themes::oledAccent, juce::dontSendNotification);
        sw->setTooltip(TR("Outline colour"));
        sw->onClick = [this, col] { if (onThemeChanged) onThemeChanged(Themes::Oled, col); };
        addAndMakeVisible(sw);
    }
    customColourBtn.setTooltip(TR("Pick any colour"));
    customColourBtn.onClick = [this] { openCustomColour(); };
    addAndMakeVisible(customColourBtn);

    // ---- audio ----
    addAndMakeVisible(deviceSelector);
    addAndMakeVisible(bufferSizeBox);
    for (int sz : { 64, 128, 256, 512, 1024, 2048 })
        bufferSizeBox.addItem(juce::String(sz) + TR(" samples"), sz);
    bufferSizeBox.setSelectedId(engine.getDeviceBufferSize(), juce::dontSendNotification);
    bufferSizeBox.onChange = [this] { engine.setBufferSize(bufferSizeBox.getSelectedId()); };

    // ---- help ----
    addAndMakeVisible(showTipsToggle);
    showTipsToggle.setToggleState(settings.getBoolValue("showTips", true), juce::dontSendNotification);
    showTipsToggle.onClick = [this]
    {
        settings.setValue("showTips", showTipsToggle.getToggleState());
        settings.saveIfNeeded();
        if (onShowTipsChanged) onShowTipsChanged(showTipsToggle.getToggleState());
    };

    UI::noFocus({ &languageBox, &themeBox, &customColourBtn, &bufferSizeBox, &showTipsToggle });
    updateAccentVisibility();
}

void SettingsPanel::updateAccentVisibility()
{
    const bool oled = Themes::current == Themes::Oled;
    for (auto* sw : swatches) sw->setVisible(oled);
    customColourBtn.setVisible(oled);
}

void SettingsPanel::openCustomColour()
{
    auto callback = onThemeChanged;
    auto picker = std::make_unique<AccentPicker>(juce::Colour(Themes::oledAccent),
        [callback](juce::Colour col) { if (callback) callback(Themes::Oled, col.getARGB()); });
    juce::CallOutBox::launchAsynchronously(std::move(picker), customColourBtn.getScreenBounds(), nullptr);
}

void SettingsPanel::resized()
{
    auto a = getLocalBounds().reduced(16, 12);
    a.removeFromTop(50);

    auto row0 = a.removeFromTop(30);
    row0.removeFromLeft(150);
    languageBox.setBounds(row0.removeFromLeft(220));
    a.removeFromTop(10);

    auto row = a.removeFromTop(30);
    row.removeFromLeft(150);
    themeBox.setBounds(row.removeFromLeft(220));
    a.removeFromTop(10);

    accentRow = a.removeFromTop(32);
    auto sr = accentRow.withTrimmedLeft(146);
    for (auto* sw : swatches) sw->setBounds(sr.removeFromLeft(30));
    sr.removeFromLeft(8);
    customColourBtn.setBounds(sr.removeFromLeft(100).withSizeKeepingCentre(100, 26));
    a.removeFromTop(18);

    auto row2 = a.removeFromTop(30);
    row2.removeFromLeft(150);
    bufferSizeBox.setBounds(row2.removeFromLeft(170));
    a.removeFromTop(10);

    auto row3 = a.removeFromTop(30);
    row3.removeFromLeft(146);
    showTipsToggle.setBounds(row3.withWidth(420));
    a.removeFromTop(18);

    deviceSelector.setBounds(a.withWidth(juce::jmin(640, a.getWidth())));
}

void SettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(c(Negro));
    auto a = getLocalBounds().reduced(16, 12);
    UI::drawSectionTitle(g, a.removeFromTop(34), TR("SETTINGS"), TR("Appearance, language, audio device and latency"));
    a.removeFromTop(16);

    g.setColour(c(Crema));
    g.setFont(PRFonts::bodyBold(15.0f));
    g.drawText(TR("Language"), a.removeFromTop(30).withWidth(150), juce::Justification::centredLeft);
    a.removeFromTop(10);
    g.drawText(TR("Theme"), a.removeFromTop(30).withWidth(150), juce::Justification::centredLeft);
    a.removeFromTop(10);
    if (Themes::current == Themes::Oled)
        g.drawText(TR("Outline colour"), a.removeFromTop(32).withWidth(150), juce::Justification::centredLeft);
    else
        a.removeFromTop(32);
    a.removeFromTop(18);
    g.drawText(TR("Buffer size"), a.removeFromTop(30).withWidth(150), juce::Justification::centredLeft);
    a.removeFromTop(10);
    g.drawText(TR("Help"), a.removeFromTop(30).withWidth(150), juce::Justification::centredLeft);

    g.setColour(c(Linea));
    g.fillRect(16, accentRow.getBottom() + 9, getWidth() - 32, 1);
}

void SettingsPanel::visibilityChanged()
{
    if (isVisible())
        showTipsToggle.setToggleState(settings.getBoolValue("showTips", true), juce::dontSendNotification);
}

// ============================================================
//  MainComponent
// ============================================================
MainComponent::MainComponent()
{
    juce::LookAndFeel::setDefaultLookAndFeel(&theme);

    // user settings file (remembers "don't show tips again", etc.)
    juce::PropertiesFile::Options opts;
    opts.applicationName     = "Pennyroyal Audio";
    opts.filenameSuffix      = "settings";
    opts.folderName          = "Pennyroyal Audio";
    opts.osxLibrarySubFolder = "Application Support";
    appProps.setStorageParameters(opts);

    Lang::current = juce::jlimit(0, 1, settings().getIntValue("language", 0));
    Themes::apply(settings().getIntValue("theme", 0),
                  juce::Colour::fromString(settings().getValue("oledAccent", "ffd47a4a")).getARGB());
    theme.refreshColours();

    setSize(1400, 880);

    engine.initialise();
    engine.addChangeListener(this);
    engine.addTrack("Audio");
    engine.addTrack("Audio");

    // Create the SafePointer here (outside the lambdas): MSVC does not accept
    // 'this' inside an init-capture of a nested lambda.
    juce::Component::SafePointer<MainComponent> safeThis(this);
    splash = std::make_unique<SplashScreenComponent>([safeThis]
    {
        juce::MessageManager::callAsync([safeThis]
        {
            if (safeThis != nullptr) safeThis->showDAW();
        });
    });
    addAndMakeVisible(*splash);
    splash->setBounds(getLocalBounds());

    addKeyListener(this);
    setWantsKeyboardFocus(true);
    startTimerHz(4);
}

MainComponent::~MainComponent()
{
    engine.removeChangeListener(this);
    removeKeyListener(this);
    settings().saveIfNeeded();
    tip.reset();
    tabs.reset();
    topBar.reset();
    splash.reset();
    engine.shutdown();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void MainComponent::showDAW()
{
    if (topBar != nullptr) return;
    buildUI();

    // Fade the splash out over the DAW
    if (splash != nullptr)
    {
        splash->toFront(false);
        juce::Desktop::getInstance().getAnimator().fadeOut(splash.get(), 600);
        splash.reset();
    }
    grabKeyboardFocus();

    if (settings().getBoolValue("showTips", true))
    {
        juce::Component::SafePointer<MainComponent> safeThis(this);
        juce::Timer::callAfterDelay(700, [safeThis]
        {
            if (safeThis != nullptr) safeThis->showTip(true);
        });
    }
}

void MainComponent::buildUI()
{
    juce::Component::SafePointer<MainComponent> safeThis(this);

    topBar = std::make_unique<TopBar>(engine);
    topBar->onOpen = [this] { openProject(); };
    topBar->onSave = [this] { saveProject(false); };
    topBar->onUndo = [this] { engine.undo(); };
    topBar->onRedo = [this] { engine.redo(); };
    topBar->onHelp = [this] { showTip(tip == nullptr || !tip->isVisible()); };

    trackPanel = new TrackPanel(engine);
    mixerPanel = new MixerPanel(engine);

    tabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    tabs->setTabBarDepth(34);
    tabs->setOutline(0);
    tabs->setIndent(10);
    tabs->addTab(TR("ARRANGE"),  c(Negro), trackPanel, true);
    tabs->addTab(TR("MIXER"),    c(Negro), mixerPanel, true);
    tabs->addTab(TR("TUNER"),    c(Negro), new TunerPanel(engine), true);
    tabs->addTab(TR("PLUGINS"),  c(Negro), new PluginsPanel(engine), true);
    tabs->addTab(TR("EXPORT"),   c(Negro), new ExportPanel(engine), true);
    auto* sp = new SettingsPanel(engine, settings());
    sp->onThemeChanged = [safeThis](int id, juce::uint32 accent)
    {
        // rebuild after the settings panel's own callback has finished
        juce::MessageManager::callAsync([safeThis, id, accent]
        {
            if (safeThis != nullptr) safeThis->applyTheme(id, accent, true);
        });
    };
    sp->onLanguageChanged = [safeThis](int id)
    {
        juce::MessageManager::callAsync([safeThis, id]
        {
            if (safeThis != nullptr) safeThis->applyLanguage(id);
        });
    };
    tabs->addTab(TR("SETTINGS"), c(Negro), sp, true);
    tabs->getTabbedButtonBar().setWantsKeyboardFocus(false);

    addAndMakeVisible(*topBar);
    addAndMakeVisible(*tabs);

    tip = std::make_unique<QuickStartTip>();
    tip->onClose = [this](bool dontShowAgain)
    {
        settings().setValue("showTips", !dontShowAgain);
        settings().saveIfNeeded();
        showTip(false);
    };
    addChildComponent(*tip);
    resized();
}

void MainComponent::applyTheme(int themeId, juce::uint32 accent, bool rebuild)
{
    Themes::apply(themeId, accent);
    theme.refreshColours();
    settings().setValue("theme", Themes::current);
    settings().setValue("oledAccent", juce::Colour(accent).toString());
    settings().saveIfNeeded();

    if (!rebuild || !topBar) { repaint(); return; }
    rebuildUI();
    setStatus(TR("Theme: ") + Themes::names()[Themes::current]);
}

void MainComponent::applyLanguage(int languageId)
{
    Lang::current = juce::jlimit(0, 1, languageId);
    settings().setValue("language", Lang::current);
    settings().saveIfNeeded();
    if (!topBar) return;
    rebuildUI();
    setStatus(TR("Language: ") + Lang::names()[Lang::current]);
}

void MainComponent::rebuildUI()
{
    // remember the view, then rebuild every component (new colours / texts)
    const int    tab        = tabs ? tabs->getCurrentTabIndex() : 0;
    const double vs         = trackPanel ? trackPanel->viewStart : 0.0;
    const double ve         = trackPanel ? trackPanel->viewEnd   : 24.0;
    const bool   tipVisible = tip && tip->isVisible();

    tip.reset();
    tabs.reset();
    topBar.reset();
    trackPanel = nullptr;
    mixerPanel = nullptr;

    buildUI();
    tabs->setCurrentTabIndex(tab, false);
    trackPanel->viewStart = vs;
    trackPanel->viewEnd   = ve;
    trackPanel->updateScrollBar();
    if (tipVisible) tip->setVisible(true);

    resized();
    repaint();
    grabKeyboardFocus();
}

void MainComponent::resized()
{
    auto a = getLocalBounds();
    if (splash) splash->setBounds(a);
    if (!topBar) return;

    topBar->setBounds(a.removeFromTop(TOPBAR_H));
    a.removeFromBottom(STATUS_H);
    tabs->setBounds(a);

    if (tip)
        tip->setBounds(a.getRight() - QuickStartTip::WIDTH - 6, a.getBottom() - QuickStartTip::HEIGHT - 2,
                       QuickStartTip::WIDTH, QuickStartTip::HEIGHT);
}

void MainComponent::showTip(bool show)
{
    if (!tip) return;
    auto& anim = juce::Desktop::getInstance().getAnimator();
    if (show)
    {
        tip->setDontShowAgain(!settings().getBoolValue("showTips", true));
        tip->toFront(false);
        anim.fadeIn(tip.get(), 250);
    }
    else if (tip->isVisible())
    {
        anim.fadeOut(tip.get(), 200);
    }
    grabKeyboardFocus();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(c(Negro));
    if (!topBar) return;

    // status bar
    auto s = getLocalBounds().removeFromBottom(STATUS_H);
    g.setColour(c(Panel));
    g.fillRect(s);
    g.setColour(c(Linea));
    g.fillRect(s.removeFromTop(1));
    s = s.reduced(12, 0);

    const double now = juce::Time::getMillisecondCounterHiRes();
    const bool showMsg = statusMessage.isNotEmpty() && now - statusTime < 4000.0;
    g.setFont(PRFonts::body(13.0f));
    g.setColour(showMsg ? c(Acento) : c(GrisClaro));
    g.drawText(showMsg ? statusMessage
                       : TR("SPACE play   R record (armed or selected track)   S split   "
                            "CTRL+D duplicate   CTRL+Z undo   ALT+drag: no snap   CTRL+wheel: zoom"),
               s, juce::Justification::centredLeft);

    const double sr = engine.getDeviceSampleRate();
    const int    bs = engine.getDeviceBufferSize();
    g.setColour(c(GrisClaro));
    g.setFont(PRFonts::num(11.0f));
    g.drawText(engine.getProject().name + "    " + juce::String(sr / 1000.0, 1) + " kHz   " +
               juce::String(bs) + " smp   " + juce::String(1000.0 * bs / sr, 1) + " ms",
               s, juce::Justification::centredRight);
}

void MainComponent::paintOverChildren(juce::Graphics&) {}

void MainComponent::timerCallback()
{
    if (topBar) repaint(getLocalBounds().removeFromBottom(STATUS_H));
}

void MainComponent::setStatus(const juce::String& msg)
{
    statusMessage = msg;
    statusTime    = juce::Time::getMillisecondCounterHiRes();
    repaint(getLocalBounds().removeFromBottom(STATUS_H));
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (!topBar || !trackPanel) return false;

    const auto mods = key.getModifiers();
    const int  code = key.getKeyCode();
    const bool cmd  = mods.isCommandDown();
    auto is = [code](char ch) { return code == ch || code == (ch + ('a' - 'A')); };

    if (code == juce::KeyPress::spaceKey)               { topBar->togglePlay(); return true; }
    if (code == juce::KeyPress::homeKey)                { engine.setPlayheadPositionSec(0.0); return true; }

    if (cmd && is('Z') && mods.isShiftDown())           { engine.redo(); return true; }
    if (cmd && is('Z'))                                 { engine.undo(); return true; }
    if (cmd && is('Y'))                                 { engine.redo(); return true; }
    if (cmd && is('S'))                                 { saveProject(mods.isShiftDown()); return true; }
    if (cmd && is('O'))                                 { openProject(); return true; }
    if (cmd && is('X'))                                 { trackPanel->cutSelectedClip(); return true; }
    if (cmd && is('C'))                                 { trackPanel->copySelectedClip(); setStatus(TR("Clip copied")); return true; }
    if (cmd && is('V'))                                 { trackPanel->pasteClip(); return true; }
    if (cmd && is('D'))                                 { trackPanel->duplicateSelectedClip(); return true; }

    if (!cmd && !mods.isAltDown())
    {
        if (is('R'))                                    { topBar->toggleRecord(); return true; }
        if (is('S'))                                    { trackPanel->splitSelectedClipAtPlayhead(); return true; }
        if (is('L'))                                    { topBar->toggleLoop(); return true; }
        if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey)
                                                        { trackPanel->deleteSelectedClip(); return true; }
        if (key.getTextCharacter() == '+' || key.getTextCharacter() == '=')
        { trackPanel->zoomAround(engine.getPlayheadPositionSec(), 0.75); return true; }
        if (key.getTextCharacter() == '-')
        { trackPanel->zoomAround(engine.getPlayheadPositionSec(), 1.33); return true; }
    }
    return false;
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    if (topBar)
    {
        topBar->refreshTrackList();
        topBar->updateButtonStates();
    }
}

// ---- project files ------------------------------------------
void MainComponent::writeProject(const juce::File& f)
{
    engine.getProject().name = f.getFileNameWithoutExtension();
    auto xml = engine.saveProjectToXml(f);
    if (xml && f.replaceWithText(xml->toString()))
    {
        currentProjectFile = f;
        setStatus(TR("Saved ") + f.getFullPathName());
        if (auto* w = findParentComponentOfClass<juce::DocumentWindow>())
            w->setName("Pennyroyal Audio  -  " + engine.getProject().name);
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, TR("Save failed"),
                                               TR("Could not write ") + f.getFullPathName());
    }
}

void MainComponent::saveProject(bool saveAs)
{
    if (!saveAs && currentProjectFile.existsAsFile())
    {
        writeProject(currentProjectFile);
        return;
    }
    auto chooser = std::make_shared<juce::FileChooser>(
        TR("Save project"),
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile(engine.getProject().name + ".pennyr"),
        "*.pennyr");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                         juce::FileBrowserComponent::warnAboutOverwriting,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f != juce::File()) writeProject(f.withFileExtension("pennyr"));
        });
}

void MainComponent::openProject()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        TR("Open project"), juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.pennyr");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (!f.existsAsFile()) return;
            auto xml = juce::XmlDocument::parse(f);
            if (xml && engine.loadProjectFromXml(*xml))
            {
                currentProjectFile = f;
                engine.getProject().name = f.getFileNameWithoutExtension();
                setStatus(TR("Opened ") + f.getFileName());
                if (auto* w = findParentComponentOfClass<juce::DocumentWindow>())
                    w->setName("Pennyroyal Audio  -  " + engine.getProject().name);
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, TR("Open failed"),
                                                       TR("This file is not a valid Pennyroyal project."));
            }
        });
}
