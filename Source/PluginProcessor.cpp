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
                        .withOutput ("Monitor", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), 
    valueTree(*this, nullptr, "Parameters", createParameters()),
    loopSyncer(this), bufferStack(4), nSamples(1024)
#endif
{
    for (int i = 0; i < nLoops; i++) {
        loopDown[i] = false;
        loopVolumes[i] = 1.f;
    }

    setupParameterListeners();
}

LooperAudioProcessor::~LooperAudioProcessor() {}

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
    setupTempBuffers(buffer.getNumSamples());

    midiMessages.clear();
    if (buffer.getNumChannels() > 2) {
        std::fill_n(buffer.getWritePointer(2), buffer.getNumSamples(), 0.f);
        std::fill_n(buffer.getWritePointer(3), buffer.getNumSamples(), 0.f);
    }

    auto playhead = getPlayHead();
    if (playhead == nullptr) {
        return;
    }

    bool playing = false;
    auto info = playhead->getPosition();
    playing = info->getIsPlaying();

    loopSyncer.handleUpdates();

    inputRMS = (calculateRMS(buffer.getReadPointer(0), nSamples) + calculateRMS(buffer.getReadPointer(1), nSamples)) / 2.f;

    if (!playing) {
        for (int i = 0; i < nLoops; i++) {
            loopDown[i] = false;
            setRMS(i, 0);
        }

        recordingIndex = -1;
        beat = -1;
        return;
    }

    doLooping(buffer, info);
}

void LooperAudioProcessor::doLooping(juce::AudioBuffer<float>& buffer, juce::Optional<juce::AudioPlayHead::PositionInfo> info) {
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
    beat = (samples / samplesPerBeat) % loopLenInBeats;
    BufferStack<float>::Buffer tempBuffer(bufferStack);

    readWriteLoops(loopsL, nextLoopL, samples, buffer.getReadPointer(0), tempBuffer.get());
    std::memcpy(buffer.getWritePointer(0), tempBuffer.get(), nSamples * sizeof(float));

    readWriteLoops(loopsR, nextLoopR, samples, buffer.getReadPointer(1), tempBuffer.get());
    std::memcpy(buffer.getWritePointer(1), tempBuffer.get(), nSamples * sizeof(float));

    for (int i = 0; i < nLoops; i++) {
        float monoRMS = (calculateRMS(loopsL[i], samples, nSamples) + calculateRMS(loopsR[i], samples, nSamples)) / 2.f;
        setRMS(i, monoRMS);
    }

    if (monitorIndex != -1 && buffer.getNumChannels() > 2) {
        loopsL[monitorIndex].readBuffer(tempBuffer.get(), samples, nSamples);
        std::memcpy(buffer.getWritePointer(2), tempBuffer.get(), nSamples * sizeof(float));

        loopsR[monitorIndex].readBuffer(tempBuffer.get(), samples, nSamples);
        std::memcpy(buffer.getWritePointer(3), tempBuffer.get(), nSamples * sizeof(float));
    }
}

void LooperAudioProcessor::readWriteLoops(Loop<float> loops[], CopyLoop<float>& tempLoop, size_t currentSample, const float* readBuffer, float* outBuffer) {
    if (muteInput) {
        std::fill_n(outBuffer, nSamples, 0.f);
    } else {
        std::memcpy(outBuffer, readBuffer, nSamples * sizeof(float));
    }

    BufferStack<float>::Buffer tempBuffer(bufferStack);
    
    for (int j = 0; j < nLoops; j++) {
        if (recordingIndex == j) {
            tempLoop.writeBuffer(readBuffer, currentSample, nSamples);
            std::memcpy(tempBuffer.get(), readBuffer, nSamples * sizeof(float));
        } else {
            loops[j].readBuffer(tempBuffer.get(), currentSample, nSamples);
        }

        float loopVal = loopVolumes[j];
        float decibles = (loopVal - 1) * -minLoopDb;
        float gain = juce::Decibels::decibelsToGain(decibles, minLoopDb);
        for (int i = 0; i < nSamples; i++) {
            outBuffer[i] += gain * tempBuffer.get()[i];
        }
    }    
}

