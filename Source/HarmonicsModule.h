/*
  ==============================================================================

    HarmonicsModule.h - Finalized Design
    Created: 27 Dec 2025 4:00:00pm
    Author:  Jules

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PressureDetector.h"

class HarmonicsModule
{
public:
    HarmonicsModule();
    ~HarmonicsModule();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(juce::AudioBuffer<float>& buffer, const PressureDetector& detector);

    float gritAmount = 0.5f;
    float clarityAmount = 0.5f;

private:
    double sampleRate = 44100.0;

    // High-pass filter for clarity harmonics
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> clarityHPF;

    juce::AudioBuffer<float> sidechainBuffer;
};
