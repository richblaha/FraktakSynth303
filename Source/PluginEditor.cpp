#include "PluginEditor.h"

static constexpr int W    = 800;
static constexpr int H    = 360;
static constexpr int TOP  = 62;
static constexpr int KW   = 54;
static constexpr int KH   = 60;
static constexpr int GAP  = 6;
static constexpr int LGAP = 24;
static constexpr int ROW2 = TOP + KH + LGAP;
static constexpr int ROW3 = ROW2 + KH + LGAP;

FraktalSynth303AudioProcessorEditor::FraktalSynth303AudioProcessorEditor
    (FraktalSynth303AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (W, H);
    auto& apvts = processor.apvts;

    // ── Wave selector ─────────────────────────────────────────────
    waveSelector.addItem ("Lorenz",      1);
    waveSelector.addItem ("FM Feedback", 2);
    waveSelector.addItem ("Granular",    3);
    waveSelector.addItem ("Saw",         4);
    waveSelector.addItem ("Square",      5);
    waveSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF141414));
    waveSelector.setColour (juce::ComboBox::textColourId,       juce::Colour (0xFFC8FF00));
    waveSelector.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xFF2A2A2A));
    waveSelector.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xFFC8FF00));
    addAndMakeVisible (waveSelector);
    waveAttach = std::make_unique<ComboAttach> (apvts, "wave", waveSelector);

    waveLabel.setText ("WAVE", juce::dontSendNotification);
    waveLabel.setFont (juce::FontOptions ("Helvetica Neue", 9.0f, juce::Font::plain));
    waveLabel.setColour (juce::Label::textColourId, juce::Colour (0xFF555555));
    waveLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (waveLabel);

    // ── Stutter Mode + Div selectors ──────────────────────────────
    stutterModeBox.addItem ("Free", 1);
    stutterModeBox.addItem ("Sync", 2);
    stutterModeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF141414));
    stutterModeBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xFFFF4488));
    stutterModeBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xFF2A2A2A));
    stutterModeBox.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xFFFF4488));
    addAndMakeVisible (stutterModeBox);
    stutterModeAttach = std::make_unique<ComboAttach> (apvts, "stutterMode", stutterModeBox);

    stutterDivBox.addItem ("1/4",   1);
    stutterDivBox.addItem ("1/8",   2);
    stutterDivBox.addItem ("1/16",  3);
    stutterDivBox.addItem ("1/32",  4);
    stutterDivBox.addItem ("1/64",  5);
    stutterDivBox.addItem ("1/128", 6);
    stutterDivBox.addItem ("1/4T",  7);   //T = Triolen
    stutterDivBox.addItem ("1/8T",  8);
    stutterDivBox.addItem ("1/16T", 9);

    stutterDivBox.addItem ("1/4Q",  10);  // Q = Quintolen
    stutterDivBox.addItem ("1/8Q",  11);

    stutterDivBox.addItem ("1/4S",  12);  // S = Septolen
    stutterDivBox.addItem ("1/8S",  13);
    
    stutterDivBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF141414));
    stutterDivBox.setJustificationType (juce::Justification::centredLeft);
    stutterDivBox.setEditableText (false);
    
 
    
    stutterDivBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xFFFF4488));
    stutterDivBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xFF2A2A2A));
    stutterDivBox.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xFFFF4488));
    addAndMakeVisible (stutterDivBox);
    stutterDivAttach = std::make_unique<ComboAttach> (apvts, "stutterDiv", stutterDivBox);

    // ── Farben ────────────────────────────────────────────────────
    juce::Colour green  (0xFFC8FF00);
    juce::Colour orange (0xFFFF8C00);
    juce::Colour pink   (0xFFFF4488);
    juce::Colour cyan   (0xFF00DDCC);
    juce::Colour yellow (0xFFFFDD00);
    juce::Colour violet (0xFFCC44FF);
    juce::Colour teal   (0xFF00FFCC);

    // ── Knobs ─────────────────────────────────────────────────────
    styleKnob (pwmKnob,       pwmLabel,       "PWM",       yellow);
    styleKnob (volumeKnob,    volumeLabel,    "VOL",       green);
    styleKnob (gran1Knob,     gran1Label,     "GRAN 1",    teal);
    styleKnob (gran2Knob,     gran2Label,     "GRAN 2",    teal);
    styleKnob (cutoffKnob,    cutoffLabel,    "CUTOFF",    green);
    styleKnob (resonanceKnob, resonanceLabel, "RESO",      green);
    styleKnob (driveKnob,     driveLabel,     "DRIVE",     orange);
    styleKnob (filterEnvKnob, filterEnvLabel, "ENV >",     green);
    styleKnob (lfoRateKnob,   lfoRateLabel,   "RATE",      orange);
    styleKnob (lfoDepthKnob,  lfoDepthLabel,  "DEPTH",     orange);
    styleKnob (attackKnob,    attackLabel,    "ATK",       cyan);
    styleKnob (decayKnob,     decayLabel,     "DEC",       cyan);
    styleKnob (sustainKnob,   sustainLabel,   "SUS",       cyan);
    styleKnob (releaseKnob,   releaseLabel,   "REL",       cyan);
    styleKnob (chaosKnob,     chaosLabel,     "SMEAR",     pink);
    styleKnob (chaos2Knob,    chaos2Label,    "FRAKTAL",   pink);
    styleKnob (stutterKnob,   stutterLabel,   "STUTTER",   pink);
    styleKnob (morphKnob,     morphLabel,     "MORPH",     violet);
    styleKnob (morphAutoKnob, morphAutoLabel, "AUTO",      violet);
    styleKnob (phaseDestKnob, phaseDestLabel, "PHASE",     violet);
    styleKnob (timeFoldKnob,  timeFoldLabel,  "TIME",      violet);

    // ── Attachments ───────────────────────────────────────────────
    pwmAttach       = std::make_unique<SliderAttach> (apvts, "pwm",          pwmKnob);
    volumeAttach    = std::make_unique<SliderAttach> (apvts, "volume",       volumeKnob);
    gran1Attach     = std::make_unique<SliderAttach> (apvts, "gran1",        gran1Knob);
    gran2Attach     = std::make_unique<SliderAttach> (apvts, "gran2",        gran2Knob);
    cutoffAttach    = std::make_unique<SliderAttach> (apvts, "cutoff",       cutoffKnob);
    resonanceAttach = std::make_unique<SliderAttach> (apvts, "resonance",    resonanceKnob);
    driveAttach     = std::make_unique<SliderAttach> (apvts, "drive",        driveKnob);
    filterEnvAttach = std::make_unique<SliderAttach> (apvts, "filterEnvAmt", filterEnvKnob);
    attackAttach    = std::make_unique<SliderAttach> (apvts, "attack",       attackKnob);
    decayAttach     = std::make_unique<SliderAttach> (apvts, "decay",        decayKnob);
    sustainAttach   = std::make_unique<SliderAttach> (apvts, "sustain",      sustainKnob);
    releaseAttach   = std::make_unique<SliderAttach> (apvts, "release",      releaseKnob);
    lfoRateAttach   = std::make_unique<SliderAttach> (apvts, "lfoRate",      lfoRateKnob);
    lfoDepthAttach  = std::make_unique<SliderAttach> (apvts, "lfoDepth",     lfoDepthKnob);
    chaosAttach     = std::make_unique<SliderAttach> (apvts, "chaos",        chaosKnob);
    chaos2Attach    = std::make_unique<SliderAttach> (apvts, "chaos2",       chaos2Knob);
    stutterAttach   = std::make_unique<SliderAttach> (apvts, "stutter",      stutterKnob);
    morphAttach     = std::make_unique<SliderAttach> (apvts, "morph",        morphKnob);
    morphAutoAttach = std::make_unique<SliderAttach> (apvts, "morphAuto",    morphAutoKnob);
    phaseDestAttach = std::make_unique<SliderAttach> (apvts, "phaseDest",    phaseDestKnob);
    timeFoldAttach  = std::make_unique<SliderAttach> (apvts, "timeFold",     timeFoldKnob);
}

