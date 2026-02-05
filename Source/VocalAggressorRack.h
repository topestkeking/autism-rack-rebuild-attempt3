/*
  ==============================================================================

    VocalAggressorRack.h - Finalized Design
    Created: 27 Dec 2025 3:30:00pm
    Author:  Jules

  ==============================================================================
*/

#pragma once

#include "PressureDetector.h"
#include "DynamicsModule.h"
#include "EQModule.h"
#include "HarmonicsModule.h"
#include "ShiftModule.h"
#include "SpaceModule.h"
#include "ClipperModule.h"
#include "WidenerModule.h"

class VocalAggressorRackEditor;

//==============================================================================
class VocalAggressorRack  : public juce::AudioProcessor
{
public:
    //==============================================================================
    VocalAggressorRack()
        : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                           .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
          apvts (*this, nullptr, "Parameters", createParameterLayout())
    {
    }

    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add (std::make_unique<juce::AudioParameterFloat>  ("intensity", "Master Intensity", 0.0f, 1.0f, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterFloat>  ("muscle", "The Muscle (Parallel)", 0.0f, 1.0f, 1.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat>  ("dyn_amount", "Dynamics Amount", 0.0f, 1.0f, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterFloat>  ("dyn_sustain", "Sustain Cut", 0.0f, 1.0f, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterBool>   ("bypass_dyn", "Bypass Dynamics", false));

        layout.add (std::make_unique<juce::AudioParameterFloat>  ("eq_scoop", "EQ Scoop", 0.0f, 1.0f, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterFloat>  ("eq_bite", "EQ Bite", 0.0f, 1.0f, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterBool>   ("bypass_eq", "Bypass EQ", false));

        layout.add (std::make_unique<juce::AudioParameterFloat>  ("harm_grit", "Harmonics Grit", 0.0f, 1.0f, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterFloat>  ("harm_clarity", "Harmonics Clarity", 0.0f, 1.0f, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterBool>   ("bypass_harm", "Bypass Harmonics", false));

        layout.add (std::make_unique<juce::AudioParameterFloat>  ("shift_pitch", "Pitch Shift", 0.0f, 1.0f, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterFloat>  ("shift_formant", "Formant Shift", 0.0f, 1.0f, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterBool>   ("bypass_shift", "Bypass Shift", false));

        layout.add (std::make_unique<juce::AudioParameterFloat>  ("space_mix", "Space Mix", 0.0f, 1.0f, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterFloat>  ("space_char", "Space Character", 0.0f, 1.0f, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterBool>   ("bypass_space", "Bypass Space", false));

        layout.add (std::make_unique<juce::AudioParameterFloat>  ("void_width", "The Void (Width)", 0.0f, 1.0f, 0.3f));
        layout.add (std::make_unique<juce::AudioParameterFloat>  ("wall_drive", "The Wall (Drive)", 0.0f, 12.0f, 0.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat>  ("wall_ceil", "The Wall (Ceiling)", -12.0f, 0.0f, -0.1f));

        return layout;
    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = getTotalNumOutputChannels();

        pressureDetector.prepare(spec);
        dynamicsModule.prepare(spec);
        eqModule.prepare(spec);
        harmonicsModule.prepare(spec);
        shiftModule.prepare(spec);
        spaceModule.prepare(spec);
        widenerModule.prepare(spec);
        clipperModule.prepare(spec);

        dryBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    }

    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
            return false;
        return true;
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override
    {
        juce::ScopedNoDenormals noDenormals;
        auto totalNumInputChannels  = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();

        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear (i, 0, buffer.getNumSamples());

        // Sidechain access
        auto sidechainBuffer = getBusBuffer (buffer, true, 1);

        updateParameters();

        // "The Muscle" - Store dry signal
        int numSamples = buffer.getNumSamples();
        for (int i = 0; i < totalNumOutputChannels; ++i)
            dryBuffer.copyFrom(i, 0, buffer.getReadPointer(i), numSamples);

        // 1. Analyze the pressure (with Sidechain support)
        pressureDetector.process(buffer, &sidechainBuffer);

        // 2. Process through the module chain
        if (! *apvts.getRawParameterValue ("bypass_dyn"))
            dynamicsModule.process(buffer, pressureDetector);

        if (! *apvts.getRawParameterValue ("bypass_eq"))
            eqModule.process(buffer, pressureDetector);

        if (! *apvts.getRawParameterValue ("bypass_harm"))
            harmonicsModule.process(buffer, pressureDetector);

        if (! *apvts.getRawParameterValue ("bypass_shift"))
            shiftModule.process(buffer, pressureDetector);

        if (! *apvts.getRawParameterValue ("bypass_space"))
            spaceModule.process(buffer, pressureDetector);

        // 3. New Features: The Void and The Wall
        widenerModule.process(buffer, pressureDetector, *apvts.getRawParameterValue("void_width"));

        // Parallel Blend (The Muscle)
        float mix = *apvts.getRawParameterValue("muscle");
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto* dryData = dryBuffer.getReadPointer(channel);
            auto* wetData = buffer.getWritePointer(channel);
            for (int sample = 0; sample < numSamples; ++sample)
                wetData[sample] = dryData[sample] * (1.0f - mix) + wetData[sample] * mix;
        }

        clipperModule.process(buffer, *apvts.getRawParameterValue("wall_drive"), *apvts.getRawParameterValue("wall_ceil"));

        // Update level for the meter
        float maxLevel = 0.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            maxLevel = std::max(maxLevel, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));

        lastLevel.set(maxLevel);
    }

    float getCurrentLevel() const { return lastLevel.get(); }
    const PressureDetector& getPressureDetector() const { return pressureDetector; }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                              { return true; }

    const juce::String getName() const override                  { return "Vocal Aggressor Rack"; }
    bool acceptsMidi() const override                            { return false; }
    bool producesMidi() const override                           { return false; }
    bool isMidiEffect() const override                           { return false; }
    double getTailLengthSeconds() const override                 { return 0.0; }

    int getNumPrograms() override                                { return 1; }
    int getCurrentProgram() override                             { return 0; }
    void setCurrentProgram (int index) override                  {}
    const juce::String getProgramName (int index) override       { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override
    {
        auto state = apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
        if (xmlState.get() != nullptr)
            if (xmlState->hasTagName (apvts.state.getType()))
                apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
    }

    juce::AudioProcessorValueTreeState apvts;

private:
    void updateParameters()
    {
        // Master INTENSITY controls the range and depth of all reactive components.
        // At 0% (0.0), it provides a controlled shaper.
        // At 100% (1.0), it pushes everything into "monster" territory.
        float m = *apvts.getRawParameterValue ("intensity");
        float aggressionScale = 0.4f + (m * 1.6f); // 0.4x to 2.0x range

        dynamicsModule.functionAmount = juce::jlimit(0.0f, 1.0f, (float)*apvts.getRawParameterValue ("dyn_amount") * aggressionScale);
        dynamicsModule.sustainCut     = juce::jlimit(0.0f, 1.0f, (float)*apvts.getRawParameterValue ("dyn_sustain") * aggressionScale);

        eqModule.scoopAmount = juce::jlimit(0.0f, 1.0f, (float)*apvts.getRawParameterValue ("eq_scoop") * aggressionScale);
        eqModule.biteAmount  = juce::jlimit(0.0f, 1.0f, (float)*apvts.getRawParameterValue ("eq_bite") * aggressionScale);

        harmonicsModule.gritAmount    = juce::jlimit(0.0f, 1.0f, (float)*apvts.getRawParameterValue ("harm_grit") * aggressionScale);
        harmonicsModule.clarityAmount = juce::jlimit(0.0f, 1.0f, (float)*apvts.getRawParameterValue ("harm_clarity") * aggressionScale);

        // Pitch/Formant shifts become much more extreme as intensity rises
        float shiftScale = 1.0f + (m * 2.0f); // 1x to 3x sensitivity
        shiftModule.pitchShift   = (*apvts.getRawParameterValue ("shift_pitch") - 0.5f) * 24.0f * shiftScale;
        shiftModule.formantShift = (*apvts.getRawParameterValue ("shift_formant") - 0.5f) * 24.0f * shiftScale;

        spaceModule.mixAmount       = juce::jlimit(0.0f, 1.0f, (float)*apvts.getRawParameterValue ("space_mix") * aggressionScale);
        spaceModule.characterAmount = juce::jlimit(0.0f, 1.0f, (float)*apvts.getRawParameterValue ("space_char") * aggressionScale);
    }

    //==============================================================================
    PressureDetector pressureDetector;
    DynamicsModule   dynamicsModule;
    EQModule         eqModule;
    HarmonicsModule  harmonicsModule;
    ShiftModule      shiftModule;
    SpaceModule      spaceModule;
    WidenerModule    widenerModule;
    ClipperModule    clipperModule;

    juce::AudioBuffer<float> dryBuffer;
    juce::Atomic<float> lastLevel { 0.0f };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalAggressorRack)
};
