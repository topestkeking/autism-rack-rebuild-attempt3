/*
  ==============================================================================

    SpaceModule.h - Finalized Design
    Created: 27 Dec 2025 4:10:00pm
    Author:  Jules

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PressureDetector.h"

class SpaceModule
{
public:
    SpaceModule();
    ~SpaceModule();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(juce::AudioBuffer<float>& buffer, const PressureDetector& detector);

    float mixAmount = 0.5f;
    float characterAmount = 0.5f; // 0: Room, 0.5: Plate, 1.0: Bloom

private:
    double sampleRate = 44100.0;
    juce::Reverb reverb;

    juce::LinearSmoothedValue<float> smoothedWet { 0.0f };
};
