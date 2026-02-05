/*
  ==============================================================================

    EQModule.h - Finalized Design
    Created: 27 Dec 2025 3:55:00pm
    Author:  Jules

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PressureDetector.h"

class EQModule
{
public:
    EQModule();
    ~EQModule();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(juce::AudioBuffer<float>& buffer, const PressureDetector& detector);

    float scoopAmount = 0.5f;
    float biteAmount = 0.5f;

private:
    double sampleRate = 44100.0;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> scoopFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> biteFilter;
};
