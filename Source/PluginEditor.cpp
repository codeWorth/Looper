/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
LooperAudioProcessorEditor::LooperAudioProcessorEditor (LooperAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p) {

    for (int i = 0; i < nLoops; i++) {
        setupSlider(volumeSliders[i], volumeSliderAttachments[i], juce::String(i + 1), labels[i]);
    }

    for (int i = 0; i < loopLenInBeats; i++) {
        beatIndicators[i].setText("", juce::dontSendNotification);
        beatIndicators[i].setColour(juce::Label::outlineColourId, juce::Colours::whitesmoke);
        addAndMakeVisible(beatIndicators[i]);
    }

    serverIndicator.setText("", juce::dontSendNotification);
    addAndMakeVisible(serverIndicator);

    startTimerHz(30);

    setSize((nLoops-1)*120 + 100 + 40, 410);
}

LooperAudioProcessorEditor::~LooperAudioProcessorEditor() {
}

//==============================================================================
void LooperAudioProcessorEditor::paint (juce::Graphics& g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void LooperAudioProcessorEditor::resized() {
    for (int i = 0; i < nLoops; i++) {
        volumeSliders[i].setBounds(20 + 120 * i, 70, 100, 200);
        labels[i].setBounds(40 + 120 * i, 20, 60, 40);
    }

    int indSize = (getWidth() - 40) / loopLenInBeats;
    for (int i = 0; i < loopLenInBeats; i++) {
        beatIndicators[i].setBounds(20 + indSize * i, 310, indSize - 4, 80);
    }

    serverIndicator.setBounds(getWidth() - 20, 0, 20, 20);
}

void LooperAudioProcessorEditor::setupSlider(
    DecibelSlider& slider,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment, 
    const juce::String& paramNumber,
    juce::Label& label
) {
    slider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 60, 30);
    addAndMakeVisible(slider);

    label.setText("Loop " + paramNumber, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.valueTree,
        "VOLUME" + paramNumber,
        slider
    );
}

void LooperAudioProcessorEditor::timerCallback() {
    drawRecording();
    drawBeat();
    
    if (audioProcessor.isServer()) {
        serverIndicator.setText("S", juce::dontSendNotification);
    } else {
        serverIndicator.setText("", juce::dontSendNotification);
    }
}

void LooperAudioProcessorEditor::drawRecording() {
    if (prevRecording == audioProcessor.recordingIndex) return;

    if (audioProcessor.recordingIndex == -1) {
        for (int i = 0; i < nLoops; i++) {
            labels[i].setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        }
    } else {
        if (prevRecording != -1) labels[prevRecording].setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        labels[audioProcessor.recordingIndex].setColour(juce::Label::backgroundColourId, juce::Colours::palevioletred);
    }

    prevRecording = audioProcessor.recordingIndex;
}

void LooperAudioProcessorEditor::drawBeat() {
    if (prevBeat == audioProcessor.beat) return;

    if (audioProcessor.beat == -1) {
        for (int i = 0; i < loopLenInBeats; i++) {
            beatIndicators[i].setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        }
    } else {
        if (prevBeat != -1) beatIndicators[prevBeat].setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        beatIndicators[audioProcessor.beat].setColour(juce::Label::backgroundColourId, juce::Colours::whitesmoke);
    }

    prevBeat = audioProcessor.beat;
}