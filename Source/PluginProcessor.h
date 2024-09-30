/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CopyLoop.h"
#include "LoopSyncer.h"
#include "Constants.h"
#include "BufferStack.h"

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
    int monitorIndex = -1;
    float getRMS(int loopIndex) const;
    float getInputRMS() const;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void setupParameterListeners();
    void setupTempBuffers(int len);
    void readWriteLoops(Loop<float> loops[], CopyLoop<float>& tempLoop, size_t currentSample, const float* readBuffer, float* outBuffer);
    float calculateRMS(const Loop<float>& loop, size_t currentSample, int nSamples);
    float calculateRMS(const float* buffer, int nSamples) const;
    void setupLoops(size_t samplesPerBeat);
    void setRMS(int loopIndex, float value);
    
    bool loopDown[nLoops];
    float loopVolumes[nLoops];
    float loopRMSs[nLoops];
    float inputRMS = 0.f;
    bool muteInput = false;

    size_t samplesPerBeat = 0;
    Loop<float> loopsL[nLoops];
    Loop<float> loopsR[nLoops];
    size_t readIndex[nLoops];
    BufferStack<float> bufferStack;
    int nSamples;

    CopyLoop<float> nextLoopL;
    CopyLoop<float> nextLoopR;

    std::vector<std::unique_ptr<juce::AudioProcessorParameter::Listener>> listeners;
    LoopSyncer loopSyncer;

    struct ButtonListener : public juce::AudioProcessorParameter::Listener {

        ButtonListener(bool& pressed, int loopIndex, LooperAudioProcessor& looper) :
            pressed(pressed), loopIndex(loopIndex), looper(looper) {};
        ~ButtonListener() {};

        bool& pressed;
        const int loopIndex;
        LooperAudioProcessor& looper;

        void parameterValueChanged(int parameterIndex, float newValue) override {
            pressed = true;
            if (looper.recordingIndex == loopIndex) {
                looper.loopSyncer.broadcastStopRecord();
            } else {
                looper.loopSyncer.broadcastStartRecord(loopIndex);
            }
        }

        void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    };

    struct VolumeListener : juce::AudioProcessorParameter::Listener {

        VolumeListener(float& volume, int loopIndex, LooperAudioProcessor& looper) :
            volume(volume), loopIndex(loopIndex), looper(looper) {};
        ~VolumeListener() {};

        float& volume;
        const int loopIndex;
        LooperAudioProcessor& looper;

        void parameterValueChanged(int parameterIndex, float newValue) override {
            if (volume == newValue) return; // prevent message from looping when updated via LoopSyncer call

            volume = newValue;
            looper.loopSyncer.broadcastLoopVolume(loopIndex, volume);
        }

        void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    };

    struct ToggleButtonListener : juce::AudioProcessorParameter::Listener {

        ToggleButtonListener(int loopIndex, LooperAudioProcessor& looper) :
            loopIndex(loopIndex), looper(looper) {};
        ~ToggleButtonListener() {};

        const int loopIndex;
        LooperAudioProcessor& looper;

        void parameterValueChanged(int parameterIndex, float newValue) override {
            if (newValue > 0.5f) {
                looper.monitorIndex = loopIndex;
            } else if (looper.monitorIndex == loopIndex) {
                looper.monitorIndex = -1;
            }
        }

        void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    };

    struct MuteInputListener : juce::AudioProcessorParameter::Listener {

        MuteInputListener(LooperAudioProcessor& looper) : looper(looper) {};
        ~MuteInputListener() {};

        LooperAudioProcessor& looper;

        void parameterValueChanged(int parameterIndex, float newValue) override {
            looper.muteInput = newValue > 0.5f;
        }

        void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperAudioProcessor)
};
