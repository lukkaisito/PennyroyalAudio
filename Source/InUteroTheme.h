#pragma once
#include <JuceHeader.h>
#include "BinaryData.h"
#include "Lang.h"

// ============================================================
//  Colour palette (runtime, switchable themes)
//  The names come from the original "In Utero" palette; every theme
//  fills the same slots, so all drawing code works with any theme.
// ============================================================
namespace InUtero
{
    inline juce::uint32 Negro       = 0xFF0D0D0D;   // main background
    inline juce::uint32 NegroSuave  = 0xFF1A1A1A;   // surfaces
    inline juce::uint32 GrisOscuro  = 0xFF2A2A2A;   // controls
    inline juce::uint32 GrisMedio   = 0xFF3D3D3D;   // outlines
    inline juce::uint32 GrisClaro   = 0xFF6B6B6B;   // secondary text
    inline juce::uint32 VerdeMosgo  = 0xFF4A5240;
    inline juce::uint32 VerdeClaro  = 0xFF6B7A5A;
    inline juce::uint32 Crema       = 0xFFE8DCC8;   // text
    inline juce::uint32 CremaClaro  = 0xFFF0EBE0;   // bright text
    inline juce::uint32 CremaOscuro = 0xFFC8BAA0;   // dim text
    inline juce::uint32 Rojo        = 0xFF8B1A1A;
    inline juce::uint32 RojoClaro   = 0xFFB02020;
    inline juce::uint32 Ocre        = 0xFF7A6840;

    inline juce::uint32 Panel       = 0xFF141414;   // headers / bars
    inline juce::uint32 Linea       = 0xFF262626;   // separators
    inline juce::uint32 Acento      = 0xFFD47A4A;   // accent
    inline juce::uint32 AcentoClaro = 0xFFE89A6C;
    inline juce::uint32 Medidor     = 0xFF8FA372;   // meter green
    inline juce::uint32 LaneA       = 0xFF101010;   // arrangement lanes
    inline juce::uint32 LaneB       = 0xFF0D0D0D;
    inline juce::uint32 Seleccion   = 0xFF1D1D1D;   // selected header / strip
    inline juce::uint32 GridBar     = 0xFF2A2A2A;
    inline juce::uint32 GridBeat    = 0xFF181818;

    static inline juce::Colour c(juce::uint32 h) { return juce::Colour(h); }
}

// ============================================================
//  Themes
// ============================================================
namespace Themes
{
    enum Id { InUteroTheme_ = 0, Catppuccin = 1, StudioGrey = 2, Oled = 3 };

    inline juce::StringArray names() { return { "In Utero", "Catppuccin Frappe", "Studio Grey", "OLED Black" }; }

    // Accent presets for the OLED theme
    inline const juce::uint32 oledPresets[] = {
        0xFFD47A4A, 0xFFE0453A, 0xFFF06292, 0xFFB388FF, 0xFF4FA3FF,
        0xFF32D6D6, 0xFF5BE07A, 0xFFF2D35B, 0xFFE8E8E8 };

    inline int          current    = 0;
    inline juce::uint32 oledAccent = 0xFFD47A4A;

    inline juce::uint32 mix(juce::uint32 a, juce::uint32 b, float t)
    {
        return juce::Colour(a).interpolatedWith(juce::Colour(b), t).getARGB();
    }

