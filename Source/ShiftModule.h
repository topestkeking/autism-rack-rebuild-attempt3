/*
  ==============================================================================

    ShiftModule.h - Finalized Design
    Created: 27 Dec 2025 4:05:00pm
    Author:  Jules

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PressureDetector.h"

class ShiftModule
{
public:
    ShiftModule();
    ~ShiftModule();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(juce::AudioBuffer<float>& buffer, const PressureDetector& detector);

    float pitchShift = 0.0f; // -12 to +12
    float formantShift = 0.0f; // -12 to +12

private:
    double sampleRate = 44100.0;

    // A simple delay-line based pitch shifter for "weirdness"
    struct DelayLine {
        juce::AudioBuffer<float> buffer;
        int writePos = 0;
        float readPos = 0.0f;

        void setup(int size) {
            buffer.setSize(1, size);
            buffer.clear();
        }

        void write(float sample) {
            buffer.setSample(0, writePos, sample);
            writePos = (writePos + 1) % buffer.getNumSamples();
        }

        float read(float offset) {
            float pos = (float)writePos - offset;
            while (pos < 0) pos += (float)buffer.getNumSamples();

            int i1 = (int)pos % buffer.getNumSamples();
            int i2 = (i1 + 1) % buffer.getNumSamples();
            float frac = pos - (float)((int)pos);

            return buffer.getSample(0, i1) * (1.0f - frac) + buffer.getSample(0, i2) * frac;
        }
    };

    std::vector<DelayLine> delayLines;
    float phase = 0.0f;
};
