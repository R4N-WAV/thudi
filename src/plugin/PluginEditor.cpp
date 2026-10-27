#include "PluginEditor.h"
#include "Assets.h"
#include "GaerroArt.h"
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
    gaerro::paintOverlay (g, graffiti, graffitiAlpha);   // under the print and every control

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

    // ---- printed labels (in GÆRRO mode they get a dark edge so they stay readable over the paint)
    const float shadow = graffiti > 0.0f ? 0.9f * graffitiAlpha * juce::jlimit (0.0f, 1.0f, graffiti * 4.0f) : 0.0f;
    auto print = [&] (const juce::String& t, juce::Rectangle<float> r, juce::Justification j, juce::Colour c, float opacity = 1.0f)
    {
        if (shadow > 0.0f)
        {
            g.setColour (juce::Colours::black.withAlpha (shadow * opacity));
            for (auto d : { juce::Point<float> (1.2f, 1.2f), juce::Point<float> (-0.8f, 0.6f), juce::Point<float> (0.6f, -0.8f) })
                g.drawText (t, r.translated (d.x, d.y), j, true);
        }
        g.setColour (c.withMultipliedAlpha (opacity));
        g.drawText (t, r, j, true);
    };
    g.setFont (assets::labelFont (13.0f));
    auto under = [&] (juce::Point<float> c, float rad, const juce::String& t, float opacity = 1.0f)
    { print (t, juce::Rectangle<float> (c.x - 60.0f, c.y + rad * 1.28f + 2.0f, 120.0f, 15.0f), juce::Justification::centredTop, colours::print, opacity); };
    under (L::masterKnob(), L::masterKnobR, "MASTER");
    under (L::driveKnob(), L::smallKnobR, "BUS DRIVE");
    if (graffiti > 0.0f)
    {
        g.setFont (assets::labelFont (12.0f));
        const float o = juce::jlimit (0.0f, 1.0f, graffiti * 3.0f) * graffitiAlpha;
        under (L::vinylKnob(), L::gaerroKnobR, "VINYL", o);
        under (L::dirtKnob(), L::gaerroKnobR, "DIRT", o);
        g.setFont (assets::labelFont (13.0f));
    }
    print ("DATA ENTRY", juce::Rectangle<float> (L::wheel().x - 60.0f, L::wheel().y - L::wheelR - 22.0f, 120.0f, 15.0f), juce::Justification::centred, colours::print);
    g.setFont (assets::labelFont (12.0f));
    for (int i = 0; i < 8; ++i)
        if (knobNames[(size_t) i].isNotEmpty())
            under (L::pageKnob (i), L::pageKnobR, knobNames[(size_t) i].toUpperCase());

    // pad labels printed above each pad (name left, number right); the selected pad's name is printed brighter
    if (proc.soloOn())
    {   // SOLO: one sound across all pads -> one main label over the grid
        auto a = soloLabelArea();
        g.setFont (assets::labelFont (12.0f));
        print ("SOLO", a, juce::Justification::bottomLeft, colours::printDim);
        print (juce::String (proc.padValue (kSoloSlot, P::Poly) >= 0.5f ? "POLY" : "MONO")
                   + (proc.padValue (kSoloSlot, P::Glide) > 0.5f ? "  GLIDE" : ""), a, juce::Justification::bottomRight, colours::printDim);
        g.setFont (assets::labelFont (21.0f));
        auto name = proc.getSoloLabel(); if (name.isEmpty()) name = "CHOOSE A SOUND";
        print (name + juce::String (juce::CharPointer_UTF8 ("  \xe2\x96\xbe")), a, juce::Justification::centredBottom, colours::print);
    }
    else
    {
        for (int i = 0; i < 16; ++i)
        {
            auto lr = L::padLabel (i);
            g.setFont (assets::labelFont (15.0f));
            print (proc.getPadLabel (i), lr, juce::Justification::bottomLeft, i == selectedPad ? juce::Colour (0xfff1eee6) : colours::print);
            print (juce::String (i + 1), lr, juce::Justification::bottomRight, i == selectedPad ? colours::print : colours::printDim);
            if (i == selectedPad)
            {   // small printed marker under the selected pad's name
                g.setColour (colours::print.withAlpha (0.7f));
                g.fillRect (lr.getX(), lr.getBottom() + 1.5f, 14.0f, 1.5f);
            }
        }
    }

    if (bg.isValid()) return; // the Blender render already contains the 3D emblem

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

