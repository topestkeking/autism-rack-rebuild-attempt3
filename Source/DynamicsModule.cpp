/*
  ==============================================================================

    DynamicsModule.cpp - Finalized Design
    Created: 27 Dec 2025 3:50:00pm
    Author:  Jules

  ==============================================================================
*/

#include "DynamicsModule.h"

DynamicsModule::DynamicsModule()
{
}

DynamicsModule::~DynamicsModule()
{
}

void DynamicsModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    smoothedGain.reset(sampleRate, 0.01); // Fast response for dynamics
}

void DynamicsModule::process(juce::AudioBuffer<float>& buffer, const PressureDetector& detector)
{
    float intensity = detector.getIntensity();
    float density = detector.getDensity();

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float targetGain = 1.0f;

        // Omnipressor-style morphing: Gate -> Expand -> Compress -> Invert
        if (functionAmount < 0.25f) // Gating / De-reverb (0.0 to 0.25)
        {
            float morph = functionAmount * 4.0f; // 0 to 1
            float gateThreshold = 0.15f * (1.0f - morph);
            float gateGain = (intensity > gateThreshold) ? 1.0f : (1.0f - sustainCut);
            float expandGain = 0.5f + (intensity * 1.5f);
            targetGain = gateGain * (1.0f - morph) + expandGain * morph;
        }
        else if (functionAmount < 0.5f) // Expansion to Linear/Compression (0.25 to 0.5)
        {
            float morph = (functionAmount - 0.25f) * 4.0f; // 0 to 1
            float expandGain = 0.5f + (intensity * 1.5f);
            float compressGain = 1.0f / (1.0f + (intensity * 2.0f));
            targetGain = expandGain * (1.0f - morph) + compressGain * morph;
        }
        else if (functionAmount < 0.75f) // Compression to Heavy Compression (0.5 to 0.75)
        {
            float morph = (functionAmount - 0.5f) * 4.0f; // 0 to 1
            float compressGain = 1.0f / (1.0f + (intensity * 2.0f));
            float heavyCompressGain = 1.0f / (1.0f + (intensity * 8.0f));
            targetGain = compressGain * (1.0f - morph) + heavyCompressGain * morph;
        }
        else // Compression to Inversion (0.75 to 1.0)
        {
            float morph = (functionAmount - 0.75f) * 4.0f; // 0 to 1
            float heavyCompressGain = 1.0f / (1.0f + (intensity * 8.0f));
            float invertGain = 1.0f - (intensity * 2.5f);
            targetGain = heavyCompressGain * (1.0f - morph) + invertGain * morph;
            if (targetGain < -0.8f) targetGain = -0.8f; // Aggressive clipping
        }

        // Apply Density influence: heavier low-freq vocals get more control
        targetGain *= (1.0f - (density * 0.3f));

        smoothedGain.setTargetValue(targetGain);
        float currentGain = smoothedGain.getNextValue();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            buffer.setSample(channel, sample, buffer.getSample(channel, sample) * currentGain);
        }
    }
}
