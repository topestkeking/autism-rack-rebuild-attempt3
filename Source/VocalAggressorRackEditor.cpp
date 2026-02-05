/*
  ==============================================================================

    VocalAggressorRackEditor.cpp - Finalized Design

  ==============================================================================
*/

#include "VocalAggressorRackEditor.h"
#include "VocalAggressorRack.h"

LevelMeter::LevelMeter(VocalAggressorRack& p) : processor(p)
{
    startTimerHz(30);
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::black);
    g.fillRect(bounds);

    float level = processor.getCurrentLevel();
    float db = juce::Decibels::gainToDecibels(level);
    float height = juce::jmap(db, -60.0f, 0.0f, 0.0f, bounds.getHeight());

    g.setColour(juce::Colours::green.withAlpha(0.8f));
    if (db > -6.0f) g.setColour(juce::Colours::yellow.withAlpha(0.8f));
    if (db > -1.0f) g.setColour(juce::Colours::red.withAlpha(0.8f));

    g.fillRect(bounds.withTop(bounds.getHeight() - height));
}

void PressureMap::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(bounds, 5.0f);

    const auto& detector = processor.getPressureDetector();
    float intensity = detector.getIntensity();
    float density = detector.getDensity();
    float timbre = detector.getTimbre();

    // Draw a 2D map: Intensity (X) vs Density (Y)
    g.setColour(juce::Colours::orange);
    float x = juce::jmap(intensity, 0.0f, 1.0f, 15.0f, bounds.getWidth() - 15.0f);
    float y = juce::jmap(density, 0.0f, 1.0f, bounds.getHeight() - 15.0f, 15.0f);

    // Circle size morphs with Timbre (Harshness)
    float radius = 4.0f + timbre * 10.0f;
    g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);

    g.setColour(juce::Colours::orange.withAlpha(0.2f));
    g.drawEllipse(x - radius - 5, y - radius - 5, (radius + 5) * 2.0f, (radius + 5) * 2.0f, 1.0f);

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(10.0f);
    g.drawText("PRESSURE MAP", bounds.reduced(5), juce::Justification::bottomLeft);
}

VocalAggressorRackEditor::VocalAggressorRackEditor (VocalAggressorRack& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      dynModule("DYNAMICS", p.apvts, "bypass_dyn"),
      dynCable(p.apvts, "bypass_dyn"),
      eqModule("EQ", p.apvts, "bypass_eq"),
      eqCable(p.apvts, "bypass_eq"),
      harmModule("HARMONICS", p.apvts, "bypass_harm"),
      harmCable(p.apvts, "bypass_harm"),
      shiftModule("SHIFT", p.apvts, "bypass_shift"),
      shiftCable(p.apvts, "bypass_shift"),
      spaceModule("SPACE", p.apvts, "bypass_space"),
      spaceCable(p.apvts, "bypass_space"),
      meter(p),
      pressureMap(p)
{
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        l.setText(name, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
    };

    // Intensity
    intensitySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    intensitySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(intensitySlider);

    intensityLabel.setText("INTENSITY", juce::dontSendNotification);
    intensityLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(intensityLabel);

    intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "intensity", intensitySlider);

    // Dynamics
    setupSlider(dynAmountSlider, dynAmountLabel, "Function");
    setupSlider(dynSustainSlider, dynSustainLabel, "Sustain");
    dynModule.addControl(dynAmountSlider, dynAmountLabel);
    dynModule.addControl(dynSustainSlider, dynSustainLabel);
    addAndMakeVisible(dynModule);
    addAndMakeVisible(dynCable);
    dynAmountAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "dyn_amount", dynAmountSlider);
    dynSustainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "dyn_sustain", dynSustainSlider);

    // EQ
    setupSlider(eqScoopSlider, eqScoopLabel, "Scoop");
    setupSlider(eqBiteSlider, eqBiteLabel, "Bite");
    eqModule.addControl(eqScoopSlider, eqScoopLabel);
    eqModule.addControl(eqBiteSlider, eqBiteLabel);
    addAndMakeVisible(eqModule);
    addAndMakeVisible(eqCable);
    eqScoopAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "eq_scoop", eqScoopSlider);
    eqBiteAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "eq_bite", eqBiteSlider);

    // Harmonics
    setupSlider(harmGritSlider, harmGritLabel, "Grit");
    setupSlider(harmClaritySlider, harmClarityLabel, "Clarity");
    harmModule.addControl(harmGritSlider, harmGritLabel);
    harmModule.addControl(harmClaritySlider, harmClarityLabel);
    addAndMakeVisible(harmModule);
    addAndMakeVisible(harmCable);
    harmGritAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "harm_grit", harmGritSlider);
    harmClarityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "harm_clarity", harmClaritySlider);

    // Shift
    setupSlider(shiftPitchSlider, shiftPitchLabel, "Pitch");
    setupSlider(shiftFormantSlider, shiftFormantLabel, "Formant");
    shiftModule.addControl(shiftPitchSlider, shiftPitchLabel);
    shiftModule.addControl(shiftFormantSlider, shiftFormantLabel);
    addAndMakeVisible(shiftModule);
    addAndMakeVisible(shiftCable);
    shiftPitchAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "shift_pitch", shiftPitchSlider);
    shiftFormantAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "shift_formant", shiftFormantSlider);

    // Space
    setupSlider(spaceMixSlider, spaceMixLabel, "Mix");
    setupSlider(spaceCharSlider, spaceCharLabel, "Char");
    spaceModule.addControl(spaceMixSlider, spaceMixLabel);
    spaceModule.addControl(spaceCharSlider, spaceCharLabel);
    addAndMakeVisible(spaceModule);
    addAndMakeVisible(spaceCable);
    spaceMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "space_mix", spaceMixSlider);
    spaceCharAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "space_char", spaceCharSlider);

    // The Muscle
    setupSlider(muscleSlider, muscleLabel, "MUSCLE");
    addAndMakeVisible(muscleSlider);
    addAndMakeVisible(muscleLabel);
    muscleAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "muscle", muscleSlider);

    // New Features
    setupSlider(voidWidthSlider, voidWidthLabel, "VOID");
    addAndMakeVisible(voidWidthSlider);
    addAndMakeVisible(voidWidthLabel);
    voidWidthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "void_width", voidWidthSlider);

    setupSlider(wallDriveSlider, wallDriveLabel, "DRIVE");
    addAndMakeVisible(wallDriveSlider);
    addAndMakeVisible(wallDriveLabel);
    wallDriveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "wall_drive", wallDriveSlider);

    setupSlider(wallCeilSlider, wallCeilLabel, "CEIL");
    addAndMakeVisible(wallCeilSlider);
    addAndMakeVisible(wallCeilLabel);
    wallCeilAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "wall_ceil", wallCeilSlider);

    addAndMakeVisible(meter);
    addAndMakeVisible(pressureMap);

    setSize (500, 850);
}