juce::Rectangle<float> PanelContent::soloLabelArea()
{
    auto first = L::padLabel (12), last = L::padLabel (15);
    return first.getUnion (last).withTrimmedTop (-4.0f);
}

void PanelContent::mouseDown (const juce::MouseEvent& e)
{
    if (proc.soloOn() && soloLabelArea().contains (e.position) && onSoloLabelClick) onSoloLabelClick();
}

void PanelContent::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (L::plaque().expanded (4.0f).contains (e.position) && onPlaqueDoubleClick) onPlaqueDoubleClick();
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
    masterKnob.setBounds (juce::Rectangle<float> (L::masterKnobR * 2.8f, L::masterKnobR * 2.8f).withCentre (L::masterKnob()).toNearestInt());
    driveKnob.setBounds (juce::Rectangle<float> (L::smallKnobR * 2.8f, L::smallKnobR * 2.8f).withCentre (L::driveKnob()).toNearestInt());
    masterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ThudiProcessor::globalParamId (G::Master), masterKnob);
    driveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ThudiProcessor::globalParamId (G::BusDrive), driveKnob);
    masterKnob.setDoubleClickReturnValue (true, 0.0);
    for (auto* k : { &vinylKnob, &dirtKnob })
    {
        setupKnob (*k, false);
        k->setVisible (false);
    }
    vinylKnob.setBounds (juce::Rectangle<float> (L::gaerroKnobR * 2.8f, L::gaerroKnobR * 2.8f).withCentre (L::vinylKnob()).toNearestInt());
    dirtKnob.setBounds (juce::Rectangle<float> (L::gaerroKnobR * 2.8f, L::gaerroKnobR * 2.8f).withCentre (L::dirtKnob()).toNearestInt());
    vinylAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ThudiProcessor::globalParamId (G::Vinyl), vinylKnob);
    dirtAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ThudiProcessor::globalParamId (G::GaerroDirt), dirtKnob);
    vinylKnob.setDoubleClickReturnValue (true, 0.5); dirtKnob.setDoubleClickReturnValue (true, 0.6);

    for (int i = 0; i < 8; ++i)
    {
        setupKnob (pageKnobs[(size_t) i], false);
        pageKnobs[(size_t) i].setBounds (juce::Rectangle<float> (L::pageKnobR * 2.8f, L::pageKnobR * 2.8f).withCentre (L::pageKnob (i)).toNearestInt());
        pageKnobs[(size_t) i].onDragStart = [this, i] { lcd.focus = i; lcd.repaint(); };
    }

    for (auto* b : { &osButton, &enterButton, &soloButton, &monoButton, &glideButton, &gaerroButton }) b->setPaintingIsUnclipped (true);
    for (auto& b : softKeys) b.setPaintingIsUnclipped (true);
    content.addAndMakeVisible (osButton);
    osButton.setBounds (L::osButton().toNearestInt());
    osButton.onClick = [this]
    {
        const int cur = (int) std::round (proc.globalParam (G::Oversampling)->convertFrom0to1 (proc.globalParam (G::Oversampling)->getValue()));
        proc.setGlobalValue (G::Oversampling, (float) ((cur + 1) % 3));
        updateOsButton();
    };
    updateOsButton();

    // ---- mode keys: SOLO (+ MONO/POLY, GLIDE), and the hidden GÆRRO key
    content.addAndMakeVisible (soloButton);
    soloButton.setButtonText ("SOLO");
    soloButton.setClickingTogglesState (true);
    soloButton.onClick = [this] { setSolo (soloButton.getToggleState()); };
    content.addChildComponent (monoButton);
    monoButton.onClick = [this]
    {
        proc.setPadValue (kSoloSlot, P::Poly, proc.padValue (kSoloSlot, P::Poly) >= 0.5f ? 0.0f : 1.0f);
        refreshAll();
    };
    content.addChildComponent (glideButton);
    glideButton.setButtonText ("GLIDE");
    glideButton.setClickingTogglesState (true);
    glideButton.onClick = [this]
    {
        const float cur = proc.padValue (kSoloSlot, P::Glide);
        if (cur > 0.5f) { proc.meta.setProperty ("soloGlide", cur, nullptr); proc.setPadValue (kSoloSlot, P::Glide, 0.0f); }
        else proc.setPadValue (kSoloSlot, P::Glide, (float) proc.meta.getProperty ("soloGlide", 70.0f));
        refreshAll();
    };
    content.addChildComponent (gaerroButton);
    gaerroButton.setButtonText (juce::CharPointer_UTF8 ("G\xc3\x86RRO"));
    gaerroButton.setClickingTogglesState (true);
    gaerroButton.getProperties().set ("ledColour", colours::padGlowG.toString());
    gaerroButton.onClick = [this] { setGaerro (gaerroButton.getToggleState(), true); };
    content.onPlaqueDoubleClick = [this] { revealGaerroButton (true); };
    content.onSoloLabelClick = [this] { showSoloMenu(); };
    gaerroButtonShown = (bool) proc.meta.getProperty ("gaerroRevealed", false) || proc.gaerroOn()
                        || PresetManager::userRoot().getChildFile (".gaerro").existsAsFile();   // found once = found for good
    gaerroButtonAnim = gaerroButtonShown ? 1.0f : 0.0f;
    layoutModeButtons();

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
        pc->onSoloMenu = [this] { showSoloMenu(); };
        pc->onLoadSample = [this] (int idx) { loadSampleInto (idx); };
        content.addAndMakeVisible (*pc);
        pads.push_back (std::move (pc));
    }

    proc.onMetaChanged = [this] { refreshAll(); };

    setResizable (true, true);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) L::W / (double) L::H);
    setResizeLimits ((int) (L::W * 0.5f), (int) (L::H * 0.5f), (int) (L::W * 1.3f), (int) (L::H * 1.3f));
    const float scale = (float) proc.meta.getProperty ("uiScale", 0.72f);
    setSize ((int) (L::W * scale), (int) (L::H * scale));

    lastSolo = proc.soloOn();
    lastGaerro = proc.gaerroOn();
    content.graffiti = lastGaerro ? 1.0f : 0.0f;
    lcd.pad = editSlot();
    selectPad (juce::jlimit (0, 15, proc.selectedPad));
    updateModeButtons();
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

