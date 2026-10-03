#pragma once
#include <JuceHeader.h>
#include "InUteroTheme.h"

// ============================================================
//  Small reusable UI pieces
// ============================================================
namespace UI
{
    using namespace InUtero;

    // Vertical level meter (dB scale -60..+6), with green / ochre / red zones
    inline void drawMeter(juce::Graphics& g, juce::Rectangle<float> r, float level)
    {
        g.setColour(c(Negro));
        g.fillRoundedRectangle(r, 1.5f);

        const float db   = juce::Decibels::gainToDecibels(level, -60.0f);
        const float prop = juce::jlimit(0.0f, 1.0f, juce::jmap(db, -60.0f, 6.0f, 0.0f, 1.0f));
        if (prop <= 0.0f) return;

        auto fill = r.withTop(r.getBottom() - r.getHeight() * prop);
        juce::ColourGradient grad(c(Medidor), 0.0f, r.getBottom(), c(RojoClaro), 0.0f, r.getY(), false);
        const float p0 = juce::jmap(-12.0f, -60.0f, 6.0f, 0.0f, 1.0f);
        const float p1 = juce::jmap(  0.0f, -60.0f, 6.0f, 0.0f, 1.0f);
        grad.addColour(p0, c(Medidor));
        grad.addColour(juce::jmin(0.99f, p0 + 0.08f), c(0xFFD1B45A));
        grad.addColour(p1, c(Acento));
        g.setGradientFill(grad);
        g.fillRoundedRectangle(fill, 1.5f);
    }

    // Horizontal thin meter (for track headers)
    inline void drawMeterH(juce::Graphics& g, juce::Rectangle<float> r, float level)
    {
        g.setColour(c(Negro));
        g.fillRect(r);
        const float db   = juce::Decibels::gainToDecibels(level, -60.0f);
        const float prop = juce::jlimit(0.0f, 1.0f, juce::jmap(db, -60.0f, 6.0f, 0.0f, 1.0f));
        if (prop <= 0.0f) return;
        juce::ColourGradient grad(c(Medidor), r.getX(), 0.0f, c(RojoClaro), r.getRight(), 0.0f, false);
        grad.addColour(juce::jmap(-12.0f, -60.0f, 6.0f, 0.0f, 1.0f), c(Medidor));
        grad.addColour(juce::jmap(  0.0f, -60.0f, 6.0f, 0.0f, 1.0f), c(Acento));
        g.setGradientFill(grad);
        g.fillRect(r.withWidth(r.getWidth() * prop));
    }

    // Section caption like "MIXER", "EXPORT"
    inline void drawSectionTitle(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title,
                                 const juce::String& subtitle = {})
    {
        g.setColour(c(CremaClaro));
        g.setFont(PRFonts::title(28.0f));
        g.drawText(title, r, juce::Justification::centredLeft);
        if (subtitle.isNotEmpty())
        {
            const int tw = (int)juce::GlyphArrangement::getStringWidth(PRFonts::title(28.0f), title);
            g.setColour(c(GrisClaro));
            g.setFont(PRFonts::body(14.0f));
            g.drawText(subtitle, r.withTrimmedLeft(tw + 14), juce::Justification::centredLeft);
        }
    }

    // Small all-caps label ("PAN", "VOL" ...)
    inline void drawCaption(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text,
                            juce::Justification j = juce::Justification::centred,
                            juce::Colour col = c(GrisClaro))
    {
        g.setColour(col);
        g.setFont(PRFonts::bodyBold(11.0f));
        g.drawText(text, r, j);
    }

    // Upper-case that also handles accented Latin letters (a with acute -> A with acute,
    // n with tilde -> N with tilde...) the same way on every OS / locale.
    inline juce::String upper(const juce::String& s)
    {
        juce::String out;
        out.preallocateBytes(s.getNumBytesAsUTF8() + 4);
        for (auto p = s.getCharPointer(); !p.isEmpty(); ++p)
        {
            juce::juce_wchar ch = *p;
            if (ch >= 'a' && ch <= 'z')                 ch -= 32;
            else if (ch >= 0xE0 && ch <= 0xFE && ch != 0xF7) ch -= 32;   // Latin-1 accented
            out += juce::String::charToString(ch);
        }
        return out;
    }

    inline void noFocus(std::initializer_list<juce::Component*> comps)
    {
        for (auto* comp : comps) { comp->setWantsKeyboardFocus(false); comp->setMouseClickGrabsKeyboardFocus(false); }
    }
}

// ============================================================
//  IconButton  --  transport / toolbar button with a vector icon
//  (no Unicode glyphs: every icon is drawn with juce::Path)
// ============================================================
class IconButton : public juce::Button
{
public:
    enum class Icon { Play, Pause, Stop, Record, Loop, Metronome, Monitor,
                      Undo, Redo, Open, Save, Plus, Help, Close };

