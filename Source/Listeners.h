#pragma once

#include "JuceHeader.h"

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

    VolumeListener(float& volume) : volume(volume) {};
    ~VolumeListener() {};

    float& volume;

    void parameterValueChanged(int parameterIndex, float newValue) override {
        volume = newValue;
    }

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
};