ThudiEditor::~ThudiEditor()
{
    stopTimer();
    proc.onMetaChanged = nullptr;
    for (auto& a : pageAttach) a.reset();
    masterAttach.reset(); driveAttach.reset(); vinylAttach.reset(); dirtAttach.reset();
    setLookAndFeel (nullptr);
}

void ThudiEditor::paint (juce::Graphics& g) { g.fillAll (colours::body); }

void ThudiEditor::resized()
{
    const float s = (float) getWidth() / L::W;
    content.setTransform (juce::AffineTransform::scale (s));
    proc.meta.setProperty ("uiScale", s, nullptr);
}

int ThudiEditor::editSlot() const { return proc.soloOn() ? kSoloSlot : proc.selectedPad; }

void ThudiEditor::updateOsButton()
{
    const int cur = (int) std::round (proc.globalParam (G::Oversampling)->convertFrom0to1 (proc.globalParam (G::Oversampling)->getValue()));
    osButton.setButtonText ("OVERSAMPLE " + juce::String (cur == 0 ? "1x" : cur == 1 ? "2x" : "4x"));
}

void ThudiEditor::updateModeButtons()
{
    const bool solo = proc.soloOn();
    soloButton.setToggleState (solo, juce::dontSendNotification);
    monoButton.setVisible (solo);
    glideButton.setVisible (solo);
    monoButton.setButtonText (proc.padValue (kSoloSlot, P::Poly) >= 0.5f ? "POLY" : "MONO");
    glideButton.setToggleState (proc.padValue (kSoloSlot, P::Glide) > 0.5f, juce::dontSendNotification);
    gaerroButton.setToggleState (proc.gaerroOn(), juce::dontSendNotification);
    const float ga = content.graffiti > 0.0f ? juce::jlimit (0.0f, 1.0f, content.graffiti * 3.0f) * content.graffitiAlpha : 0.0f;
    for (auto* k : { &vinylKnob, &dirtKnob }) { k->setVisible (ga > 0.0f); k->setAlpha (ga); }
}