    inline void apply(int id, juce::uint32 accent)
    {
        using namespace InUtero;
        current    = juce::jlimit(0, 3, id);
        oledAccent = accent;

        switch (current)
        {
            case Catppuccin:   // Catppuccin Frappe
                Negro = 0xFF232634; NegroSuave = 0xFF303446; GrisOscuro = 0xFF414559; GrisMedio = 0xFF51576D;
                GrisClaro = 0xFF838BA7; VerdeMosgo = 0xFF81C8BE; VerdeClaro = 0xFFA6D189;
                Crema = 0xFFC6D0F5; CremaClaro = 0xFFEFF1FB; CremaOscuro = 0xFFA5ADCE;
                Rojo = 0xFFB0606A; RojoClaro = 0xFFE78284; Ocre = 0xFFE5C890;
                Panel = 0xFF292C3C; Linea = 0xFF3B3F53; Acento = 0xFFCA9EE6; AcentoClaro = 0xFFBABBF1;
                Medidor = 0xFFA6D189; LaneA = 0xFF2A2D3E; LaneB = 0xFF262939; Seleccion = 0xFF3A3F55;
                GridBar = 0xFF464B62; GridBeat = 0xFF323648;
                break;

            case StudioGrey:   // mid grey, orange + green (FL Studio style)
                Negro = 0xFF33393C; NegroSuave = 0xFF454E52; GrisOscuro = 0xFF4E585C; GrisMedio = 0xFF66727A;
                GrisClaro = 0xFF9DA9AE; VerdeMosgo = 0xFF6A8F4E; VerdeClaro = 0xFF8DB86A;
                Crema = 0xFFDCE3E6; CremaClaro = 0xFFF6F8F9; CremaOscuro = 0xFFB8C3C8;
                Rojo = 0xFFA8363A; RojoClaro = 0xFFE5534B; Ocre = 0xFFC9A048;
                Panel = 0xFF3C4448; Linea = 0xFF2A3134; Acento = 0xFFF2A13B; AcentoClaro = 0xFFF7BC6A;
                Medidor = 0xFF9BD64B; LaneA = 0xFF3A4245; LaneB = 0xFF363E41; Seleccion = 0xFF4C565B;
                GridBar = 0xFF262D30; GridBeat = 0xFF30373A;
                break;

            case Oled:         // pure black, outlines in the user colour
                Negro = 0xFF000000; NegroSuave = 0xFF080808; GrisOscuro = 0xFF121212;
                GrisMedio = mix(0xFF000000, accent, 0.70f);
                GrisClaro = 0xFF8A8A8A; VerdeMosgo = mix(0xFF000000, accent, 0.5f); VerdeClaro = accent;
                Crema = 0xFFE6E6E6; CremaClaro = 0xFFFFFFFF; CremaOscuro = 0xFFB4B4B4;
                Rojo = 0xFF8E1F1F; RojoClaro = 0xFFE5403A; Ocre = mix(0xFF000000, accent, 0.8f);
                Panel = 0xFF000000; Linea = mix(0xFF000000, accent, 0.40f);
                Acento = accent; AcentoClaro = juce::Colour(accent).brighter(0.3f).getARGB();
                Medidor = 0xFF5BE07A; LaneA = 0xFF040404; LaneB = 0xFF000000;
                Seleccion = mix(0xFF000000, accent, 0.12f);
                GridBar = mix(0xFF000000, accent, 0.28f); GridBeat = 0xFF101010;
                break;

            default:           // In Utero
                Negro = 0xFF0D0D0D; NegroSuave = 0xFF1A1A1A; GrisOscuro = 0xFF2A2A2A; GrisMedio = 0xFF3D3D3D;
                GrisClaro = 0xFF6B6B6B; VerdeMosgo = 0xFF4A5240; VerdeClaro = 0xFF6B7A5A;
                Crema = 0xFFE8DCC8; CremaClaro = 0xFFF0EBE0; CremaOscuro = 0xFFC8BAA0;
                Rojo = 0xFF8B1A1A; RojoClaro = 0xFFB02020; Ocre = 0xFF7A6840;
                Panel = 0xFF141414; Linea = 0xFF262626; Acento = 0xFFD47A4A; AcentoClaro = 0xFFE89A6C;
                Medidor = 0xFF8FA372; LaneA = 0xFF101010; LaneB = 0xFF0D0D0D; Seleccion = 0xFF1D1D1D;
                GridBar = 0xFF2A2A2A; GridBeat = 0xFF181818;
                break;
        }
    }
}

// ============================================================
//  Embedded fonts (all open-source, see Resources/Fonts/licenses)
//    Bebas Neue     -> titles, tabs        (same as the website)
//    Barlow         -> general UI text
//    Space Mono     -> numbers / time display
//    Special Elite  -> grunge typewriter accents (splash)
// ============================================================
namespace PRFonts
{
    inline juce::Typeface::Ptr load(const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor(data, (size_t)size);
    }
    inline juce::Typeface::Ptr bebas()      { static auto t = load(BinaryData::BebasNeueRegular_ttf,   BinaryData::BebasNeueRegular_ttfSize);   return t; }
    inline juce::Typeface::Ptr barlow()     { static auto t = load(BinaryData::BarlowRegular_ttf,      BinaryData::BarlowRegular_ttfSize);      return t; }
    inline juce::Typeface::Ptr barlowSemi() { static auto t = load(BinaryData::BarlowSemiBold_ttf,     BinaryData::BarlowSemiBold_ttfSize);     return t; }
    inline juce::Typeface::Ptr mono()       { static auto t = load(BinaryData::SpaceMonoRegular_ttf,   BinaryData::SpaceMonoRegular_ttfSize);   return t; }
    inline juce::Typeface::Ptr monoBold()   { static auto t = load(BinaryData::SpaceMonoBold_ttf,      BinaryData::SpaceMonoBold_ttfSize);      return t; }
    inline juce::Typeface::Ptr grunge()     { static auto t = load(BinaryData::SpecialEliteRegular_ttf,BinaryData::SpecialEliteRegular_ttfSize);return t; }

