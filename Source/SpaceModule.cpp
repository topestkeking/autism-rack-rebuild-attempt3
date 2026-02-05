/*
  ==============================================================================

    SpaceModule.cpp - Finalized Design
    Created: 27 Dec 2025 4:10:00pm
    Author:  Jules

  ==============================================================================
*/

#include "SpaceModule.h"

SpaceModule::SpaceModule()
{
}

SpaceModule::~SpaceModule()
{
}

void SpaceModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    reverb.setSampleRate(sampleRate);
    smoothedWet.reset(sampleRate, 0.05);
}

void SpaceModule::process(juce::AudioBuffer<float>& buffer, const PressureDetector& detector)
{
    float intensity = detector.getIntensity();

    // Reverb parameters morphing: Room -> Plate -> Bloom
    juce::Reverb::Parameters params;

    float bloom = characterAmount * intensity; // Explosive growth when loud and char is high

    if (characterAmount < 0.5f) // Room towards Plate
    {
        float morph = characterAmount * 2.0f;
        params.roomSize = 0.1f * (1.0f - morph) + 0.6f * morph;
        params.damping = 0.8f * (1.0f - morph) + 0.3f * morph;
    }
    else // Plate towards Bloom
    {
        float morph = (characterAmount - 0.5f) * 2.0f;
        params.roomSize = 0.6f * (1.0f - morph) + 1.0f * morph;
        params.damping = 0.3f * (1.0f - morph) + 0.1f * morph;
    }

    // Add intensity "Bloom"
    params.roomSize = juce::jlimit(0.0f, 1.0f, params.roomSize + bloom * 0.5f);
    params.width = 1.0f;

    // Auto-ducking: High intensity pushes reverb down initially to keep transients,
    // then it swells as intensity drops (modeled by smoothing)
    float ducking = 1.0f - (intensity * 0.5f);
    float targetWet = mixAmount * ducking * (1.0f + bloom);

    smoothedWet.setTargetValue(juce::jlimit(0.0f, 1.0f, targetWet));
    params.wetLevel = smoothedWet.getNextValue();
    params.dryLevel = 1.0f;

    reverb.setParameters(params);

    if (buffer.getNumChannels() == 1)
    {
        reverb.processMono(buffer.getWritePointer(0), buffer.getNumSamples());
    }
    else if (buffer.getNumChannels() == 2)
    {
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());
    }
}
