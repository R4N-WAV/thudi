#include "PluginEditor.h"
#include "Assets.h"
#include "Layout.h"
#include "Models.h"
#include "ParamFormat.h"

using namespace thudi;
namespace L = thudi::layout;

// ===================================================================== DataWheel
void DataWheel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto img = assets::image ("wheel.png");
    const auto c = r.getCentre();
    if (img.isValid())
    {
        g.drawImageTransformed (img, juce::AffineTransform::scale (r.getWidth() / (float) img.getWidth(), r.getHeight() / (float) img.getHeight())
                                        .rotated (angle, c.x, c.y));
        auto top = assets::image ("wheel_light.png"); // static highlight overlay (does not rotate)
        if (top.isValid()) g.drawImage (top, r, juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        auto w = r.reduced (2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (w.translated (0, 4.0f));
        juce::ColourGradient cg (juce::Colour (0xffd8d8d4), w.getX(), w.getY(), juce::Colour (0xff6f6f6c), w.getRight(), w.getBottom(), false);
        g.setGradientFill (cg); g.fillEllipse (w);
        g.setColour (juce::Colour (0xff2a2a2c));
        for (int i = 0; i < 24; ++i)
        {
            const float a = angle + (float) i * juce::MathConstants<float>::twoPi / 24.0f;
            g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (c.getPointOnCircumference (w.getWidth() * 0.42f, a)));
        }
        g.setColour (juce::Colour (0xffbdbdb8));
        g.fillEllipse (w.reduced (w.getWidth() * 0.2f));
        g.setColour (juce::Colour (0xff3a3a3c));
        g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (c.getPointOnCircumference (w.getWidth() * 0.22f, angle)));
    }
}

void DataWheel::turn (float delta)
{
    angle += delta; accum += delta;
    const float step = juce::MathConstants<float>::twoPi / 24.0f;
    while (accum >= step) { accum -= step; if (onStep) onStep (1); }
    while (accum <= -step) { accum += step; if (onStep) onStep (-1); }
    repaint();
}

void DataWheel::mouseDown (const juce::MouseEvent& e)
{
    auto c = getLocalBounds().toFloat().getCentre();
    lastAngle = std::atan2 (e.position.y - c.y, e.position.x - c.x);
}

void DataWheel::mouseDrag (const juce::MouseEvent& e)
{
    auto c = getLocalBounds().toFloat().getCentre();
    const float a = std::atan2 (e.position.y - c.y, e.position.x - c.x);
    float d = a - lastAngle;
    if (d > juce::MathConstants<float>::pi) d -= juce::MathConstants<float>::twoPi;
    if (d < -juce::MathConstants<float>::pi) d += juce::MathConstants<float>::twoPi;
    lastAngle = a;
    turn (d);
}

void DataWheel::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    turn ((w.deltaY > 0 ? -1.0f : 1.0f) * juce::MathConstants<float>::twoPi / 24.0f);
}

// ===================================================================== PanelContent
void PanelContent::paintFallbackBody (juce::Graphics& g)
{
    auto r = juce::Rectangle<float> (0, 0, L::W, L::H);
    juce::ColourGradient body (juce::Colour (0xff2a2a2d), 0, 0, juce::Colour (0xff141416), 0, L::H, false);
    g.setGradientFill (body); g.fillRect (r);
    g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawRect (r.reduced (6.0f), 2.0f);
    // recesses
    auto well = [&] (juce::Rectangle<float> w, float rad)
    {
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (w, rad);
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawRoundedRectangle (w.translated (0, 1.0f), rad, 1.0f);
    };
    well (L::lcdBezel(), 10.0f);
    // simple engraved swirls
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    for (int k = 0; k < 6; ++k)
    {
        juce::Path s; const float cx = 820.0f + (float) (k % 2) * 40.0f, cy = 300.0f + (float) k * 20.0f;
        s.addCentredArc (cx, cy, 30.0f + (float) k * 4.0f, 18.0f, 0.3f, 0.0f, 4.5f, true);
        g.strokePath (s, juce::PathStrokeType (1.2f));
    }
    // brass plaque
    auto pl = L::plaque();
    juce::ColourGradient brass (juce::Colour (0xffe2c47a), pl.getX(), pl.getY(), juce::Colour (0xff8c6d2c), pl.getRight(), pl.getBottom(), false);
    g.setGradientFill (brass); g.fillRoundedRectangle (pl, 3.0f);
}

