/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CopyLoop.h"
#include "LoopSyncer.h"

constexpr int nLoops = 6;
constexpr int loopLenInBeats = 8;
constexpr float minLoopDb = -30.f;

//==============================================================================
/**
*/
class LooperAudioProcessor : 
    public juce::AudioProcessor, 
    public LoopSyncer::MessageListener
{
public:
    //==============================================================================
    LooperAudioProcessor();
    ~LooperAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void doLooping(juce::AudioBuffer<float>& buffer, juce::Optional<juce::AudioPlayHead::PositionInfo> info);

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void startRecordLoop(int loopIndex) override;
    void stopRecordLoop() override;
    void setLoopVolume(int loopIndex, float volume) override;

    juce::AudioProcessorValueTreeState valueTree;

    int recordingIndex = -1;
    int beat = -1;
    bool isServer() const;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void setupParameterListeners();
    void setupTempBuffers(int len);
    void readWriteLoops(Loop<float> loops[], CopyLoop<float>& tempLoop, size_t currentSample, const float* readBuffer, float* outBuffer);
    void setupLoops(size_t samplesPerBeat);
    
    bool loopDown[nLoops];
    float loopVolumes[nLoops];

    size_t samplesPerBeat = 0;
    Loop<float> loopsL[nLoops];
    Loop<float> loopsR[nLoops];
    size_t readIndex[nLoops];
    float* tempBuffer1;
    float* tempBuffer2;
    int nSamples;

    CopyLoop<float> nextLoopL;
    CopyLoop<float> nextLoopR;

    std::vector<std::unique_ptr<juce::AudioProcessorParameter::Listener>> listeners;
    LoopSyncer loopSyncer;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperAudioProcessor)
};
