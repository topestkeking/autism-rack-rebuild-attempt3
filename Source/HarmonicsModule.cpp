/*
  ==============================================================================

    HarmonicsModule.cpp - Finalized Design
    Created: 27 Dec 2025 4:00:00pm
    Author:  Jules

  ==============================================================================
*/

#include "HarmonicsModule.h"

HarmonicsModule::HarmonicsModule()
{
}

HarmonicsModule::~HarmonicsModule()
{
}

void HarmonicsModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    clarityHPF.prepare(spec);
    sidechainBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
}

void HarmonicsModule::process(juce::AudioBuffer<float>& buffer, const PressureDetector& detector)
{
    float intensity = detector.getIntensity();
    float density = detector.getDensity();
    float timbre = detector.getTimbre();

    // 1. Grit (Low-mid saturation)
    // Linked to the EQ's carving: Grit is focused just above where the EQ carves mud (~400-800Hz)
    // Depth increases with intensity and spectral density.
    float gritDrive = 1.0f + gritAmount * (4.0f * intensity + density * 2.0f);

    // 2. Clarity (High harmonics)
    // Dynamically shaped to avoid amplifying harsh frequencies identified by the Timbre detector.
    float clarityDrive = 1.0f + clarityAmount * 3.0f * (1.0f - timbre);

    // The Clarity path is high-passed to stay in the "Air" region
    *clarityHPF.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 6000.0f);

    // sidechainBuffer is pre-allocated in prepare
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numChannels; ++i)
        sidechainBuffer.copyFrom(i, 0, buffer.getReadPointer(i), numSamples);

    // Process Clarity path
    juce::dsp::AudioBlock<float> block(sidechainBuffer.getArrayOfWritePointers(), numChannels, numSamples);
    juce::dsp::ProcessContextReplacing<float> context(block);
    clarityHPF.process(context);

    for (int i = 0; i < numChannels; ++i)
        sidechainBuffer.applyGain(i, 0, numSamples, clarityDrive);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* mainData = buffer.getWritePointer(channel);
        auto* sideData = sidechainBuffer.getReadPointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // Apply Grit to main signal
            float input = mainData[sample] * gritDrive;
            mainData[sample] = std::tanh(input);

            // Add Clarity harmonics (soft clipped)
            float clarity = std::tanh(sideData[sample]);
            mainData[sample] += clarity * clarityAmount * 0.3f;
        }
    }
}
