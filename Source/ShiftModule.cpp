/*
  ==============================================================================

    ShiftModule.cpp - Finalized Design
    Created: 27 Dec 2025 4:05:00pm
    Author:  Jules

  ==============================================================================
*/

#include "ShiftModule.h"

ShiftModule::ShiftModule()
{
}

ShiftModule::~ShiftModule()
{
}

void ShiftModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    delayLines.resize(spec.numChannels);
    for (auto& dl : delayLines)
        dl.setup(8192);
}

void ShiftModule::process(juce::AudioBuffer<float>& buffer, const PressureDetector& detector)
{
    float intensity = detector.getIntensity();

    // The "Demonic Bloom": The shift module reacts to the performance.
    // We set a base formant shift, and as intensity increases, it "blooms" further down (demonic).
    // The README mentions a specific -5 semitone bloom target on screams.
    float bloomAmount = intensity * 5.0f;
    float dynamicFormant = formantShift - bloomAmount;
    float dynamicPitch = pitchShift;

    // Map semitones to ratio
    float ratio = std::pow(2.0f, (dynamicPitch + dynamicFormant) / 12.0f);

    // We use a small delay range to keep it "unstable" and gritty as requested.
    float delayRange = 400.0f; // samples

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        phase += (1.0f - ratio);
        if (phase >= delayRange) phase -= delayRange;
        if (phase < 0) phase += delayRange;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float input = buffer.getSample(channel, sample);
            delayLines[channel].write(input);

            // Simple dual-tap crossfade to hide the wrap-around (slightly)
            float out1 = delayLines[channel].read(phase);
            float out2 = delayLines[channel].read(std::fmod(phase + delayRange * 0.5f, delayRange));

            float fade = std::abs((phase / delayRange) - 0.5f) * 2.0f;
            float output = out1 * fade + out2 * (1.0f - fade);

            buffer.setSample(channel, sample, output);
        }
    }
}