    inline juce::Font title   (float h) { return juce::Font(juce::FontOptions(bebas()).withHeight(h)); }
    inline juce::Font body    (float h) { return juce::Font(juce::FontOptions(barlow()).withHeight(h)); }
    inline juce::Font bodyBold(float h) { return juce::Font(juce::FontOptions(barlowSemi()).withHeight(h)); }
    inline juce::Font num     (float h) { return juce::Font(juce::FontOptions(mono()).withHeight(h)); }
    inline juce::Font numBold (float h) { return juce::Font(juce::FontOptions(monoBold()).withHeight(h)); }
    inline juce::Font typewriter(float h) { return juce::Font(juce::FontOptions(grunge()).withHeight(h)); }
}

// ============================================================
//  InUteroTheme  --  custom JUCE LookAndFeel
// ============================================================
class InUteroTheme : public juce::LookAndFeel_V4
{
public:
    InUteroTheme()
    {
        setDefaultSansSerifTypeface(PRFonts::barlow());
        refreshColours();
    }

    // Re-read the palette (call after Themes::apply)
    void refreshColours()
    {
        using namespace InUtero;

        setColour(juce::ResizableWindow::backgroundColourId, c(Negro));
        setColour(juce::DocumentWindow::backgroundColourId,  c(Negro));

        setColour(juce::TextButton::buttonColourId,   c(GrisOscuro));
        setColour(juce::TextButton::buttonOnColourId, c(Acento));
        setColour(juce::TextButton::textColourOffId,  c(CremaOscuro));
        setColour(juce::TextButton::textColourOnId,   c(Negro));

        setColour(juce::Label::textColourId,       c(Crema));
        setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

        setColour(juce::Slider::thumbColourId,               c(Crema));
        setColour(juce::Slider::trackColourId,               c(Acento));
        setColour(juce::Slider::backgroundColourId,          c(GrisOscuro));
        setColour(juce::Slider::rotarySliderFillColourId,    c(Acento));
        setColour(juce::Slider::rotarySliderOutlineColourId, c(GrisOscuro));
        setColour(juce::Slider::textBoxTextColourId,         c(CremaOscuro));
        setColour(juce::Slider::textBoxBackgroundColourId,   c(NegroSuave));
        setColour(juce::Slider::textBoxOutlineColourId,      c(GrisMedio));

        setColour(juce::ComboBox::backgroundColourId, c(NegroSuave));
        setColour(juce::ComboBox::textColourId,       c(Crema));
        setColour(juce::ComboBox::outlineColourId,    c(GrisMedio));
        setColour(juce::ComboBox::buttonColourId,     c(GrisOscuro));
        setColour(juce::ComboBox::arrowColourId,      c(CremaOscuro));

        setColour(juce::PopupMenu::backgroundColourId,            c(NegroSuave));
        setColour(juce::PopupMenu::textColourId,                  c(Crema));
        setColour(juce::PopupMenu::headerTextColourId,            c(GrisClaro));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, c(Acento));
        setColour(juce::PopupMenu::highlightedTextColourId,       c(Negro));

        setColour(juce::TabbedComponent::backgroundColourId, c(Negro));
        setColour(juce::TabbedComponent::outlineColourId,    juce::Colours::transparentBlack);
        setColour(juce::TabbedButtonBar::tabOutlineColourId,   juce::Colours::transparentBlack);
        setColour(juce::TabbedButtonBar::frontOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::TabbedButtonBar::tabTextColourId,      c(GrisClaro));
        setColour(juce::TabbedButtonBar::frontTextColourId,    c(CremaClaro));

        setColour(juce::ListBox::backgroundColourId, c(Panel));
        setColour(juce::ListBox::outlineColourId,    c(Linea));
        setColour(juce::ListBox::textColourId,       c(Crema));

        setColour(juce::ScrollBar::thumbColourId, c(GrisMedio));
        setColour(juce::ScrollBar::trackColourId, juce::Colours::transparentBlack);