float LooperAudioProcessor::calculateRMS(const Loop<float>& loop, size_t currentSample, int nSamples) {
    BufferStack<float>::Buffer tempBuffer(bufferStack);
    loop.readBuffer(tempBuffer.get(), currentSample, nSamples);
    return calculateRMS(tempBuffer.get(), nSamples);
}

float LooperAudioProcessor::calculateRMS(const float* buffer, int nSamples) const {
    float meanSquared = 0.f;
    for (int i = 0; i < nSamples; i++) {
        meanSquared += buffer[i] * buffer[i];
    }
    meanSquared /= static_cast<float>(nSamples);

    return std::sqrt(meanSquared);
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
    using namespace juce;
    MemoryOutputStream stream;
    valueTree.copyState().writeToStream(stream);
    destData.replaceAll(stream.getData(), stream.getDataSize());
}

void LooperAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    using namespace juce;
    valueTree.replaceState(ValueTree::readFromData(data, sizeInBytes));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new LooperAudioProcessor();
}

void LooperAudioProcessor::startRecordLoop(int loopIndex) {
    if (recordingIndex == loopIndex) return; // if we're already recording loopIndex, do nothing
    loopDown[loopIndex] = true;
}

void LooperAudioProcessor::stopRecordLoop() {
    if (recordingIndex == -1) return; // if we're not recording anything, do nothing
    loopDown[recordingIndex] = true;
}

void LooperAudioProcessor::setLoopVolume(int loopIndex, float volume) {
    loopVolumes[loopIndex] = volume;
    valueTree.getParameter("VOLUME" + juce::String(loopIndex + 1))->setValueNotifyingHost(volume);
}

juce::AudioProcessorValueTreeState::ParameterLayout LooperAudioProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 0; i < nLoops; i++) {
        const juce::String number(i + 1);
        params.push_back(std::make_unique<juce::AudioParameterBool>("LOOP" + number, "Loop" + number, false));
    }

    for (int i = 0; i < nLoops; i++) {
        const juce::String number(i + 1);
        params.push_back(std::make_unique<juce::AudioParameterFloat>("VOLUME" + number, "Volume" + number, minLoopDb, 0.f, 0.f));
    }

    for (int i = 0; i < nLoops; i++) {
        const juce::String number(i + 1);
        params.push_back(std::make_unique<juce::AudioParameterBool>("MONITOR" + number, "Monitor" + number, false));
    }

    params.push_back(std::make_unique<juce::AudioParameterBool>("MUTEINPUT", "muteinput", false));

    return { params.begin(), params.end() };
}

void LooperAudioProcessor::setupParameterListeners() {
    for (int i = 0; i < nLoops; i++) {
        const juce::String number(i + 1);

        listeners.push_back(std::make_unique<ButtonListener>(loopDown[i], i, *this));
        valueTree.getParameter("LOOP" + number)->addListener(listeners.back().get());

        listeners.push_back(std::make_unique<VolumeListener>(loopVolumes[i], i, *this));
        valueTree.getParameter("VOLUME" + number)->addListener(listeners.back().get());

        listeners.push_back(std::make_unique<ToggleButtonListener>(i, *this));
        valueTree.getParameter("MONITOR" + number)->addListener(listeners.back().get());
    }

    listeners.push_back(std::make_unique<MuteInputListener>(*this));
    valueTree.getParameter("MUTEINPUT")->addListener(listeners.back().get());
}

void LooperAudioProcessor::setupTempBuffers(int len) {
    nSamples = len;
    bufferStack.setupBuffersIfNeeded(len, 0.f);
}

float LooperAudioProcessor::getRMS(int loopIndex) const {
    return loopRMSs[loopIndex];
}

void LooperAudioProcessor::setRMS(int loopIndex, float value) {
    loopRMSs[loopIndex] = value;
}

float LooperAudioProcessor::getInputRMS() const {
    return inputRMS;
}