void PanelContent::paint (juce::Graphics& g)
{
    auto bg = assets::image ("bg.png");
    if (bg.isValid()) g.drawImage (bg, { 0, 0, L::W, L::H }, juce::RectanglePlacement::stretchToFit);
    else paintFallbackBody (g);

    // ---- brass plaque text (engraved)
    {
        auto pl = L::plaque();
        g.setFont (assets::labelFont (24.0f));
        g.setColour (juce::Colour (0x66ffffff));
        g.drawText ("R4N", pl.translated (0, 1.0f).withTrimmedBottom (14.0f), juce::Justification::centred);
        g.setColour (juce::Colour (0xff3b2d10));
        g.drawText ("R4N", pl.withTrimmedBottom (14.0f), juce::Justification::centred);
        g.setFont (assets::labelFontMedium (10.5f));
        g.drawText ("THUDI  DRUM SYNTHESIZER", pl.withTrimmedTop (27.0f), juce::Justification::centred);
    }

    // ---- printed labels
    g.setColour (colours::print);
    g.setFont (assets::labelFont (13.0f));
    auto under = [&] (juce::Point<float> c, float rad, const juce::String& t)
    { g.drawText (t, juce::Rectangle<float> (c.x - 60.0f, c.y + rad + 3.0f, 120.0f, 15.0f), juce::Justification::centredTop); };
    under (L::masterKnob(), L::masterKnobR, "MASTER");
    under (L::driveKnob(), L::smallKnobR, "BUS DRIVE");
    g.drawText ("DATA ENTRY", juce::Rectangle<float> (L::wheel().x - 60.0f, L::wheel().y - L::wheelR - 22.0f, 120.0f, 15.0f), juce::Justification::centred);
    g.setFont (assets::labelFont (12.0f));
    for (int i = 0; i < 8; ++i)
        if (knobNames[(size_t) i].isNotEmpty())
            under (L::pageKnob (i), L::pageKnobR, knobNames[(size_t) i].toUpperCase());

    // pad labels printed above each pad (name left, number right) + a small status LED
    for (int i = 0; i < 16; ++i)
    {
        auto lr = L::padLabel (i);
        const float act = juce::jlimit (0.0f, 1.0f, padActivity[(size_t) i]);
        auto led = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ lr.getX() + 4.0f, lr.getCentreY() + 2.0f });
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.fillEllipse (led.expanded (1.5f));
        const float on = i == selectedPad ? juce::jmax (0.75f, act) : act;
        g.setColour (juce::Colour (0xff3a0b08).interpolatedWith (juce::Colour (0xffff3b2a), on));
        g.fillEllipse (led);
        g.setColour (juce::Colours::white.withAlpha (0.25f + 0.3f * on));
        g.fillEllipse (led.reduced (2.2f).translated (-0.8f, -0.8f));
        g.setFont (assets::labelFont (15.0f));
        g.setColour (colours::print);
        g.drawText (proc.getPadLabel (i), lr.withTrimmedLeft (13.0f), juce::Justification::bottomLeft, true);
        g.setColour (colours::printDim);
        g.drawText (juce::String (i + 1), lr, juce::Justification::bottomRight);
    }

    if (bg.isValid()) return; // the Blender render already contains the 3D துடி emblem

    // ---- Tamil logo துடி: vertical, reading bottom -> top, stretched to fill the strip
    {
        const auto& path = assets::tamilLogo();
        auto pb = path.getBounds();
        auto box = L::logo();
        // rotate -90 deg: original width -> vertical extent
        auto t = juce::AffineTransform::translation (-pb.getX(), -pb.getY())
                     .followedBy (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi))
                     .followedBy (juce::AffineTransform::translation (0.0f, pb.getWidth()));
        // now bounds are (0,0,h,w) -> stretch to box
        t = t.followedBy (juce::AffineTransform::scale (box.getWidth() / pb.getHeight(), box.getHeight() / pb.getWidth()))
             .followedBy (juce::AffineTransform::translation (box.getX(), box.getY()));
        juce::Path logo (path); logo.applyTransform (t);
        // engraved: dark lower edge, light upper edge, silver fill
        g.setColour (juce::Colours::black.withAlpha (0.75f));
        g.fillPath (logo, juce::AffineTransform::translation (1.5f, 2.0f));
        juce::ColourGradient silver (juce::Colour (0xffe9e6dd), box.getX(), box.getY(), juce::Colour (0xff9d9a92), box.getRight(), box.getBottom(), false);
        silver.addColour (0.5, juce::Colour (0xffcfccc3));
        g.setGradientFill (silver);
        g.fillPath (logo);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.strokePath (logo, juce::PathStrokeType (0.6f));
    }
}

