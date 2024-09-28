/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include "DecibelSlider.h"
#include "VerticalMeter.h"
#include "HeadphonesButton.h"

//==============================================================================
/**
*/
class LooperAudioProcessorEditor : 
    public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    LooperAudioProcessorEditor (LooperAudioProcessor&);
    ~LooperAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;

private:
    void setupSlider(
        DecibelSlider& slider, 
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment, 
        const juce::String& paramNumber,
        juce::Label& label
    );

    void drawRecording();
    void drawBeat();
    void drawMeters();
    void clearMonitoring();

    LooperAudioProcessor& audioProcessor;

    juce::Label labels[nLoops];
    DecibelSlider volumeSliders[nLoops];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeSliderAttachments[nLoops];
    VerticalMeter meters[nLoops];
    std::unique_ptr<HeadphonesButton> monitorButtons[nLoops];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monitorButtonAttachments[nLoops];

    juce::Label beatIndicators[loopLenInBeats];

    int prevRecording = -1;
    int prevBeat = -1;
    int prevMonitoring = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperAudioProcessorEditor)
};
