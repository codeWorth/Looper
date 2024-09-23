#pragma once

#include "JuceHeader.h"
#include "LoopSyncer.h"

class ButtonListener : public juce::AudioProcessorParameter::Listener {
public:
    ButtonListener(bool& pressed) : pressed(pressed) {};
    ~ButtonListener() {};

    bool& pressed;

    void parameterValueChanged(int parameterIndex, float newValue) override {
        pressed = true;
    }

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
};

struct VolumeListener : juce::AudioProcessorParameter::Listener {

    VolumeListener(float& volume, int loopIndex, LoopSyncer& loopSyncer) : 
        volume(volume), loopIndex(loopIndex), loopSyncer(loopSyncer) {};
    ~VolumeListener() {};

    float& volume;
    int loopIndex;
    LoopSyncer& loopSyncer;

    void parameterValueChanged(int parameterIndex, float newValue) override {
        volume = newValue;
        loopSyncer.setLoopVolume(loopIndex, newValue);
    }

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
};