/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
DiodeClipperNRAudioProcessorEditor::DiodeClipperNRAudioProcessorEditor(DiodeClipperNRAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p), audioTree(vts)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(450, 250);

    /*
 ColourIds{
  backgroundColourId = 0x1001200, thumbColourId = 0x1001300, trackColourId = 0x1001310, rotarySliderFillColourId = 0x1001311,
  rotarySliderOutlineColourId = 0x1001312, textBoxTextColourId = 0x1001400, textBoxBackgroundColourId = 0x1001500, textBoxHighlightColourId = 0x1001600,
  textBoxOutlineColourId = 0x1001700 }
*/

    controlR.setColour(0x1001400, juce::Colour::fromRGBA(0x00, 0x40, 0x00, 0x80));
    controlR.setColour(0x1001700, juce::Colour::fromRGBA(0x00, 0x00, 0x00, 0x00));

    // these define the parameters of our slider object

    controlR.setSliderStyle(juce::Slider::LinearHorizontal);
    controlR.addListener(this);
    controlR.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    addAndMakeVisible(controlR);
    sliderAttachR.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioTree, "controlR_ID", controlR));
    //controlR.setPopupDisplayEnabled(true, false, this);
    labelR.setText(("Input Gain"), juce::dontSendNotification);
    labelR.setFont(juce::Font("Slope Opera", 16, 0));
    labelR.setColour(juce::Label::textColourId, juce::Colour::fromRGBA(0x40, 0x40, 0x80, 0xff));
    addAndMakeVisible(labelR);

}

DiodeClipperNRAudioProcessorEditor::~DiodeClipperNRAudioProcessorEditor()
{
    sliderAttachR.reset();
}

//==============================================================================
void DiodeClipperNRAudioProcessorEditor::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::white);

    // set the current drawing colour to black
    g.setColour(juce::Colours::black);

    // set the font size and draw text to the screen
    g.setFont(15.0f);
    g.setFont(juce::Font("Slope Opera", 35.0f, 1));
    g.drawFittedText("NR Diode Clipper", getLocalBounds(), juce::Justification::centred, 1);
}

void DiodeClipperNRAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    controlR.setBounds(140, getHeight() - 20, getWidth() - 140, 20);
    labelR.setBounds(0, getHeight() - 20, 140, 20);

}
void DiodeClipperNRAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
}