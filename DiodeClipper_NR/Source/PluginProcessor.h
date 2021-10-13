/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <fstream>
//==============================================================================
/**
*/

class DiodeClipperNRAudioProcessor : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    DiodeClipperNRAudioProcessor();
    ~DiodeClipperNRAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    //==============================================================================
    //void setK(double val) { controlledK = val; };
    //void setVt(double val) { controlledR = val; };
    void parameterChanged(const juce::String& parameterID, float newValue);
    void updateFilter();
    float gdExp(float vc);
    float gdExpDiff(float vc);
    float limiter(float val);

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioProcessorValueTreeState audioTree;

private:
    float controlledR;
    float Id, C, Ve, Vp, R, err;
    float Fs, T;
    float vNom, vDenom;
    float vin;
    float vout, voutTemp, voutOld;
    float beta, betaM1;
    std::ofstream myfile;
    int oversample;
    std::vector<float> blockInput, blockOutput, blockOutputDownsampled;
    float oldBlockOutput;
    juce::dsp::ProcessorDuplicator< juce::dsp::IIR::Filter <float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiodeClipperNRAudioProcessor)
};
