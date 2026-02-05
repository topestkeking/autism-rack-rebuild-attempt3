/*
  ==============================================================================

    EQModule.cpp - Finalized Design
    Created: 27 Dec 2025 3:55:00pm
    Author:  Jules

  ==============================================================================
*/

#include "EQModule.h"

EQModule::EQModule()
{
}

EQModule::~EQModule()
{
}

void EQModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    scoopFilter.prepare(spec);
    biteFilter.prepare(spec);
}

void EQModule::process(juce::AudioBuffer<float>& buffer, const PressureDetector& detector)
{
    float density = detector.getDensity();
    float timbre = detector.getTimbre();

    // 1. Dynamic Scoop (Low-Mid Mud Removal) - The "Auto-Engineer"
    // Deepen scoop when density is high (thick low vocals) or timbre shows muddy resonance
    float autoScoop = scoopAmount * (0.4f + density * 0.6f + timbre * 0.3f);
    float scoopGain = juce::Decibels::decibelsToGain(-32.0f * juce::jlimit(0.0f, 1.0f, autoScoop));
    *scoopFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 300.0f, 0.8f, scoopGain);

    // 2. Dynamic Bite (High-Mid Aggression)
    // Increase bite for intelligibility, but intelligently ease off if the detector hears piercing harshness
    float autoBite = biteAmount * (1.2f - timbre);
    float biteGain = juce::Decibels::decibelsToGain(18.0f * juce::jlimit(0.0f, 1.0f, autoBite));
    *biteFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 3200.0f, 0.6f, biteGain);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    scoopFilter.process(context);
    biteFilter.process(context);
}