void ThudiEditor::layoutModeButtons()
{
    soloButton.setBounds (L::soloButton().toNearestInt());
    monoButton.setBounds (L::monoButton().toNearestInt());
    glideButton.setBounds (L::glideButton().toNearestInt());
    // the GÆRRO key slides out from under the plaque
    const float x = gaerroButtonAnim, e = 1.0f - (1.0f - x) * (1.0f - x) * (1.0f - x);
    auto target = L::gaerroButton(), from = target.withCentre (L::plaque().getCentre());
    auto r = from.toFloat().translated ((target.getX() - from.getX()) * e, (target.getY() - from.getY()) * e);
    gaerroButton.setBounds (r.toNearestInt());
    gaerroButton.setVisible (x > 0.0f);
    gaerroButton.setAlpha (juce::jlimit (0.0f, 1.0f, x * 1.6f));
    gaerroButton.setInterceptsMouseClicks (x >= 1.0f, false);
}

void ThudiEditor::revealGaerroButton (bool animate)
{
    if (gaerroButtonShown) { if (gaerroButtonAnim >= 1.0f) lcd.flash ("..."); return; }
    gaerroButtonShown = true;
    proc.meta.setProperty ("gaerroRevealed", true, nullptr);
    {
        auto f = PresetManager::userRoot().getChildFile (".gaerro");
        f.getParentDirectory().createDirectory();
        f.replaceWithText ("unlocked\n");
    }
    gaerroButtonAnim = animate ? 0.001f : 1.0f;
    layoutModeButtons();
}

void ThudiEditor::setSolo (bool on)
{
    proc.setGlobalValue (G::SoloOn, on ? 1.0f : 0.0f);
    lastSolo = on;
    lcd.pad = editSlot();
    lcd.page = 0;
    if (lcd.mode == LcdDisplay::Mode::Browse) lcd.refreshEntries();
    lcd.flash (on ? "SOLO: ONE SOUND ON EVERY NOTE" : "SOLO OFF");
    refreshAll();
}

void ThudiEditor::setGaerro (bool on, bool animate)
{
    if (proc.gaerroOn() != on) proc.setGlobalValue (G::Gaerro, on ? 1.0f : 0.0f);
    lastGaerro = on;
    if (on)
    {
        content.graffitiAlpha = 1.0f;
        if (animate) { content.graffiti = 0.0001f; lcd.startSmoke(); }
        else content.graffiti = 1.0f;
    }
    else
    {
        lcd.stopSmoke();
        if (! animate) { content.graffiti = 0.0f; content.graffitiAlpha = 1.0f; }
        lcd.flash ("BACK TO CLEAN");
    }
    updateModeButtons();
    for (auto& pc : pads) pc->setLight (pc->getLight(), on);
    content.repaint();
}

void ThudiEditor::finishAnimations()
{
    lcd.stopSmoke();
    content.graffiti = proc.gaerroOn() ? 1.0f : 0.0f;
    content.graffitiAlpha = 1.0f;
    gaerroButtonAnim = gaerroButtonShown ? 1.0f : 0.0f;
    layoutModeButtons();
    updateModeButtons();
    content.repaint();
}

void ThudiEditor::selectPad (int pad)
{
    proc.selectedPad = pad;
    content.selectedPad = pad;
    if (! proc.soloOn())
    {
        lcd.pad = pad;
        if (lcd.mode == LcdDisplay::Mode::Browse && lcd.browse == LcdDisplay::Browse::Pads) lcd.refreshEntries();
        rebuildPage();
    }
    content.repaint();
}

void ThudiEditor::setPage (int page) { lcd.page = page; rebuildPage(); }

void ThudiEditor::rebuildPage()
{
    lcd.pad = editSlot();
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
    updateModeButtons();
    content.repaint();
}