    IconButton(const juce::String& name, Icon i) : juce::Button(name), icon(i)
    {
        setWantsKeyboardFocus(false);
        setMouseClickGrabsKeyboardFocus(false);
    }

    void setIcon(Icon i)                 { icon = i; repaint(); }
    void setOnColour(juce::Colour col)   { onColour = col; repaint(); }
    void setFlat(bool f)                 { flat = f; repaint(); }

    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        using namespace InUtero;
        auto b = getLocalBounds().toFloat().reduced(1.0f);
        const bool on = getToggleState();

        juce::Colour bg = on ? onColour : c(GrisOscuro);
        if (flat && !on) bg = juce::Colours::transparentBlack;
        if (down)      bg = (on ? onColour : c(GrisMedio)).brighter(0.1f);
        else if (over) bg = on ? onColour.brighter(0.15f) : c(GrisMedio).withAlpha(flat ? 0.6f : 1.0f);

        g.setColour(bg);
        g.fillRoundedRectangle(b, 5.0f);
        if (!flat || on)
        {
            g.setColour(on ? onColour.brighter(0.25f) : c(GrisMedio));
            g.drawRoundedRectangle(b, 5.0f, 1.0f);
        }

        juce::Colour ic = on ? c(Negro) : (over ? c(CremaClaro) : c(Crema));
        if (!isEnabled()) ic = ic.withAlpha(0.3f);
        if (icon == Icon::Record && !on) ic = isEnabled() ? c(RojoClaro).brighter(0.2f) : ic;

        const float s = juce::jmin(b.getWidth(), b.getHeight()) * 0.42f;
        auto r = juce::Rectangle<float>(s, s).withCentre(b.getCentre());
        drawIcon(g, r, ic);
    }

private:
    void drawIcon(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour col)
    {
        g.setColour(col);
        const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight();
        const float stroke = juce::jmax(1.4f, w * 0.12f);
        juce::Path p;

        switch (icon)
        {
            case Icon::Play:
                p.addTriangle(x + w * 0.12f, y, x + w * 0.12f, y + h, x + w, y + h * 0.5f);
                g.fillPath(p);
                break;

            case Icon::Pause:
                g.fillRoundedRectangle(x + w * 0.08f, y, w * 0.30f, h, 1.0f);
                g.fillRoundedRectangle(x + w * 0.62f, y, w * 0.30f, h, 1.0f);
                break;

            case Icon::Stop:
                g.fillRoundedRectangle(r.reduced(w * 0.06f), 2.0f);
                break;

            case Icon::Record:
                g.fillEllipse(r.reduced(w * 0.02f));
                break;

            case Icon::Loop:
            {
                auto rr = r.reduced(0.0f, h * 0.12f);
                p.addRoundedRectangle(rr.getX(), rr.getY(), rr.getWidth(), rr.getHeight(), rr.getHeight() * 0.45f);
                g.strokePath(p, juce::PathStrokeType(stroke));
                juce::Path a;
                const float ax = rr.getCentreX() + w * 0.08f, ay = rr.getY();
                a.addTriangle(ax - w * 0.16f, ay - h * 0.18f, ax - w * 0.16f, ay + h * 0.18f, ax + w * 0.08f, ay);
                g.fillPath(a);
                juce::Path b2;
                const float bx = rr.getCentreX() - w * 0.08f, by = rr.getBottom();
                b2.addTriangle(bx + w * 0.16f, by - h * 0.18f, bx + w * 0.16f, by + h * 0.18f, bx - w * 0.08f, by);
                g.fillPath(b2);
                break;
            }

            case Icon::Metronome:
                p.startNewSubPath(x + w * 0.30f, y);
                p.lineTo(x + w * 0.70f, y);
                p.lineTo(x + w * 0.95f, y + h);
                p.lineTo(x + w * 0.05f, y + h);
                p.closeSubPath();
                g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::mitered));
                g.drawLine(x + w * 0.5f, y + h * 0.85f, x + w * 0.85f, y + h * 0.1f, stroke);
                break;

            case Icon::Monitor:  // headphones
            {
                p.addCentredArc(x + w * 0.5f, y + h * 0.55f, w * 0.42f, h * 0.45f, 0.0f,
                                -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
                g.strokePath(p, juce::PathStrokeType(stroke));
                g.fillRoundedRectangle(x, y + h * 0.55f, w * 0.26f, h * 0.45f, 2.0f);
                g.fillRoundedRectangle(x + w * 0.74f, y + h * 0.55f, w * 0.26f, h * 0.45f, 2.0f);
                break;
            }

            case Icon::Undo:
            case Icon::Redo:
            {
                const bool undo = icon == Icon::Undo;
                p.addCentredArc(x + w * 0.5f, y + h * 0.6f, w * 0.38f, h * 0.36f, 0.0f,
                                undo ? -1.6f : 1.6f, undo ? 1.6f : -1.6f, true);
                g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
                juce::Path a;
                const float ax = undo ? x + w * 0.12f : x + w * 0.88f;
                const float ay = y + h * 0.6f;
                a.addTriangle(ax - w * 0.18f, ay - h * 0.05f, ax + w * 0.18f, ay - h * 0.05f, ax, ay + h * 0.25f);
                g.fillPath(a);
                break;
            }

            case Icon::Open:  // folder
                p.startNewSubPath(x, y + h * 0.15f);
                p.lineTo(x + w * 0.38f, y + h * 0.15f);
                p.lineTo(x + w * 0.5f, y + h * 0.3f);
                p.lineTo(x + w, y + h * 0.3f);
                p.lineTo(x + w, y + h * 0.9f);
                p.lineTo(x, y + h * 0.9f);
                p.closeSubPath();
                g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::mitered));
                break;

            case Icon::Save:  // floppy
                p.startNewSubPath(x, y);
                p.lineTo(x + w * 0.78f, y);
                p.lineTo(x + w, y + h * 0.22f);
                p.lineTo(x + w, y + h);
                p.lineTo(x, y + h);
                p.closeSubPath();
                g.strokePath(p, juce::PathStrokeType(stroke, juce::PathStrokeType::mitered));
                g.fillRect(x + w * 0.22f, y, w * 0.46f, h * 0.3f);
                g.fillRect(x + w * 0.2f, y + h * 0.58f, w * 0.6f, h * 0.42f);
                break;

            case Icon::Plus:
                g.fillRoundedRectangle(x + w * 0.42f, y, w * 0.16f, h, 1.0f);
                g.fillRoundedRectangle(x, y + h * 0.42f, w, h * 0.16f, 1.0f);
                break;

            case Icon::Help:
                g.drawEllipse(r.expanded(w * 0.12f), stroke * 0.9f);
                g.setFont(PRFonts::bodyBold(h * 1.15f));
                g.drawText("?", r.expanded(w * 0.12f).translated(0.0f, h * 0.02f), juce::Justification::centred);
                break;

            case Icon::Close:
                g.drawLine(x + w * 0.15f, y + h * 0.15f, x + w * 0.85f, y + h * 0.85f, stroke);
                g.drawLine(x + w * 0.85f, y + h * 0.15f, x + w * 0.15f, y + h * 0.85f, stroke);
                break;
        }
    }

    Icon icon;
    juce::Colour onColour { InUtero::c(InUtero::Acento) };
    bool flat { false };
};

