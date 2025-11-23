/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include "dsp/Voice.h"
#include "dsp/Limiter.h"
#include "dsp/Comb.h"
#include "dsp/Resonator.h"
#include "libMTSClient.h"

struct MIDIMsg {
    int offset;
    int isNoteon;
    int note;
    int vel;
};

//==============================================================================
/**
*/
class RipplerXAudioProcessor  : public juce::AudioProcessor, public juce::AudioProcessorParameter::Listener, public juce::VST3ClientExtensions
{
public:
    float scale = 1.0f; // UI scale
    int polyphony = 8;
    bool velMap = false; // config used by UI to set velocity edit mode
    bool darkTheme = false;
    int last_a_model = -1;
    int last_b_model = -1;
    int last_a_partials = -1;
    int last_b_partials = -1;
    int currentProgram = -1;
    std::atomic<float> rmsValue { 0.0f };
    MTSClient *mtsClientPtr;

    //==============================================================================
    RipplerXAudioProcessor();
    ~RipplerXAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;
    bool supportsDoublePrecisionProcessing() const override;
    bool getPluginHasMainInput() const override { return false; }

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    float normalizeVolSlider(float val);
    void onNote (MIDIMsg msg);
    void offNote (MIDIMsg msg);
    void onSlider ();
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    template <typename FloatType>
    void processBlockByType(AudioBuffer<FloatType>& buffer, MidiBuffer& midiMessages);
    void clearVoices();
    void toggleTheme();
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
    void resetLastModels();

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void loadSettings();
    void saveSettings();
    void setPolyphony(int value);
    void setScale(float value);

    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState params;
    juce::UndoManager undoManager;

private:
    bool paramChanged = false; // flag that triggers on any param change
    juce::ApplicationProperties settings;
    std::vector<MIDIMsg> midi;
    std::vector<std::unique_ptr<Voice>> voices;
    std::unique_ptr<Models> models;
    int nvoice = 0; // next voice to use
    Comb comb{};
    Limiter limiter{};



    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RipplerXAudioProcessor)
};