/*
  ==============================================================================

    VocalAggressorRackEditor.h - Finalized Design

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class VocalAggressorRack; // Forward declaration

//==============================================================================
class PatchCable : public juce::Component
{
public:
    PatchCable(juce::AudioProcessorValueTreeState& vts, const juce::String& paramID)
        : apvts(vts), id(paramID)
    {
        setInterceptsMouseClicks(true, false);
    }

    void paint(juce::Graphics& g) override
    {
        bool bypassed = *apvts.getRawParameterValue(id) > 0.5f;

        g.setColour(bypassed ? juce::Colours::grey : juce::Colours::red.darker(0.2f));

        auto bounds = getLocalBounds().toFloat();
        juce::Path p;

        if (bypassed)
        {
            // Dangling cable
            p.startNewSubPath(bounds.getCentreX(), 0);
            p.quadraticTo(bounds.getCentreX() + 10, bounds.getHeight() * 0.4f,
                         bounds.getCentreX() - 5, bounds.getHeight() * 0.7f);
        }
        else
        {
            // Connected cable
            p.startNewSubPath(bounds.getCentreX(), 0);
            p.quadraticTo(bounds.getCentreX() - 20, bounds.getHeight() * 0.5f,
                         bounds.getCentreX(), bounds.getHeight());
        }

        g.strokePath(p, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Plugs
        g.setColour(juce::Colours::silver);
        g.fillEllipse(bounds.getCentreX() - 4, -2, 8, 8);
        if (!bypassed)
            g.fillEllipse(bounds.getCentreX() - 4, bounds.getHeight() - 6, 8, 8);
        else
            g.fillEllipse(p.getCurrentPosition().getX() - 4, p.getCurrentPosition().getY() - 4, 8, 8);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        auto* param = apvts.getParameter(id);
        param->setValueNotifyingHost(param->getValue() > 0.5f ? 0.0f : 1.0f);
        repaint();
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String id;
};

//==============================================================================
class PressureMap : public juce::Component, public juce::Timer
{
public:
    PressureMap(VocalAggressorRack& p) : processor(p)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override;

    void timerCallback() override { repaint(); }

private:
    VocalAggressorRack& processor;
};

//==============================================================================
class LevelMeter : public juce::Component, public juce::Timer
{
public:
    LevelMeter(VocalAggressorRack& p);
    void paint(juce::Graphics& g) override;
    void timerCallback() override { repaint(); }

private:
    VocalAggressorRack& processor;
};

//==============================================================================
class RackModule : public juce::GroupComponent
{
public:
    RackModule(const juce::String& name, juce::AudioProcessorValueTreeState& vts, const juce::String& bypassID)
        : juce::GroupComponent({}, name), apvts(vts), id(bypassID)
    {
        setTextLabelPosition(juce::Justification::centredTop);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        bool bypassed = *apvts.getRawParameterValue(id) > 0.5f;

        // Faceplate
        g.setColour(juce::Colours::darkgrey.darker(0.5f));
        g.fillRoundedRectangle(bounds.reduced(2), 4.0f);

        if (bypassed)
            g.setOpacity(0.3f);
        else
            g.setOpacity(1.0f);

        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(bounds.reduced(2), 4.0f, 1.0f);

        juce::GroupComponent::paint(g);

        // Rack Screws
        g.setColour(juce::Colours::grey);
        g.fillEllipse(5, 5, 4, 4);
        g.fillEllipse(bounds.getWidth() - 9, 5, 4, 4);
        g.fillEllipse(5, bounds.getHeight() - 9, 4, 4);
        g.fillEllipse(bounds.getWidth() - 9, bounds.getHeight() - 9, 4, 4);
    }

    void addControl(juce::Component& c, juce::Component& label)
    {
        addAndMakeVisible(c);
        addAndMakeVisible(label);
        controls.add(&c);
        labels.add(&label);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        area.removeFromTop(20); // Room for title

        int count = controls.size();
        if (count == 0) return;

        int width = area.getWidth() / count;
        for (int i = 0; i < count; ++i)
        {
            auto cArea = area.removeFromLeft(width);
            labels[i]->setBounds(cArea.removeFromBottom(20));
            controls[i]->setBounds(cArea.reduced(5));
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String id;
    juce::Array<juce::Component*> controls;
    juce::Array<juce::Component*> labels;
};

//==============================================================================
class VocalAggressorRackEditor  : public juce::AudioProcessorEditor
{
public:
    VocalAggressorRackEditor (VocalAggressorRack&);
    ~VocalAggressorRackEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VocalAggressorRack& audioProcessor;

    juce::Slider intensitySlider;
    juce::Label intensityLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;

    RackModule dynModule;
    juce::Slider dynAmountSlider, dynSustainSlider;
    juce::Label dynAmountLabel, dynSustainLabel;
    PatchCable dynCable;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dynAmountAttach, dynSustainAttach;

    RackModule eqModule;
    juce::Slider eqScoopSlider, eqBiteSlider;
    juce::Label eqScoopLabel, eqBiteLabel;
    PatchCable eqCable;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eqScoopAttach, eqBiteAttach;

    RackModule harmModule;
    juce::Slider harmGritSlider, harmClaritySlider;
    juce::Label harmGritLabel, harmClarityLabel;
    PatchCable harmCable;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> harmGritAttach, harmClarityAttach;

    RackModule shiftModule;
    juce::Slider shiftPitchSlider, shiftFormantSlider;
    juce::Label shiftPitchLabel, shiftFormantLabel;
    PatchCable shiftCable;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shiftPitchAttach, shiftFormantAttach;

    RackModule spaceModule;
    juce::Slider spaceMixSlider, spaceCharSlider;
    juce::Label spaceMixLabel, spaceCharLabel;
    PatchCable spaceCable;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> spaceMixAttach, spaceCharAttach;

    LevelMeter meter;
    PressureMap pressureMap;

    juce::Slider muscleSlider;
    juce::Label muscleLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> muscleAttach;

    juce::Slider wallDriveSlider, wallCeilSlider, voidWidthSlider;
    juce::Label wallDriveLabel, wallCeilLabel, voidWidthLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wallDriveAttach, wallCeilAttach, voidWidthAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalAggressorRackEditor)
};