void ThudiEditor::softKey (int i)
{
    if (lcd.smoking()) return;
    if (lcd.mode == LcdDisplay::Mode::Edit)
    {
        if (i == 0) { lcd.page = (lcd.page - 1 + lcd.numPages()) % lcd.numPages(); rebuildPage(); }
        else if (i == 1) { lcd.page = (lcd.page + 1) % lcd.numPages(); rebuildPage(); }
        else if (i == 2)
        {
            lcd.mode = LcdDisplay::Mode::Browse;
            // an empty pad or the solo slot wants a sound: start in the sound library
            const bool wantsSound = lcd.pad == kSoloSlot || (int) std::round (proc.padValue (lcd.pad, P::Model)) == (int) Model::Empty;
            lcd.browse = wantsSound ? LcdDisplay::Browse::Pads : LcdDisplay::Browse::Kits;
            lcd.refreshEntries();
        }
        else showSaveMenu();
    }
    else
    {
        if (i == 0) { lcd.browse = lcd.browse == LcdDisplay::Browse::Kits ? LcdDisplay::Browse::Pads : LcdDisplay::Browse::Kits; lcd.refreshEntries(); }
        else if (i == 1) loadSelectedEntry();
        else if (i == 2)
        {
            if (lcd.browse == LcdDisplay::Browse::Kits)
                askText ("Save kit", proc.getKitName() == "Init" ? juce::String ("My Kit") : proc.getKitName(), [this] (const juce::String& t)
                {
                    lcd.flash (proc.presets->saveKit (t) ? "SAVED KIT " + t.toUpperCase() : proc.presets->lastError.toUpperCase());
                    lcd.refreshEntries();
                });
            else
                askText (lcd.pad == kSoloSlot ? "Save sound" : "Save pad preset", proc.getPadLabel (lcd.pad), [this] (const juce::String& t)
                {
                    lcd.flash (proc.presets->savePad (lcd.pad, t) ? "SAVED " + t.toUpperCase() : proc.presets->lastError.toUpperCase());
                    lcd.refreshEntries();
                });
        }
        else { lcd.mode = LcdDisplay::Mode::Edit; rebuildPage(); }
    }
    lcd.repaint();
}

void ThudiEditor::enter()
{
    if (lcd.smoking()) return;
    if (lcd.mode == LcdDisplay::Mode::Browse) loadSelectedEntry();
    else { lcd.mode = LcdDisplay::Mode::Browse; lcd.browse = LcdDisplay::Browse::Pads; lcd.refreshEntries(); }
}

void ThudiEditor::loadSelectedEntry()
{
    if (lcd.entries.empty()) return;
    const auto e = lcd.entries[(size_t) juce::jlimit (0, (int) lcd.entries.size() - 1, lcd.browseIndex)];
    bool ok;
    if (lcd.browse == LcdDisplay::Browse::Kits) ok = proc.presets->loadKit (e);
    else
    {
        const int slot = lcd.pad;
        // SOLO: MONO/POLY and GLIDE are performance switches, they stay as set when another sound is chosen
        const float poly = proc.padValue (kSoloSlot, P::Poly), glide = proc.padValue (kSoloSlot, P::Glide);
        ok = proc.presets->loadPad (slot, e);
        if (ok && slot == kSoloSlot) { proc.setPadValue (kSoloSlot, P::Poly, poly); proc.setPadValue (kSoloSlot, P::Glide, glide); }
    }
    lcd.flash (ok ? "LOADED " + e.name.toUpperCase() : proc.presets->lastError.toUpperCase());
    refreshAll();
}

void ThudiEditor::wheelStep (int steps)
{
    if (lcd.mode == LcdDisplay::Mode::Browse) { lcd.scrollBrowse (steps); return; }
    auto pages = pagesForModel ((int) std::round (proc.padValue (lcd.pad, P::Model)));
    const auto& knobs = pages[(size_t) juce::jlimit (0, (int) pages.size() - 1, lcd.page)].knobs;
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

void ThudiEditor::addLibraryMenu (juce::PopupMenu& m, int slot)
{
    juce::String currentCat;
    juce::PopupMenu catMenu;
    auto flush = [&] { if (currentCat.isNotEmpty()) m.addSubMenu (currentCat, catMenu); catMenu = {}; };
    for (auto& e : proc.presets->listPads())
    {
        if (e.category != currentCat) { flush(); currentCat = e.category; }
        catMenu.addItem ((e.factory ? "" : "* ") + e.name, [this, e, slot]
        {
            lcd.mode = LcdDisplay::Mode::Browse; lcd.browse = LcdDisplay::Browse::Pads;
            lcd.pad = slot; lcd.refreshEntries();
            for (size_t i = 0; i < lcd.entries.size(); ++i)
                if (lcd.entries[i].name == e.name && lcd.entries[i].category == e.category) lcd.browseIndex = (int) i;
            loadSelectedEntry();
            lcd.mode = LcdDisplay::Mode::Edit; rebuildPage();
        });
    }
    flush();
}

void ThudiEditor::showSoloMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("SOLO  " + proc.getSoloLabel());
    juce::PopupMenu lib;
    addLibraryMenu (lib, kSoloSlot);
    m.addSubMenu ("Choose sound", lib);
    m.addItem ("Edit sound", [this] { lcd.mode = LcdDisplay::Mode::Edit; lcd.page = 0; rebuildPage(); });
    m.addItem ("Rename...", [this] { askText ("Rename solo sound", proc.getSoloLabel(), [this] (const juce::String& t) { proc.setSoloLabel (t); refreshAll(); }); });
    m.addItem ("Save as pad preset...", [this]
    {
        askText ("Save sound", proc.getSoloLabel(), [this] (const juce::String& t)
        { lcd.flash (proc.presets->savePad (kSoloSlot, t) ? "SAVED " + t.toUpperCase() : proc.presets->lastError.toUpperCase()); });
    });
    m.addSeparator();
    const bool poly = proc.padValue (kSoloSlot, P::Poly) >= 0.5f;
    m.addItem ("Mono (legato)", true, ! poly, [this] { proc.setPadValue (kSoloSlot, P::Poly, 0.0f); refreshAll(); });
    m.addItem ("Poly", true, poly, [this] { proc.setPadValue (kSoloSlot, P::Poly, 1.0f); refreshAll(); });
    m.addItem ("Glide", true, proc.padValue (kSoloSlot, P::Glide) > 0.5f, [this] { glideButton.triggerClick(); });
    juce::PopupMenu root;
    const int curRoot = (int) std::round (proc.globalParam (G::KeysRoot)->convertFrom0to1 (proc.globalParam (G::KeysRoot)->getValue()));
    for (int n = 24; n <= 84; n += 12)
        root.addItem ("Pad 1 = " + juce::MidiMessage::getMidiNoteName (n, true, true, 3), true, curRoot == n, [this, n] { proc.setGlobalValue (G::KeysRoot, (float) n); refreshAll(); });
    m.addSubMenu ("Pads play from", root);
    m.addSeparator();
    m.addItem ("Leave solo mode", [this] { setSolo (false); });
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&soloButton));
}

