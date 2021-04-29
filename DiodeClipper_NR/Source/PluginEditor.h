/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class DiodeClipperNRAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Slider::Listener
{
public:
    DiodeClipperNRAudioProcessorEditor(DiodeClipperNRAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~DiodeClipperNRAudioProcessorEditor() override;

    //==============================================================================
    void sliderValueChanged(juce::Slider* slider) override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DiodeClipperNRAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& audioTree;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    juce::Slider controlR;
    juce::Label labelR;

    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiodeClipperNRAudioProcessorEditor)
};
