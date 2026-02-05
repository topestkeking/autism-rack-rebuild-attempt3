/*
  ==============================================================================

    DynamicsModule.h - Finalized Design
    Created: 27 Dec 2025 3:50:00pm
    Author:  Jules

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PressureDetector.h"

class DynamicsModule
{
public:
    DynamicsModule();
    ~DynamicsModule();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(juce::AudioBuffer<float>& buffer, const PressureDetector& detector);

    // Parameters (would normally be in APVTS, but keeping it simple for now)
    float functionAmount = 0.5f; // 0.0 to 1.0 (Gate -> Inversion)
    float sustainCut = 0.5f;

private:
    double sampleRate = 44100.0;
    juce::LinearSmoothedValue<float> smoothedGain { 1.0f };

    float calculateGain(float inputLevel, float intensity, float density);
};