void ThudiEditor::showSaveMenu()
{
    juce::PopupMenu m;
    const auto kitName = proc.getKitName();
    const bool userKit = PresetManager::kitsDir().getChildFile (juce::File::createLegalFileName (kitName) + ".thudikit").existsAsFile();
    if (userKit)
        m.addItem ("Save kit \"" + kitName + "\"", [this, kitName]
        { lcd.flash (proc.presets->saveKit (kitName) ? "SAVED KIT " + kitName.toUpperCase() : proc.presets->lastError.toUpperCase()); });
    m.addItem ("Save kit as...", [this]
    {
        askText ("Save kit", proc.getKitName() == "Init" ? juce::String ("My Kit") : proc.getKitName(), [this] (const juce::String& t)
        { lcd.flash (proc.presets->saveKit (t) ? "SAVED KIT " + t.toUpperCase() : proc.presets->lastError.toUpperCase()); });
    });
    const int slot = lcd.pad;
    m.addItem (slot == kSoloSlot ? juce::String ("Save solo sound as preset...") : "Save pad " + juce::String (slot + 1) + " as preset...", [this, slot]
    {
        askText ("Save pad preset", proc.getPadLabel (slot), [this, slot] (const juce::String& t)
        { lcd.flash (proc.presets->savePad (slot, t) ? "SAVED PAD " + t.toUpperCase() : proc.presets->lastError.toUpperCase()); });
    });
    m.addSeparator();
    m.addItem ("New empty kit (Init)", [this] { proc.presets->initKit(); lcd.flash ("INIT: EMPTY KIT"); refreshAll(); });
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

// ---------------------------------------------------------------- samples (GÆRRO mode)
static bool isAudioFile (const juce::String& path)
{
    return juce::File (path).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg");
}

bool ThudiEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    if (! proc.gaerroOn()) return false;
    for (auto& f : files) if (isAudioFile (f)) return true;
    return false;
}

void ThudiEditor::filesDropped (const juce::StringArray& files, int x, int y)
{
    juce::String path;
    for (auto& f : files) if (isAudioFile (f)) { path = f; break; }
    if (path.isEmpty()) return;
    const auto pt = content.getLocalPoint (this, juce::Point<int> (x, y)).toFloat();
    // dropped on the waveform of a sample pad: replace just that pad's audio, keep its region
    if (lcd.getBounds().toFloat().contains (pt) && lcd.pad != kSoloSlot && (int) std::round (proc.padValue (lcd.pad, P::Model)) == (int) Model::Sample)
    {
        juce::String err;
        if (auto s = proc.loadSampleFile (juce::File (path), err)) { proc.setSlotSample (lcd.pad, s); lcd.flash ("LOADED " + juce::String (s->name).toUpperCase()); }
        else lcd.flash (err.toUpperCase());
        refreshAll();
        return;
    }
    int first = 0;
    for (int i = 0; i < 16; ++i) if (L::pad (i).expanded (8.0f).contains (pt)) first = i;
    askChop (juce::File (path), first);
}

