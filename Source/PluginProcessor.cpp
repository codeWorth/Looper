/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstring>

//==============================================================================
LooperAudioProcessor::LooperAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), 
    valueTree(*this, nullptr, "Parameters", createParameters())
#endif
{
    nSamples = 1024;
    tempBuffer1 = new float[nSamples];
    tempBuffer2 = new float[nSamples];
}

LooperAudioProcessor::~LooperAudioProcessor() {
    delete[] tempBuffer1;
    delete[] tempBuffer2;
}

//==============================================================================
const juce::String LooperAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool LooperAudioProcessor::acceptsMidi() const {
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LooperAudioProcessor::producesMidi() const {
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LooperAudioProcessor::isMidiEffect() const {
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LooperAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int LooperAudioProcessor::getNumPrograms() {
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int LooperAudioProcessor::getCurrentProgram() {
    return 0;
}

void LooperAudioProcessor::setCurrentProgram (int index) {
}

const juce::String LooperAudioProcessor::getProgramName (int index) {
    return {};
}

void LooperAudioProcessor::changeProgramName (int index, const juce::String& newName) {
}

//==============================================================================
void LooperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    setupTempBuffers(samplesPerBlock);
}

void LooperAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LooperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void LooperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    if (nSamples != buffer.getNumSamples()) {
        setupTempBuffers(buffer.getNumSamples());
    }

    midiMessages.clear();

    auto playhead = getPlayHead();
    if (playhead == nullptr) {
        buffer.clear();
        return;
    }

    bool playing = false;
    auto info = playhead->getPosition();
    playing = info->getIsPlaying();

    if (!playing) {
        buffer.clear();
        for (int i = 0; i < nLoops; i++) loopDown[i] = false;
        recordingIndex = -1;
        return;
    }

    auto bpm = info->getBpm().orFallback(120);
    auto sampleRate = getSampleRate();
    size_t samplesPerBeat = ceil(sampleRate * 60.0 / bpm);
    if (samplesPerBeat != this->samplesPerBeat) {
        setupLoops(samplesPerBeat);
    }

    for (int i = 0; i < nLoops; i++) {
        if (!loopDown[i]) continue;
        loopDown[i] = false;
        
        if (recordingIndex == i) {
            recordingIndex = -1;
        } else {
            recordingIndex = i;
            nextLoopL.setupCopy(loopsL + recordingIndex);
            nextLoopR.setupCopy(loopsR + recordingIndex);
        }
    }

    auto samples = info->getTimeInSamples().orFallback(0);

    readWriteLoops(loopsL, nextLoopL, samples, buffer.getReadPointer(0), tempBuffer1);
    std::memcpy(buffer.getWritePointer(0), tempBuffer1, nSamples * sizeof(float));

    readWriteLoops(loopsR, nextLoopR, samples, buffer.getReadPointer(1), tempBuffer1);
    std::memcpy(buffer.getWritePointer(1), tempBuffer1, nSamples * sizeof(float));
}

void LooperAudioProcessor::readWriteLoops(Loop<float> loops[], CopyLoop<float>& tempLoop, size_t currentSample, const float* readBuffer, float* outBuffer) {
    std::fill_n(outBuffer, nSamples, 0.f);
    for (int j = 0; j < nLoops; j++) {
        if (recordingIndex == j) {
            tempLoop.writeBuffer(readBuffer, currentSample, nSamples);
            std::memcpy(tempBuffer2, readBuffer, sizeof(float) * nSamples);
        } else {
            loops[j].readBuffer(tempBuffer2, currentSample, nSamples);
        }

        for (int i = 0; i < nSamples; i++) {
            outBuffer[i] += tempBuffer2[i];
        }
    }    
}

void LooperAudioProcessor::setupLoops(size_t samplesPerBeat) {
    this->samplesPerBeat = samplesPerBeat;

    for (int i = 0; i < nLoops; i++) {
        loopsL[i].setLength(samplesPerBeat, loopLenInBeats, 0.f);
        loopsR[i].setLength(samplesPerBeat, loopLenInBeats, 0.f);
    }

    nextLoopL.setLength(samplesPerBeat, loopLenInBeats, 0.f);
    nextLoopR.setLength(samplesPerBeat, loopLenInBeats, 0.f);
}

//==============================================================================
bool LooperAudioProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* LooperAudioProcessor::createEditor() {
    return new LooperAudioProcessorEditor (*this);
}

//==============================================================================
void LooperAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void LooperAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new LooperAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout LooperAudioProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterBool>("LOOP1", "Loop1", false));
    params.back().get()->addListener(this);
    params.push_back(std::make_unique<juce::AudioParameterBool>("LOOP2", "Loop2", false));
    params.back().get()->addListener(this);
    params.push_back(std::make_unique<juce::AudioParameterBool>("LOOP3", "Loop3", false));
    params.back().get()->addListener(this);
    params.push_back(std::make_unique<juce::AudioParameterBool>("LOOP4", "Loop4", false));
    params.back().get()->addListener(this);
    params.push_back(std::make_unique<juce::AudioParameterBool>("LOOP5", "Loop5", false));
    params.back().get()->addListener(this);
    params.push_back(std::make_unique<juce::AudioParameterBool>("LOOP6", "Loop6", false));
    params.back().get()->addListener(this);

    params.push_back(std::make_unique<juce::AudioParameterFloat>("VOLUME1", "Volume1", -100.f, 0.f, 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("VOLUME2", "Volume2", -100.f, 0.f, 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("VOLUME3", "Volume3", -100.f, 0.f, 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("VOLUME4", "Volume4", -100.f, 0.f, 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("VOLUME5", "Volume5", -100.f, 0.f, 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("VOLUME6", "Volume6", -100.f, 0.f, 0.f));

    return { params.begin(), params.end() };
}

void LooperAudioProcessor::setupTempBuffers(int len) {
    delete[] tempBuffer1;
    delete[] tempBuffer2;
    nSamples = len;
    tempBuffer1 = new float[nSamples];
    std::fill_n(tempBuffer1, nSamples, 0.f);
    tempBuffer2 = new float[nSamples];
    std::fill_n(tempBuffer2, nSamples, 0.f);
}

void LooperAudioProcessor::parameterValueChanged(int parameterIndex, float newValue) {
    loopDown[parameterIndex] = true;
}

void LooperAudioProcessor::parameterGestureChanged(int parameterIndex, bool gestureIsStarting) {}