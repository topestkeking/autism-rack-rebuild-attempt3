/*
  ==============================================================================

    WidenerModule.h - "THE VOID"
    Dynamic stereo widening linked to intensity.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PressureDetector.h"

class WidenerModule
{
public:
    WidenerModule() {}
    ~WidenerModule() {}

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        for (auto& d : delays)
        {
            d.prepare(spec);
            d.setMaximumDelayInSamples(spec.sampleRate * 0.05); // 50ms max
        }
    }

    void process(juce::AudioBuffer<float>& buffer, const PressureDetector& detector, float widthAmount)
    {
        if (buffer.getNumChannels() < 2) return;

        float intensity = detector.getIntensity();
        // Widening "blooms" with intensity
        float dynamicWidth = widthAmount * (0.2f + intensity * 0.8f);

        // Use a simple Haas effect for widening (delaying one side slightly)
        // or Mid-Side processing. Haas is more "unstable" and characterful.
        float delayMs = dynamicWidth * 25.0f; // up to 25ms
        int delaySamples = (int)(delayMs * sampleRate / 1000.0);

        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Simple Mid-Side widening
            float mid = (left[i] + right[i]) * 0.5f;
            float side = (left[i] - right[i]) * 0.5f;

            side *= (1.0f + dynamicWidth * 2.0f);

            left[i] = mid + side;
            right[i] = mid - side;
        }
    }

private:
    double sampleRate = 44100.0;
    std::vector<juce::dsp::DelayLine<float>> delays { 2 };
};