FraktalSynth303AudioProcessorEditor::~FraktalSynth303AudioProcessorEditor() {}

void FraktalSynth303AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (C_BG);
    g.setColour (C_ACCENT);
    g.fillRect (0, 0, W, 2);

    g.setFont (juce::FontOptions ("Helvetica Neue", 12.0f, juce::Font::bold));
    g.setColour (C_ACCENT);
    g.drawText ("FRAKTAL 303", 12, 7, 200, 16, juce::Justification::left);

    g.setFont (juce::FontOptions ("Helvetica Neue", 8.0f, juce::Font::plain));
    g.setColour (C_MUTED);
    g.drawText ("morph · phase dest · time fold · granular · chaos", 12, 22, 380, 12,
                juce::Justification::left);

    g.setColour (C_DIM);
    g.fillRect (178, 38, 1, H - 44);
    g.fillRect (408, 38, 1, H - 44);
    g.fillRect (556, 38, 1, H - 44);
    g.fillRect (676, 38, 1, H - 44);

    juce::Colour violet (0xFFCC44FF);
    juce::Colour teal   (0xFF00FFCC);
    drawSection (g, "OSC",    {   0, 38, 178, 18 }, C_MUTED);
    drawSection (g, "FILTER", { 179, 38, 229, 18 }, C_MUTED);
    drawSection (g, "ADSR",   { 409, 38, 147, 18 }, C_MUTED);
    drawSection (g, "FX",     { 557, 38, 119, 18 }, C_MUTED);
    drawSection (g, "LIVE",   { 677, 38, 123, 18 }, violet);

    // LIVE glow
    g.setColour (violet.withAlpha (0.04f));
    g.fillRect (677, 38, 123, H - 44);

    // Granular label unter OSC
    g.setFont (juce::FontOptions ("Helvetica Neue", 7.5f, juce::Font::plain));
    g.setColour (teal.withAlpha (0.5f));
    g.drawText ("GRANULAR MIX", 8, ROW3 - 14, 160, 12, juce::Justification::left);
}