        setColour(juce::AlertWindow::backgroundColourId, c(NegroSuave));
        setColour(juce::AlertWindow::textColourId,       c(Crema));
        setColour(juce::AlertWindow::outlineColourId,    c(GrisMedio));

        setColour(juce::TextEditor::backgroundColourId,      c(Negro));
        setColour(juce::TextEditor::textColourId,            c(Crema));
        setColour(juce::TextEditor::outlineColourId,         c(GrisMedio));
        setColour(juce::TextEditor::focusedOutlineColourId,  c(Acento));
        setColour(juce::TextEditor::highlightColourId,       c(Acento).withAlpha(0.4f));
        setColour(juce::TextEditor::highlightedTextColourId, c(CremaClaro));
        setColour(juce::CaretComponent::caretColourId,       c(Acento));

        setColour(juce::TooltipWindow::backgroundColourId, c(Crema));
        setColour(juce::TooltipWindow::textColourId,       c(Negro));
        setColour(juce::TooltipWindow::outlineColourId,    c(Crema));

        setColour(juce::ToggleButton::textColourId,         c(Crema));
        setColour(juce::ToggleButton::tickColourId,         c(Acento));
        setColour(juce::ToggleButton::tickDisabledColourId, c(GrisClaro));

        setColour(juce::ResizableWindow::backgroundColourId, c(Negro));
    }

    // Bold requests on the default font -> Barlow SemiBold
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& f) override
    {
        if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
            return f.isBold() ? PRFonts::barlowSemi() : PRFonts::barlow();
        return juce::LookAndFeel_V4::getTypefaceForFont(f);
    }

    // ---- Buttons --------------------------------------------------------
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& bg, bool highlighted, bool down) override
    {
        using namespace InUtero;
        auto b = button.getLocalBounds().toFloat().reduced(0.5f);
        const bool on = button.getToggleState();
        auto fill = on ? button.findColour(juce::TextButton::buttonOnColourId) : bg;

        if (!button.isEnabled())   fill = fill.withMultipliedAlpha(0.4f);
        else if (down)             fill = fill.brighter(0.15f);
        else if (highlighted)      fill = fill.brighter(on ? 0.12f : 0.08f);

        g.setColour(fill);
        g.fillRoundedRectangle(b, 4.0f);
        g.setColour(on ? fill.brighter(0.2f) : c(GrisMedio).withAlpha(highlighted ? 1.0f : 0.7f));
        g.drawRoundedRectangle(b, 4.0f, 1.0f);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int h) override
    {
        return PRFonts::bodyBold(juce::jlimit(8.0f, 13.0f, (float)h * 0.55f));
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        auto font = getTextButtonFont(b, b.getHeight());
        g.setFont(font);
        auto col = b.findColour(b.getToggleState() ? juce::TextButton::textColourOnId
                                                   : juce::TextButton::textColourOffId);
        g.setColour(b.isEnabled() ? col : col.withMultipliedAlpha(0.4f));
        g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(4, 0),
                         juce::Justification::centred, 1);
    }

    // ---- Rotary knobs ---------------------------------------------------
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startA, float endA, juce::Slider& s) override
    {
        using namespace InUtero;
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(3.0f);
        const float r  = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto  cc = bounds.getCentre();
        const float trackW = juce::jmax(2.0f, r * 0.14f);
        const float arcR   = r - trackW * 0.5f;
        const float angle  = startA + pos * (endA - startA);

        // track
        juce::Path bgArc;
        bgArc.addCentredArc(cc.x, cc.y, arcR, arcR, 0.0f, startA, endA, true);
        g.setColour(c(GrisOscuro));
        g.strokePath(bgArc, juce::PathStrokeType(trackW, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        // value arc (from centre for bipolar controls)
        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float from   = bipolar ? (startA + endA) * 0.5f : startA;
        if (std::abs(angle - from) > 0.01f)
        {
            juce::Path valArc;
            valArc.addCentredArc(cc.x, cc.y, arcR, arcR, 0.0f,
                                 juce::jmin(from, angle), juce::jmax(from, angle), true);
            g.setColour(s.findColour(juce::Slider::rotarySliderFillColourId)
                          .withMultipliedAlpha(s.isEnabled() ? 1.0f : 0.4f));
            g.strokePath(valArc, juce::PathStrokeType(trackW, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }

        // knob body
        const float kr = arcR - trackW * 1.3f;
        juce::ColourGradient grad(c(GrisMedio), cc.x, cc.y - kr, c(NegroSuave), cc.x, cc.y + kr, false);
        g.setGradientFill(grad);
        g.fillEllipse(cc.x - kr, cc.y - kr, kr * 2.0f, kr * 2.0f);
        g.setColour(c(Negro));
        g.drawEllipse(cc.x - kr, cc.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);

        // pointer
        juce::Path p;
        p.addRoundedRectangle(-1.2f, -kr + 2.0f, 2.4f, kr * 0.55f, 1.0f);
        g.setColour(c(CremaClaro));
        g.fillPath(p, juce::AffineTransform::rotation(angle).translated(cc.x, cc.y));
    }

    // ---- Linear sliders / faders ----------------------------------------
    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float, float, juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        using namespace InUtero;
        auto area = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);

        if (style == juce::Slider::LinearBarVertical || style == juce::Slider::LinearBar)
        {
            // value box (e.g. BPM): draggable number
            auto b = s.getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(c(Negro));
            g.fillRoundedRectangle(b, 4.0f);
            g.setColour(s.isMouseOverOrDragging() ? c(Acento) : c(GrisMedio));
            g.drawRoundedRectangle(b, 4.0f, 1.0f);
            g.setColour(c(CremaClaro));
            g.setFont(PRFonts::numBold(juce::jlimit(8.0f, 16.0f, b.getHeight() * 0.62f)));
            g.drawText(juce::String(s.getValue(), s.getNumDecimalPlacesToDisplay()),
                       b, juce::Justification::centred);
            return;
        }

        if (style == juce::Slider::LinearVertical)
        {
            // Mixer fader: groove + scale + cap
            const float cx = area.getCentreX();
            g.setColour(c(Negro));
            g.fillRoundedRectangle(cx - 2.5f, area.getY(), 5.0f, area.getHeight(), 2.5f);
            g.setColour(c(Linea));
            for (int i = 0; i <= 10; ++i)
            {
                const float ty = area.getY() + area.getHeight() * (float)i / 10.0f;
                const float tw = (i % 5 == 0) ? 9.0f : 5.0f;
                g.drawHorizontalLine((int)ty, cx - 8.0f - tw, cx - 8.0f);
            }
            auto cap = juce::Rectangle<float>(28.0f, 16.0f).withCentre({ cx, pos });
            juce::ColourGradient cg(c(GrisClaro), cap.getX(), cap.getY(), c(GrisOscuro),
                                    cap.getX(), cap.getBottom(), false);
            g.setGradientFill(cg);
            g.fillRoundedRectangle(cap, 3.0f);
            g.setColour(c(Negro));
            g.drawRoundedRectangle(cap, 3.0f, 1.0f);
            g.setColour(c(CremaClaro));
            g.fillRect(cap.getX() + 4.0f, cap.getCentreY() - 0.75f, cap.getWidth() - 8.0f, 1.5f);
            return;
        }

        // Horizontal: thin track, fill from the left (or from centre for pan)
        const float cy = area.getCentreY();
        auto track = juce::Rectangle<float>(area.getX(), cy - 1.5f, area.getWidth(), 3.0f);
        g.setColour(c(GrisOscuro));
        g.fillRoundedRectangle(track, 1.5f);

        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float from   = bipolar ? area.getCentreX() : area.getX();
        g.setColour(s.findColour(juce::Slider::trackColourId)
                      .withMultipliedAlpha(s.isEnabled() ? 1.0f : 0.4f));
        g.fillRoundedRectangle(juce::jmin(from, pos), cy - 1.5f, std::abs(pos - from), 3.0f, 1.5f);

        const float tr = s.isMouseOverOrDragging() ? 6.0f : 5.0f;
        g.setColour(c(CremaClaro));
        g.fillEllipse(pos - tr, cy - tr, tr * 2.0f, tr * 2.0f);
        g.setColour(c(Negro));
        g.drawEllipse(pos - tr, cy - tr, tr * 2.0f, tr * 2.0f, 1.0f);
    }

    // ---- Tabs -------------------------------------------------------------
    int getTabButtonBestWidth(juce::TabBarButton& b, int depth) override
    {
        auto f = PRFonts::title(juce::jmax(10.0f, (float)depth * 0.62f));
        return (int)juce::GlyphArrangement::getStringWidth(f, b.getButtonText()) + 34;
    }

    void drawTabButton(juce::TabBarButton& b, juce::Graphics& g, bool over, bool) override
    {
        using namespace InUtero;
        auto area = b.getActiveArea().toFloat();
        const bool front = b.isFrontTab();

        if (front)
        {
            g.setColour(c(Acento));
            g.fillRect(area.removeFromBottom(2.0f).reduced(10.0f, 0.0f));
        }
        g.setColour(front ? c(CremaClaro) : (over ? c(CremaOscuro) : c(GrisClaro)));
        g.setFont(PRFonts::title(juce::jmax(10.0f, (float)b.getHeight() * 0.62f)));
        g.drawText(b.getButtonText(), b.getActiveArea(), juce::Justification::centred);
    }

    void drawTabbedButtonBarBackground(juce::TabbedButtonBar&, juce::Graphics&) override {}

    void drawTabAreaBehindFrontButton(juce::TabbedButtonBar& bar, juce::Graphics& g, int w, int h) override
    {
        using namespace InUtero;
        juce::ignoreUnused(bar);
        g.setColour(c(Linea));
        g.fillRect(0, h - 1, w, 1);
    }

    // ---- ComboBox ---------------------------------------------------------
    void drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        using namespace InUtero;
        auto b = juce::Rectangle<float>(0, 0, (float)w, (float)h).reduced(0.5f);
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(b, 4.0f);
        g.setColour(box.hasKeyboardFocus(true) || box.isMouseOver(true) ? c(Acento) : c(GrisMedio));
        g.drawRoundedRectangle(b, 4.0f, 1.0f);

        juce::Path arrow;
        const float ax = (float)w - 14.0f, ay = (float)h * 0.5f;
        arrow.startNewSubPath(ax - 4.0f, ay - 2.0f);
        arrow.lineTo(ax, ay + 2.5f);
        arrow.lineTo(ax + 4.0f, ay - 2.0f);
        g.setColour(c(CremaOscuro));
        g.strokePath(arrow, juce::PathStrokeType(1.5f));
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override { return PRFonts::body(14.0f); }
    juce::Font getPopupMenuFont() override               { return PRFonts::body(15.0f); }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(8, 1, box.getWidth() - 26, box.getHeight() - 2);
        label.setFont(getComboBoxFont(box));
    }

    void drawPopupMenuBackground(juce::Graphics& g, int w, int h) override
    {
        using namespace InUtero;
        g.fillAll(c(NegroSuave));
        g.setColour(c(GrisMedio));
        g.drawRect(0, 0, w, h);
    }

    // ---- Scrollbars -------------------------------------------------------
    int getDefaultScrollbarWidth() override { return 10; }

    void drawScrollbar(juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h,
                       bool vertical, int thumbStart, int thumbSize, bool over, bool down) override
    {
        using namespace InUtero;
        juce::Rectangle<float> thumb = vertical
            ? juce::Rectangle<float>((float)x + 2.0f, (float)thumbStart, (float)w - 4.0f, (float)thumbSize)
            : juce::Rectangle<float>((float)thumbStart, (float)y + 2.0f, (float)thumbSize, (float)h - 4.0f);
        g.setColour(c(down ? GrisClaro : (over ? GrisMedio : GrisOscuro)));
        g.fillRoundedRectangle(thumb, 3.0f);
    }

    // ---- Tooltips ---------------------------------------------------------
    juce::Rectangle<int> getTooltipBounds(const juce::String& tip, juce::Point<int> pos,
                                          juce::Rectangle<int> parent) override
    {
        const int w = (int)juce::GlyphArrangement::getStringWidth(PRFonts::body(14.0f), tip) + 18;
        const int h = 24;
        return juce::Rectangle<int>(pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 12,
                                    pos.y > parent.getCentreY() ? pos.y - (h + 6)  : pos.y + 6,
                                    w, h).constrainedWithin(parent);
    }

    void drawTooltip(juce::Graphics& g, const juce::String& text, int w, int h) override
    {
        using namespace InUtero;
        g.setColour(c(Crema));
        g.fillRoundedRectangle(0, 0, (float)w, (float)h, 3.0f);
        g.setColour(c(Negro));
        g.setFont(PRFonts::body(14.0f));
        g.drawText(text, 0, 0, w, h, juce::Justification::centred);
    }

    // ---- Alert windows ----------------------------------------------------
    juce::Font getAlertWindowTitleFont()   override { return PRFonts::title(26.0f); }
    juce::Font getAlertWindowMessageFont() override { return PRFonts::body(15.0f); }
    juce::Font getAlertWindowFont()        override { return PRFonts::body(14.0f); }

    juce::Font getLabelFont(juce::Label& l) override
    {
        return l.getFont();
    }
};