// ===================================================================== Editor
ThudiEditor::ThudiEditor (ThudiProcessor& p)
    : juce::AudioProcessorEditor (p), proc (p), content (p), lcd (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    content.setBounds (0, 0, (int) L::W, (int) L::H);

    auto setupKnob = [this] (juce::Slider& s, bool red)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::degreesToRadians (-135.0f), juce::degreesToRadians (135.0f), true);
        s.setMouseDragSensitivity (220);
        s.setVelocityBasedMode (false);
        if (red) s.getProperties().set ("knobStyle", "red");
        content.addAndMakeVisible (s);
    };
    setupKnob (masterKnob, true); setupKnob (driveKnob, false);
    masterKnob.setBounds (juce::Rectangle<float> (L::masterKnobR * 2, L::masterKnobR * 2).withCentre (L::masterKnob()).toNearestInt());
    driveKnob.setBounds (juce::Rectangle<float> (L::smallKnobR * 2, L::smallKnobR * 2).withCentre (L::driveKnob()).toNearestInt());
    masterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ThudiProcessor::globalParamId (G::Master), masterKnob);
    driveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ThudiProcessor::globalParamId (G::BusDrive), driveKnob);
    masterKnob.setDoubleClickReturnValue (true, 0.0);

    for (int i = 0; i < 8; ++i)
    {
        setupKnob (pageKnobs[(size_t) i], false);
        pageKnobs[(size_t) i].setBounds (juce::Rectangle<float> (L::pageKnobR * 2, L::pageKnobR * 2).withCentre (L::pageKnob (i)).toNearestInt());
        pageKnobs[(size_t) i].onDragStart = [this, i] { lcd.focus = i; lcd.repaint(); };
    }

    content.addAndMakeVisible (osButton);
    osButton.setBounds (L::osButton().toNearestInt());
    osButton.onClick = [this]
    {
        const int cur = (int) std::round (proc.globalParam (G::Oversampling)->convertFrom0to1 (proc.globalParam (G::Oversampling)->getValue()));
        proc.setGlobalValue (G::Oversampling, (float) ((cur + 1) % 3));
        updateOsButton();
    };
    updateOsButton();

    content.addAndMakeVisible (lcd);
    lcd.setBounds (L::lcdGlass().toNearestInt());
    lcd.onPageChanged = [this] (int) { rebuildPage(); };
    lcd.onFocusChanged = [] (int) {};
    lcd.onLoadSelected = [this] { loadSelectedEntry(); };

    content.addAndMakeVisible (wheel);
    wheel.setBounds (juce::Rectangle<float> (L::wheelR * 2, L::wheelR * 2).withCentre (L::wheel()).toNearestInt());
    wheel.onStep = [this] (int s) { wheelStep (s); };

    content.addAndMakeVisible (enterButton);
    enterButton.setButtonText ("ENTER");
    enterButton.setBounds (L::enterButton().toNearestInt());
    enterButton.onClick = [this] { enter(); };

    for (int i = 0; i < 4; ++i)
    {
        content.addAndMakeVisible (softKeys[(size_t) i]);
        softKeys[(size_t) i].setBounds (L::softKey (i).toNearestInt());
        softKeys[(size_t) i].setButtonText ("SOFT KEY " + juce::String (i + 1));
        softKeys[(size_t) i].onClick = [this, i] { softKey (i); };
    }

    for (int i = 0; i < 16; ++i)
    {
        auto pc = std::make_unique<PadComponent> (proc, i);
        pc->setBounds (L::pad (i).toNearestInt());
        pc->onSelect = [this] (int idx) { selectPad (idx); };
        pc->onEdit = [this] (int idx) { selectPad (idx); lcd.mode = LcdDisplay::Mode::Edit; lcd.page = 0; rebuildPage(); };
        pc->onRename = [this] (int idx)
        {
            askText ("Rename pad " + juce::String (idx + 1), proc.getPadLabel (idx), [this, idx] (const juce::String& t) { proc.setPadLabel (idx, t); refreshAll(); });
        };
        pc->onSavePreset = [this] (int idx)
        {
            askText ("Save pad preset", proc.getPadLabel (idx), [this, idx] (const juce::String& t)
            {
                if (proc.presets->savePad (idx, t)) lcd.flash ("SAVED PAD " + t.toUpperCase());
                else lcd.flash (proc.presets->lastError.toUpperCase());
            });
        };
        pc->onChanged = [this] { refreshAll(); };
        content.addAndMakeVisible (*pc);
        pads.push_back (std::move (pc));
    }

    proc.onMetaChanged = [this] { refreshAll(); };

    setResizable (true, true);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) L::W / (double) L::H);
    setResizeLimits ((int) (L::W * 0.5f), (int) (L::H * 0.5f), (int) (L::W * 1.3f), (int) (L::H * 1.3f));
    const float scale = (float) proc.meta.getProperty ("uiScale", 0.72f);
    setSize ((int) (L::W * scale), (int) (L::H * scale));

    selectPad (juce::jlimit (0, 15, proc.selectedPad));
    startTimerHz (30);
}