void FraktalSynth303AudioProcessorEditor::resized()
{
    // ── OSC (0–177) ───────────────────────────────────────────────
    waveLabel.setBounds    (10, TOP - 14, 155, 12);
    waveSelector.setBounds (10, TOP,      155, 24);

    // Row 2: PWM + VOL
    pwmKnob.setBounds      (10,        ROW2, KW, KH);
    pwmLabel.setBounds     (10,        ROW2 + KH - 2, KW, 13);
    volumeKnob.setBounds   (10+KW+GAP, ROW2, KW, KH);
    volumeLabel.setBounds  (10+KW+GAP, ROW2 + KH - 2, KW, 13);

    // Row 3: GRAN 1 + GRAN 2
    gran1Knob.setBounds    (10,        ROW3, KW, KH);
    gran1Label.setBounds   (10,        ROW3 + KH - 2, KW, 13);
    gran2Knob.setBounds    (10+KW+GAP, ROW3, KW, KH);
    gran2Label.setBounds   (10+KW+GAP, ROW3 + KH - 2, KW, 13);

    // ── FILTER (179–407) ─────────────────────────────────────────
    int fx = 184;
    cutoffKnob.setBounds    (fx,            TOP,  KW, KH);
    cutoffLabel.setBounds   (fx,            TOP+KH-2, KW, 13);
    resonanceKnob.setBounds (fx+KW+GAP,     TOP,  KW, KH);
    resonanceLabel.setBounds(fx+KW+GAP,     TOP+KH-2, KW, 13);
    filterEnvKnob.setBounds (fx+(KW+GAP)*2, TOP,  KW, KH);
    filterEnvLabel.setBounds(fx+(KW+GAP)*2, TOP+KH-2, KW, 13);
    driveKnob.setBounds     (fx,            ROW2, KW, KH);
    driveLabel.setBounds    (fx,            ROW2+KH-2, KW, 13);
    lfoRateKnob.setBounds   (fx+KW+GAP,     ROW2, KW, KH);
    lfoRateLabel.setBounds  (fx+KW+GAP,     ROW2+KH-2, KW, 13);
    lfoDepthKnob.setBounds  (fx+(KW+GAP)*2, ROW2, KW, KH);
    lfoDepthLabel.setBounds (fx+(KW+GAP)*2, ROW2+KH-2, KW, 13);

    // ── ADSR (409–555) ────────────────────────────────────────────
    int ax = 414;
    attackKnob.setBounds   (ax,       TOP,  KW, KH);
    attackLabel.setBounds  (ax,       TOP+KH-2, KW, 13);
    decayKnob.setBounds    (ax+KW+GAP, TOP,  KW, KH);
    decayLabel.setBounds   (ax+KW+GAP, TOP+KH-2, KW, 13);
    sustainKnob.setBounds  (ax,       ROW2, KW, KH);
    sustainLabel.setBounds (ax,       ROW2+KH-2, KW, 13);
    releaseKnob.setBounds  (ax+KW+GAP, ROW2, KW, KH);
    releaseLabel.setBounds (ax+KW+GAP, ROW2+KH-2, KW, 13);

    // ── FX (557–675) ──────────────────────────────────────────────
    int cx = 562;
    chaosKnob.setBounds    (cx,        TOP,  KW, KH);
    chaosLabel.setBounds   (cx,        TOP+KH-2, KW, 13);
    chaos2Knob.setBounds   (cx+KW+GAP, TOP,  KW, KH);
    chaos2Label.setBounds  (cx+KW+GAP, TOP+KH-2, KW, 13);
    stutterKnob.setBounds  (cx,        ROW2, KW, KH);
    stutterLabel.setBounds (cx,        ROW2+KH-2, KW, 13);
    
    // Stutter mode/div dropdowns 
    stutterModeBox.setBounds (cx, ROW3, KW + 20, 20);
    stutterDivBox.setBounds  (cx, ROW3 + 22, KW + 20, 20);

    // ── LIVE (677–799) ────────────────────────────────────────────
    int lx = 681;
    morphKnob.setBounds     (lx,        TOP,  KW, KH);
    morphLabel.setBounds    (lx,        TOP+KH-2, KW, 13);
    morphAutoKnob.setBounds (lx+KW+GAP, TOP,  KW, KH);
    morphAutoLabel.setBounds(lx+KW+GAP, TOP+KH-2, KW, 13);
    phaseDestKnob.setBounds (lx,        ROW2, KW, KH);
    phaseDestLabel.setBounds(lx,        ROW2+KH-2, KW, 13);
    timeFoldKnob.setBounds  (lx+KW+GAP, ROW2, KW, KH);
    timeFoldLabel.setBounds (lx+KW+GAP, ROW2+KH-2, KW, 13);
}

void FraktalSynth303AudioProcessorEditor::styleKnob (
    juce::Slider& s, juce::Label& l, const juce::String& name, juce::Colour accent)
{
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setColour (juce::Slider::rotarySliderFillColourId,    accent);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF202020));
    s.setColour (juce::Slider::thumbColourId,               accent);
    s.setColour (juce::Slider::backgroundColourId,          juce::Colour (0xFF141414));
    addAndMakeVisible (s);

    l.setText (name, juce::dontSendNotification);
    l.setFont (juce::FontOptions ("Helvetica Neue", 7.5f, juce::Font::plain));
    l.setColour (juce::Label::textColourId, accent.withAlpha (0.75f));
    l.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (l);
}

void FraktalSynth303AudioProcessorEditor::drawSection (
    juce::Graphics& g, const juce::String& text,
    juce::Rectangle<int> bounds, juce::Colour col)
{
    g.setFont (juce::FontOptions ("Helvetica Neue", 7.5f, juce::Font::plain));
    g.setColour (col.withAlpha (0.9f));
    g.drawText (text, bounds, juce::Justification::centred);
}

juce::AudioProcessorEditor* FraktalSynth303AudioProcessor::createEditor()
{
    return new FraktalSynth303AudioProcessorEditor (*this);
}
