#pragma once
#include <JuceHeader.h>

// ============================================================
//  InUtero Colour Palette  -- "In Utero" (Nirvana, 1993)
//  Dark, organic, worn aesthetic
// ============================================================
namespace InUtero
{
    // Base colours
    static constexpr juce::uint32 Negro       = 0xFF0D0D0D;
    static constexpr juce::uint32 NegroSuave  = 0xFF1A1A1A;
    static constexpr juce::uint32 GrisOscuro  = 0xFF2A2A2A;
    static constexpr juce::uint32 GrisMedio   = 0xFF3D3D3D;
    static constexpr juce::uint32 GrisClaro   = 0xFF6B6B6B;
    static constexpr juce::uint32 VerdeMosgo  = 0xFF4A5240;
    static constexpr juce::uint32 VerdeClaro  = 0xFF6B7A5A;
    static constexpr juce::uint32 Crema       = 0xFFE8DCC8;
    static constexpr juce::uint32 CremaClaro  = 0xFFF0EBE0;
    static constexpr juce::uint32 CremaOscuro = 0xFFC8BAA0;
    static constexpr juce::uint32 Rojo        = 0xFF8B1A1A;
    static constexpr juce::uint32 RojoClaro   = 0xFFB02020;
    static constexpr juce::uint32 Ocre        = 0xFF7A6840;

    static inline juce::Colour c(juce::uint32 h) { return juce::Colour(h); }
}

// ============================================================
//  InUteroTheme  --  custom JUCE LookAndFeel
// ============================================================
class InUteroTheme : public juce::LookAndFeel_V4
{
public:
    InUteroTheme()
    {
        using namespace InUtero;

        // Window / component backgrounds
        setColour(juce::ResizableWindow::backgroundColourId, c(Negro));
        setColour(juce::DocumentWindow::backgroundColourId,  c(Negro));

        // Text buttons
        setColour(juce::TextButton::buttonColourId,   c(GrisOscuro));
        setColour(juce::TextButton::buttonOnColourId, c(VerdeMosgo));
        setColour(juce::TextButton::textColourOffId,  c(CremaOscuro));
        setColour(juce::TextButton::textColourOnId,   c(CremaClaro));

        // Labels
        setColour(juce::Label::textColourId,       c(Crema));
        setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

        // Sliders
        setColour(juce::Slider::thumbColourId,             c(VerdeMosgo));
        setColour(juce::Slider::trackColourId,             c(GrisOscuro));
        setColour(juce::Slider::backgroundColourId,        c(Negro));
        setColour(juce::Slider::rotarySliderFillColourId,  c(VerdeMosgo));
        setColour(juce::Slider::rotarySliderOutlineColourId, c(GrisMedio));
        setColour(juce::Slider::textBoxTextColourId,       c(CremaOscuro));
        setColour(juce::Slider::textBoxBackgroundColourId, c(GrisOscuro));
        setColour(juce::Slider::textBoxOutlineColourId,    c(GrisMedio));

        // ComboBox
        setColour(juce::ComboBox::backgroundColourId,    c(GrisOscuro));
        setColour(juce::ComboBox::textColourId,          c(Crema));
        setColour(juce::ComboBox::outlineColourId,       c(GrisMedio));
        setColour(juce::ComboBox::buttonColourId,        c(VerdeMosgo));
        setColour(juce::ComboBox::arrowColourId,         c(CremaOscuro));

        // PopupMenu
        setColour(juce::PopupMenu::backgroundColourId,       c(NegroSuave));
        setColour(juce::PopupMenu::textColourId,             c(Crema));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, c(VerdeMosgo));
        setColour(juce::PopupMenu::highlightedTextColourId,  c(CremaClaro));

        // TabbedComponent
        setColour(juce::TabbedButtonBar::tabOutlineColourId,        c(GrisMedio));
        setColour(juce::TabbedButtonBar::frontOutlineColourId,      c(VerdeMosgo));
        setColour(juce::TabbedButtonBar::tabTextColourId,           c(CremaOscuro));
        setColour(juce::TabbedButtonBar::frontTextColourId,         c(CremaClaro));

