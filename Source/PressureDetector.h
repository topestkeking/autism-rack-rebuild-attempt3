/*
  ==============================================================================

    PressureDetector.h - Finalized Design
    Created: 27 Dec 2025 3:45:00pm
    Author:  Jules

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class PressureDetector
{
public:
    PressureDetector();
    ~PressureDetector();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(const juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>* sidechain = nullptr);

    float getIntensity() const;
    float getDensity() const;
    float getTimbre() const;

private:
    float intensity = 0.0f;
    float density = 0.0f;
    float timbre = 0.0f;

    double sampleRate = 44100.0;

    // Filters for spectral analysis
    juce::dsp::IIR::Filter<float> lowFilter;
    juce::dsp::IIR::Filter<float> midFilter;
    juce::dsp::IIR::Filter<float> highFilter;

    juce::LinearSmoothedValue<float> smoothedIntensity { 0.0f };
    juce::LinearSmoothedValue<float> smoothedDensity   { 0.0f };
    juce::LinearSmoothedValue<float> smoothedTimbre    { 0.0f };

    juce::AudioBuffer<float> monoBuffer;
    juce::AudioBuffer<float> analysisBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PressureDetector)
};
