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

    using namespace juce;

    for (int i = 0; i < nLoops; i++) {
        setupSlider(
            volumeSliders[i], monitorButtons[i], 
            volumeSliderAttachments[i], monitorButtonAttachments[i], 
            String(i + 1), labels[i]
        );
        addAndMakeVisible(meters[i]);
    }

    addAndMakeVisible(inputMeter);
    muteInput.setButtonText("M");
    muteInput.setClickingTogglesState(true);
    muteInput.setColour(TextButton::buttonOnColourId, Colours::palevioletred);
    addAndMakeVisible(muteInput);
    inputLabel.setJustificationType(Justification::centred);
    inputLabel.setText("Input", dontSendNotification);
    addAndMakeVisible(inputLabel);

    muteInputAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.valueTree,
        "MUTEINPUT",
        muteInput
    );

    for (int i = 0; i < loopLenInBeats; i++) {
        beatIndicators[i].setText("", dontSendNotification);
        beatIndicators[i].setColour(Label::outlineColourId, Colours::whitesmoke);
        addAndMakeVisible(beatIndicators[i]);
    }

    startTimerHz(30);


    setSize((nLoops-1)*120 + 100 + 40 + loopsX, 410);
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
        volumeSliders[i].setBounds(loopsX + 5 + 120 * i, 70, 100, 200);
        labels[i].setBounds(loopsX + 40 + 120 * i, 20, 60, 40);
        meters[i].setBounds(loopsX + 80 + 120 * i, 95, 15, 135);
        monitorButtons[i].get()->setBounds(loopsX + 75 + 120 * i, 66, 25, 25);
    }

    inputLabel.setBounds(10, 20, 60, 40);
    inputMeter.setBounds(32, 95, 15, 135);
    muteInput.setBounds(27, 66, 25, 25);

    int indSize = (getWidth() - 40) / loopLenInBeats;
    for (int i = 0; i < loopLenInBeats; i++) {
        beatIndicators[i].setBounds(20 + indSize * i, 310, indSize - 4, 80);
    }
}

void LooperAudioProcessorEditor::setupSlider(
    DecibelSlider& slider,
    std::unique_ptr<HeadphonesButton>& monitorButton,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment, 
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& monitorAttachment,
    const juce::String& paramNumber,
    juce::Label& label
) {
    using namespace juce;

    slider.setSliderStyle(Slider::SliderStyle::LinearVertical);
    slider.setTextBoxStyle(Slider::TextBoxBelow, true, 60, 30);
    addAndMakeVisible(slider);

    label.setText("Loop " + paramNumber, dontSendNotification);
    label.setJustificationType(Justification::centred);
    addAndMakeVisible(label);

    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.valueTree,
        "VOLUME" + paramNumber,
        slider
    );

    monitorButton = std::make_unique<HeadphonesButton>("MONITOR" + paramNumber);
    addAndMakeVisible(monitorButton.get());
    monitorAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.valueTree,
        "MONITOR" + paramNumber,
        *monitorButton.get()
    );
}

void LooperAudioProcessorEditor::timerCallback() {
    drawRecording();
    drawBeat();
    drawMeters();
    clearMonitoring();
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

void LooperAudioProcessorEditor::drawMeters() {
    for (int i = 0; i < nLoops; i++) {
        VerticalMeter& meter = meters[i];
        float rms = audioProcessor.getRMS(i);
        float levelInDb = juce::Decibels::gainToDecibels(rms, meter.MIN_LEVEL);
        if (levelInDb < meter.MIN_LEVEL + 1) levelInDb = meter.MIN_LEVEL;

        meter.setLevel(levelInDb);
        meter.repaint();
    }

    VerticalMeter& meter = inputMeter;
    float rms = audioProcessor.getInputRMS();
    float levelInDb = juce::Decibels::gainToDecibels(rms, meter.MIN_LEVEL);
    if (levelInDb < meter.MIN_LEVEL + 1) levelInDb = meter.MIN_LEVEL;

    meter.setLevel(levelInDb);
    meter.repaint();
}

void LooperAudioProcessorEditor::clearMonitoring() {
    if (prevMonitoring == audioProcessor.monitorIndex || audioProcessor.monitorIndex == -1 || prevMonitoring == -1) {
        prevMonitoring = audioProcessor.monitorIndex;
        return;
    }

    auto& prevButton = *monitorButtons[prevMonitoring].get();
    if (prevButton.getToggleState()) {
        prevButton.setToggleState(false, juce::NotificationType::sendNotification);
    }

    prevMonitoring = audioProcessor.monitorIndex;
}