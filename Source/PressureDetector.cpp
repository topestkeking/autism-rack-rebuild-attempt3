/*
  ==============================================================================

    PressureDetector.cpp - Finalized Design
    Created: 27 Dec 2025 3:45:00pm
    Author:  Jules

  ==============================================================================
*/

#include "PressureDetector.h"

PressureDetector::PressureDetector()
{
}

PressureDetector::~PressureDetector()
{
}

void PressureDetector::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    lowFilter.prepare(monoSpec);
    midFilter.prepare(monoSpec);
    highFilter.prepare(monoSpec);

    // Low band for "Density" (Fundamental weight ~100-400Hz)
    *lowFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 350.0f);

    // Mid band for "Timbre" analysis (Harshness/Presence ~2.5kHz-5kHz)
    *midFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 3500.0f, 0.4f);

    // High band for "Harshness/Air" (>6kHz)
    *highFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 7000.0f);

    smoothedIntensity.reset(sampleRate, 0.05);
    smoothedDensity.reset(sampleRate, 0.1);
    smoothedTimbre.reset(sampleRate, 0.1);

    monoBuffer.setSize(1, spec.maximumBlockSize);
    analysisBuffer.setSize(1, spec.maximumBlockSize);
}

void PressureDetector::process(const juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>* sidechain)
{
    int numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;

    // Use sidechain for analysis if available, otherwise use input buffer
    const juce::AudioBuffer<float>& analysisSource = (sidechain != nullptr && sidechain->getNumSamples() >= numSamples) ? *sidechain : buffer;

    // 1. Intensity: Overall RMS
    float rawIntensity = analysisSource.getRMSLevel(0, 0, numSamples);
    smoothedIntensity.setTargetValue(juce::jlimit(0.0f, 1.0f, rawIntensity * 2.0f)); // Normalized/boosted
    intensity = smoothedIntensity.getNextValue();

    // Use pre-allocated monoBuffer for spectral analysis
    monoBuffer.copyFrom(0, 0, analysisSource, 0, 0, numSamples);
    if (analysisSource.getNumChannels() > 1) {
        monoBuffer.addFrom(0, 0, analysisSource, 1, 0, numSamples);
        monoBuffer.applyGain(0.5f);
    }

    // 2. Density: Low energy vs mid/high energy
    analysisBuffer.copyFrom(0, 0, monoBuffer, 0, 0, numSamples);

    juce::dsp::AudioBlock<float> block(analysisBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowFilter.process(context);

    float lowRMS = analysisBuffer.getRMSLevel(0, 0, numSamples);
    float totalRMS = monoBuffer.getRMSLevel(0, 0, numSamples) + 0.0001f;

    // Density is the ratio of low-frequency energy to total energy
    smoothedDensity.setTargetValue(juce::jlimit(0.0f, 1.0f, lowRMS / totalRMS));
    density = smoothedDensity.getNextValue();

    // 3. Timbre: High-mid harshness
    analysisBuffer.copyFrom(0, 0, monoBuffer, 0, 0, numSamples);

    juce::dsp::AudioBlock<float> midBlock(analysisBuffer);
    juce::dsp::ProcessContextReplacing<float> midContext(midBlock);
    midFilter.process(midContext);

    float midRMS = analysisBuffer.getRMSLevel(0, 0, numSamples);

    // Timbre here represents the presence of harsh resonant energy in the mids
    smoothedTimbre.setTargetValue(juce::jlimit(0.0f, 1.0f, midRMS / (totalRMS * 0.5f)));
    timbre = smoothedTimbre.getNextValue();
}

float PressureDetector::getIntensity() const { return intensity; }
float PressureDetector::getDensity() const   { return density; }
float PressureDetector::getTimbre() const    { return timbre; }
