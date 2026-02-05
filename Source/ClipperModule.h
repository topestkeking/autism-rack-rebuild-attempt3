/*
  ==============================================================================

    ClipperModule.h - "THE WALL"
    Aggressive final stage clipper and limiter.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PressureDetector.h"

class ClipperModule
{
public:
    ClipperModule() {}
    ~ClipperModule() {}

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        dcBlocker.prepare(spec);
        *dcBlocker.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 20.0f);
        sampleRate = spec.sampleRate;
    }

    void process(juce::AudioBuffer<float>& buffer, float drive, float ceiling)
    {
        float gain = juce::Decibels::decibelsToGain(drive);
        float limit = juce::Decibels::decibelsToGain(ceiling);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* data = buffer.getWritePointer(channel);

            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                float x = data[sample] * gain;

                // Soft clipping transition into hard clipping
                if (std::abs(x) > limit)
                    x = (x > 0) ? limit : -limit;
                else if (std::abs(x) > limit * 0.7f)
                    x = x - (0.1f * std::pow(x, 3.0f)); // Subtle cubic saturation

                data[sample] = x;
            }
        }

        // Block DC offset that might build up from asymmetric clipping
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        dcBlocker.process(context);
    }

private:
    double sampleRate = 44100.0;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> dcBlocker;
};