void ThudiEditor::askChop (const juce::File& f, int firstPad)
{
    auto* w = new juce::AlertWindow ("Chop " + f.getFileName(), "Spread the sample over the pads, starting at pad " + juce::String (firstPad + 1) + ".",
                                     juce::MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel (&lnf);
    juce::StringArray choices;
    const int maxSlices = kNumPads - firstPad;
    for (int n = 1; n <= maxSlices; ++n) choices.add (n == 1 ? juce::String ("1 pad (whole file)") : juce::String (n) + " pads");
    w->addComboBox ("slices", choices, "Slices");
    auto* combo = w->getComboBoxComponent ("slices");
    const int last = (int) proc.meta.getProperty ("lastSlices", 16);
    combo->setSelectedItemIndex (juce::jlimit (0, maxSlices - 1, last - 1));
    w->addButton ("CHOP", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w, f, firstPad] (int result)
    {
        if (result != 1) return;
        const int n = w->getComboBoxComponent ("slices")->getSelectedItemIndex() + 1;
        proc.meta.setProperty ("lastSlices", n, nullptr);
        juce::String err;
        const int made = proc.chopFileOntoPads (f, n, firstPad, err);
        if (made > 0)
        {
            lcd.flash ("CHOPPED INTO " + juce::String (made) + (made == 1 ? " PAD" : " PADS"));
            if (proc.soloOn()) setSolo (false);
            selectPad (firstPad);
            lcd.mode = LcdDisplay::Mode::Edit; lcd.page = 0;
        }
        else lcd.flash (err.toUpperCase());
        refreshAll();
    }), true);
}

void ThudiEditor::loadSampleInto (int pad)
{
    chooser = std::make_unique<juce::FileChooser> ("Load a sample to chop", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, pad] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f.existsAsFile()) askChop (f, pad);
    });
}

// ---------------------------------------------------------------- animation / meters (60 Hz)
void ThudiEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = (float) juce::jlimit (0.0, 0.1, (now - lastTick) * 0.001);
    lastTick = now;

    const bool gaerro = proc.gaerroOn();
    if (gaerro != lastGaerro) setGaerro (gaerro, true);          // automation / kit load
    if (proc.soloOn() != lastSolo) { lastSolo = proc.soloOn(); lcd.pad = editSlot(); refreshAll(); }

    for (int i = 0; i < 16; ++i) pads[(size_t) i]->setLight (proc.padLight (i), gaerro);

    // graffiti thrown on / washed off
    if (gaerro && content.graffiti > 0.0f && content.graffiti < 1.0f)
    {
        content.graffiti = juce::jmin (1.0f, content.graffiti + dt / gaerro::kRevealSeconds);
        content.repaint(); updateModeButtons();
    }
    if (! gaerro && content.graffiti > 0.0f)
    {
        content.graffitiAlpha -= dt / 0.6f;
        if (content.graffitiAlpha <= 0.0f) { content.graffiti = 0.0f; content.graffitiAlpha = 1.0f; }
        content.repaint(); updateModeButtons();
    }
    if (gaerroButtonAnim > 0.0f && gaerroButtonAnim < 1.0f)
    {
        gaerroButtonAnim = juce::jmin (1.0f, gaerroButtonAnim + dt / 0.45f);
        layoutModeButtons();
        content.repaint (L::gaerroButton().getUnion (L::plaque()).expanded (12.0f).toNearestInt());
        if (gaerroButtonAnim >= 1.0f) { lcd.flash (juce::CharPointer_UTF8 ("G\xc3\x86RRO MODE UNLOCKED")); }
    }
    lcd.advance (dt);

    if (lcd.messageTicks > 0 && --lcd.messageTicks == 0) lcd.message.clear();
    const int model = (int) std::round (proc.padValue (lcd.pad, P::Model));
    if (model != lastModel) rebuildPage();
    if (((int) (now / 33.0)) % 2 == 0) lcd.repaint();   // ~30 fps is plenty for the text
}