        // ListBox / TableListBox
        setColour(juce::ListBox::backgroundColourId,          c(NegroSuave));
        setColour(juce::ListBox::outlineColourId,             c(GrisMedio));
        setColour(juce::ListBox::textColourId,                c(Crema));

        // ScrollBar
        setColour(juce::ScrollBar::thumbColourId,        c(GrisMedio));
        setColour(juce::ScrollBar::trackColourId,        c(Negro));

        // AlertWindow
        setColour(juce::AlertWindow::backgroundColourId, c(NegroSuave));
        setColour(juce::AlertWindow::textColourId,       c(Crema));
        setColour(juce::AlertWindow::outlineColourId,    c(GrisMedio));

        // TextEditor
        setColour(juce::TextEditor::backgroundColourId,  c(GrisOscuro));
        setColour(juce::TextEditor::textColourId,        c(Crema));
        setColour(juce::TextEditor::outlineColourId,     c(VerdeMosgo));
        setColour(juce::TextEditor::highlightColourId,   c(VerdeMosgo));
        setColour(juce::TextEditor::highlightedTextColourId, c(CremaClaro));
        setColour(juce::TextEditor::shadowColourId,      c(Negro));
    }

    // ---- Button background ------------------------------------------------
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& /*bgColour*/,
                              bool highlighted, bool down) override
    {
        using namespace InUtero;
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        bool toggled = button.getToggleState();

        juce::Colour fill;
        if      (down)      fill = c(VerdeMosgo).darker(0.3f);
        else if (highlighted) fill = c(toggled ? VerdeClaro : GrisMedio);
        else if (toggled)   fill = c(VerdeMosgo);
        else                fill = button.findColour(juce::TextButton::buttonColourId);

        g.setColour(fill);
        g.fillRoundedRectangle(bounds, 3.0f);

        // Border
        g.setColour(toggled ? c(VerdeClaro) : c(GrisMedio));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        // Accent line at top for active buttons
        if (toggled)
        {
            g.setColour(c(VerdeClaro).withAlpha(0.8f));
            g.fillRect(bounds.removeFromTop(2.0f).reduced(2.0f, 0.0f));
        }
    }

    // ---- Button text -------------------------------------------------------
    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool /*highlighted*/, bool /*down*/) override
    {
        using namespace InUtero;
        g.setFont(getTextButtonFont(button, button.getHeight()));
        bool toggled = button.getToggleState();
        g.setColour(button.findColour(toggled ? juce::TextButton::textColourOnId
                                              : juce::TextButton::textColourOffId)
                    .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.4f));
        g.drawText(button.getButtonText(), button.getLocalBounds(),
                   juce::Justification::centred, true);
    }

    // ---- Linear Slider ----------------------------------------------------
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                          const juce::Slider::SliderStyle /*style*/,
                          juce::Slider& slider) override
    {
        using namespace InUtero;

        if (slider.isHorizontal())
        {
            float trackH = 5.0f;
            float cy     = (float)y + (float)height * 0.5f - trackH * 0.5f;

            // Track bg (visible dark groove)
            g.setColour(c(Negro));
            g.fillRoundedRectangle((float)x, cy, (float)width, trackH, 2.5f);
            g.setColour(c(GrisMedio));
            g.drawRoundedRectangle((float)x, cy, (float)width, trackH, 2.5f, 1.0f);

            // Fill (green-moss to where thumb is)
            g.setColour(slider.findColour(juce::Slider::thumbColourId).withAlpha(0.8f));
            g.fillRoundedRectangle((float)x, cy, sliderPos - (float)x, trackH, 2.5f);

            // Thumb
            float thumbR = 7.0f;
            float tx     = sliderPos - thumbR;
            float ty     = (float)y + (float)height * 0.5f - thumbR;
            g.setColour(c(VerdeClaro));
            g.fillEllipse(tx, ty, thumbR * 2.0f, thumbR * 2.0f);
            g.setColour(c(GrisMedio));
            g.drawEllipse(tx, ty, thumbR * 2.0f, thumbR * 2.0f, 1.0f);
        }
        else  // Vertical (mixer fader)
        {
            float trackW = 5.0f;
            float cx     = (float)x + (float)width * 0.5f - trackW * 0.5f;

            g.setColour(c(Negro));
            g.fillRoundedRectangle(cx, (float)y, trackW, (float)height, 2.5f);
            g.setColour(c(GrisMedio));
            g.drawRoundedRectangle(cx, (float)y, trackW, (float)height, 2.5f, 1.0f);

            g.setColour(slider.findColour(juce::Slider::thumbColourId).withAlpha(0.8f));
            g.fillRoundedRectangle(cx, sliderPos, trackW, (float)y + (float)height - sliderPos, 2.5f);

            // Fader cap
            float capW = 22.0f, capH = 10.0f;
            float capX = (float)x + (float)width * 0.5f - capW * 0.5f;
            float capY = sliderPos - capH * 0.5f;
            g.setColour(c(CremaOscuro));
            g.fillRoundedRectangle(capX, capY, capW, capH, 2.0f);
            g.setColour(c(GrisMedio));
            g.drawRoundedRectangle(capX, capY, capW, capH, 2.0f, 1.0f);
            // Center line on cap
            g.setColour(c(GrisOscuro));
            g.drawHorizontalLine((int)(capY + capH * 0.5f), capX + 4.0f, capX + capW - 4.0f);
        }
    }

    // ---- Rotary Slider (knob) -----------------------------------------------
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        using namespace InUtero;

        float radius    = (float)std::min(width, height) * 0.5f - 4.0f;
        float centreX   = (float)x + (float)width  * 0.5f;
        float centreY   = (float)y + (float)height * 0.5f;
        float angle     = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Outer track ring
        juce::Path outerRing;
        outerRing.addArc(centreX - radius, centreY - radius,
                         radius * 2.0f, radius * 2.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(c(GrisOscuro));
        g.strokePath(outerRing, juce::PathStrokeType(3.0f));

        // Filled arc up to thumb
        juce::Path filledArc;
        filledArc.addArc(centreX - radius, centreY - radius,
                         radius * 2.0f, radius * 2.0f,
                         rotaryStartAngle, angle, true);
        g.setColour(c(VerdeMosgo));
        g.strokePath(filledArc, juce::PathStrokeType(3.0f));

        // Knob body
        float knobR = radius - 5.0f;
        g.setColour(c(GrisOscuro));
        g.fillEllipse(centreX - knobR, centreY - knobR, knobR * 2.0f, knobR * 2.0f);
        g.setColour(c(GrisMedio));
        g.drawEllipse(centreX - knobR, centreY - knobR, knobR * 2.0f, knobR * 2.0f, 1.0f);

        // Pointer line
        juce::Path pointer;
        float pointerLen = knobR - 2.0f;
        pointer.addLineSegment(
            juce::Line<float>(centreX, centreY,
                              centreX + std::sin(angle) * pointerLen,
                              centreY - std::cos(angle) * pointerLen), 2.0f);
        g.setColour(slider.isEnabled() ? c(VerdeClaro) : c(GrisClaro));
        g.fillPath(pointer);
    }

    // ---- Tab button ----------------------------------------------------------
    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g,
                       bool isMouseOver, bool isMouseDown) override
    {
        using namespace InUtero;
        auto area = button.getActiveArea();
        bool active = button.isFrontTab();

        g.setColour(active ? c(GrisOscuro) : c(Negro));
        g.fillRect(area);

        // Active tab: green-moss accent line at top
        if (active)
        {
            g.setColour(c(VerdeMosgo));
            g.fillRect(area.removeFromTop(2));
        }
        else if (isMouseOver)
        {
            g.setColour(c(GrisMedio).withAlpha(0.5f));
            g.fillRect(area);
        }

        // Divider
        g.setColour(c(GrisMedio));
        g.drawVerticalLine(button.getWidth() - 1, 0.0f, (float)button.getHeight());

        // Tab text
        g.setColour(active ? c(CremaClaro) : c(GrisClaro));
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.5f).withStyle("Bold")));
        g.drawText(button.getButtonText(), button.getLocalBounds(),
                   juce::Justification::centred, true);
    }
};