ThudiEditor::~ThudiEditor()
{
    stopTimer();
    proc.onMetaChanged = nullptr;
    for (auto& a : pageAttach) a.reset();
    masterAttach.reset(); driveAttach.reset();
    setLookAndFeel (nullptr);
}

void ThudiEditor::paint (juce::Graphics& g) { g.fillAll (colours::body); }

void ThudiEditor::resized()
{
    const float s = (float) getWidth() / L::W;
    content.setTransform (juce::AffineTransform::scale (s));
    proc.meta.setProperty ("uiScale", s, nullptr);
}

void ThudiEditor::updateOsButton()
{
    const int cur = (int) std::round (proc.globalParam (G::Oversampling)->convertFrom0to1 (proc.globalParam (G::Oversampling)->getValue()));
    osButton.setButtonText ("OVERSAMPLE " + juce::String (cur == 0 ? "1x" : cur == 1 ? "2x" : "4x"));
}

void ThudiEditor::selectPad (int pad)
{
    proc.selectedPad = pad;
    content.selectedPad = pad;
    lcd.pad = pad;
    if (lcd.mode == LcdDisplay::Mode::Browse && lcd.browse == LcdDisplay::Browse::Pads) lcd.refreshEntries();
    rebuildPage();
    content.repaint();
}

void ThudiEditor::setPage (int page) { lcd.page = page; rebuildPage(); }

void ThudiEditor::rebuildPage()
{
    const int pad = lcd.pad;
    const int model = (int) std::round (proc.padValue (pad, P::Model));
    lastModel = model;
    auto pages = pagesForModel (model);
    lcd.page = juce::jlimit (0, (int) pages.size() - 1, lcd.page);
    const auto& knobs = pages[(size_t) lcd.page].knobs;
    for (int i = 0; i < 8; ++i)
    {
        pageAttach[(size_t) i].reset();
        auto& s = pageKnobs[(size_t) i];
        if (i < (int) knobs.size())
        {
            const auto& pi = info (knobs[(size_t) i]);
            s.setVisible (true);
            s.getProperties().set ("bipolar", pi.min < 0.0f ? "yes" : "no");
            pageAttach[(size_t) i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                proc.apvts, ThudiProcessor::padParamId (pad, knobs[(size_t) i]), s);
            s.setDoubleClickReturnValue (true, pi.def);
            content.knobNames[(size_t) i] = pi.name;
        }
        else
        {
            s.setVisible (false);
            content.knobNames[(size_t) i] = {};
        }
    }
    lcd.focus = juce::jlimit (0, juce::jmax (0, (int) knobs.size() - 1), lcd.focus);
    lcd.repaint();
    content.repaint();
}

void ThudiEditor::refreshAll()
{
    for (auto& pc : pads) pc->repaint();
    rebuildPage();
    if (lcd.mode == LcdDisplay::Mode::Browse) lcd.refreshEntries();
    updateOsButton();
    content.repaint();
}

void ThudiEditor::softKey (int i)
{
    if (lcd.mode == LcdDisplay::Mode::Edit)
    {
        if (i == 0) { lcd.page = (lcd.page - 1 + lcd.numPages()) % lcd.numPages(); rebuildPage(); }
        else if (i == 1) { lcd.page = (lcd.page + 1) % lcd.numPages(); rebuildPage(); }
        else if (i == 2) { lcd.mode = LcdDisplay::Mode::Browse; lcd.browse = LcdDisplay::Browse::Kits; lcd.refreshEntries(); }
        else showSaveMenu();
    }
    else
    {
        if (i == 0) { lcd.browse = LcdDisplay::Browse::Kits; lcd.refreshEntries(); }
        else if (i == 1) { lcd.browse = LcdDisplay::Browse::Pads; lcd.refreshEntries(); }
        else if (i == 2) loadSelectedEntry();
        else { lcd.mode = LcdDisplay::Mode::Edit; rebuildPage(); }
    }
    lcd.repaint();
}