// ============================================================
//  ColourSwatch  --  round colour chip (theme accent presets)
// ============================================================
class ColourSwatch : public juce::Button
{
public:
    explicit ColourSwatch(juce::uint32 col) : juce::Button("swatch"), colour(col)
    {
        setWantsKeyboardFocus(false);
        setMouseClickGrabsKeyboardFocus(false);
    }
    juce::uint32 getColour() const { return colour; }

    void paintButton(juce::Graphics& g, bool over, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced(3.0f);
        if (getToggleState())
        {
            g.setColour(juce::Colours::white);
            g.drawEllipse(b, 2.0f);
        }
        g.setColour(juce::Colour(colour).withMultipliedBrightness(over ? 1.15f : 1.0f));
        g.fillEllipse(b.reduced(getToggleState() ? 4.0f : 2.0f));
    }

private:
    juce::uint32 colour;
};

// ============================================================
//  AccentPicker  --  colour selector shown in a CallOutBox
// ============================================================
class AccentPicker : public juce::Component
{
public:
    AccentPicker(juce::Colour initial, std::function<void(juce::Colour)> apply)
        : onApply(std::move(apply)),
          selector(juce::ColourSelector::showColourAtTop | juce::ColourSelector::showSliders
                   | juce::ColourSelector::showColourspace)
    {
        selector.setCurrentColour(initial, juce::dontSendNotification);
        selector.setColour(juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(selector);
        applyBtn.setButtonText(TR("APPLY"));
        applyBtn.setColour(juce::TextButton::buttonColourId, InUtero::c(InUtero::Acento));
        applyBtn.setColour(juce::TextButton::textColourOffId, InUtero::c(InUtero::Negro));
        applyBtn.onClick = [this]
        {
            auto col = selector.getCurrentColour().withAlpha(1.0f);
            if (onApply) onApply(col);
            if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
                box->dismiss();
        };
        addAndMakeVisible(applyBtn);
        setSize(300, 330);
    }

    void resized() override
    {
        auto a = getLocalBounds().reduced(6);
        applyBtn.setBounds(a.removeFromBottom(30));
        a.removeFromBottom(6);
        selector.setBounds(a);
    }

private:
    std::function<void(juce::Colour)> onApply;
    juce::ColourSelector selector;
    juce::TextButton applyBtn { "APPLY" };
};