VocalAggressorRackEditor::~VocalAggressorRackEditor() {}

void VocalAggressorRackEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff1a1a1a)); // Very dark rack background

    g.setColour (juce::Colours::orange.withAlpha(0.7f));
    g.setFont (juce::Font(24.0f, juce::Font::bold));
    g.drawText ("VOCAL AGGRESSOR RACK", getLocalBounds().removeFromTop(40), juce::Justification::centred, true);

    // Draw some rack rails
    g.setColour(juce::Colours::black);
    g.fillRect(0, 0, 10, getHeight());
    g.fillRect(getWidth() - 10, 0, 10, getHeight());
}

void VocalAggressorRackEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(40); // Title space

    auto meterArea = area.removeFromRight(40).reduced(5);
    meter.setBounds(meterArea);

    auto mainArea = area.reduced(10);

    // Header: Intensity, Muscle, Map
    auto headerArea = mainArea.removeFromTop(150);
    auto leftHeader = headerArea.removeFromLeft(180);

    intensityLabel.setBounds(leftHeader.removeFromTop(20));
    intensitySlider.setBounds(leftHeader.removeFromTop(60).reduced(5));
    muscleLabel.setBounds(leftHeader.removeFromTop(20));
    muscleSlider.setBounds(leftHeader.reduced(5));

    pressureMap.setBounds(headerArea.reduced(10));

    // Footer: The Wall and The Void
    auto footerArea = mainArea.removeFromBottom(100);
    auto f1 = footerArea.removeFromLeft(footerArea.getWidth() / 3);
    voidWidthLabel.setBounds(f1.removeFromTop(20));
    voidWidthSlider.setBounds(f1.reduced(5));

    auto f2 = footerArea.removeFromLeft(footerArea.getWidth() / 2);
    wallDriveLabel.setBounds(f2.removeFromTop(20));
    wallDriveSlider.setBounds(f2.reduced(5));

    auto f3 = footerArea;
    wallCeilLabel.setBounds(f3.removeFromTop(20));
    wallCeilSlider.setBounds(f3.reduced(5));

    // Modules stacked vertically
    int moduleHeight = mainArea.getHeight() / 5;

    auto layoutModule = [&](RackModule& m, PatchCable& c, juce::Rectangle<int> bounds) {
        m.setBounds(bounds);
        auto cArea = bounds;
        c.setBounds(cArea.removeFromRight(40).reduced(5));
    };

    layoutModule(dynModule, dynCable, mainArea.removeFromTop(moduleHeight));
    layoutModule(eqModule, eqCable, mainArea.removeFromTop(moduleHeight));
    layoutModule(harmModule, harmCable, mainArea.removeFromTop(moduleHeight));
    layoutModule(shiftModule, shiftCable, mainArea.removeFromTop(moduleHeight));
    layoutModule(spaceModule, spaceCable, mainArea.removeFromTop(moduleHeight));
}