void ThudiEditor::enter()
{
    if (lcd.mode == LcdDisplay::Mode::Browse) loadSelectedEntry();
    else { lcd.mode = LcdDisplay::Mode::Browse; lcd.browse = LcdDisplay::Browse::Pads; lcd.refreshEntries(); }
}

void ThudiEditor::loadSelectedEntry()
{
    if (lcd.entries.empty()) return;
    const auto e = lcd.entries[(size_t) juce::jlimit (0, (int) lcd.entries.size() - 1, lcd.browseIndex)];
    bool ok = lcd.browse == LcdDisplay::Browse::Kits ? proc.presets->loadKit (e) : proc.presets->loadPad (lcd.pad, e);
    lcd.flash (ok ? "LOADED " + e.name.toUpperCase() : proc.presets->lastError.toUpperCase());
    refreshAll();
}

void ThudiEditor::wheelStep (int steps)
{
    if (lcd.mode == LcdDisplay::Mode::Browse) { lcd.scrollBrowse (steps); return; }
    auto pages = pagesForModel ((int) std::round (proc.padValue (lcd.pad, P::Model)));
    const auto& knobs = pages[(size_t) lcd.page].knobs;
    if (lcd.focus >= (int) knobs.size()) return;
    const P p = knobs[(size_t) lcd.focus];
    const auto& pi = info (p);
    float v = proc.padValue (lcd.pad, p);
    if (pi.scale == Scale::Choice || pi.scale == Scale::Int) v = juce::jlimit (pi.min, pi.max, v + (float) steps);
    else v = fromNormalised (pi, juce::jlimit (0.0f, 1.0f, toNormalised (pi, v) + (float) steps * 0.005f));
    proc.setPadValue (lcd.pad, p, v);
    if (p == P::Model) rebuildPage();
    lcd.repaint();
}

void ThudiEditor::showSaveMenu()
{
    juce::PopupMenu m;
    m.addItem ("Save kit as...", [this]
    {
        askText ("Save kit", proc.getKitName(), [this] (const juce::String& t)
        { lcd.flash (proc.presets->saveKit (t) ? "SAVED KIT " + t.toUpperCase() : proc.presets->lastError.toUpperCase()); });
    });
    m.addItem ("Save pad " + juce::String (lcd.pad + 1) + " as preset...", [this]
    {
        askText ("Save pad preset", proc.getPadLabel (lcd.pad), [this] (const juce::String& t)
        { lcd.flash (proc.presets->savePad (lcd.pad, t) ? "SAVED PAD " + t.toUpperCase() : proc.presets->lastError.toUpperCase()); });
    });
    m.addSeparator();
    m.addItem ("Open preset folder", [] { auto d = PresetManager::userRoot(); d.createDirectory(); d.startAsProcess(); });
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&softKeys[3]));
}

void ThudiEditor::askText (const juce::String& title, const juce::String& initial, std::function<void (const juce::String&)> onOk)
{
    auto* w = new juce::AlertWindow (title, {}, juce::MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel (&lnf);
    w->addTextEditor ("name", initial);
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([w, onOk] (int result)
    {
        if (result == 1)
        {
            auto t = w->getTextEditorContents ("name").trim();
            if (t.isNotEmpty()) onOk (t);
        }
    }), true);
}

void ThudiEditor::timerCallback()
{
    for (int i = 0; i < 16; ++i)
    {
        const float a = proc.padActivity (i);
        auto& pc = *pads[(size_t) i];
        if (std::abs (a - pc.lastActivity) > 0.03f || (a < 0.03f && pc.lastActivity >= 0.03f))
        {
            pc.lastActivity = a;
            content.padActivity[(size_t) i] = a;
            content.repaint (L::padLabel (i).expanded (4.0f).toNearestInt());
        }
    }
    if (lcd.messageTicks > 0 && --lcd.messageTicks == 0) lcd.message.clear();
    const int model = (int) std::round (proc.padValue (lcd.pad, P::Model));
    if (model != lastModel) rebuildPage();
    lcd.repaint();
}
