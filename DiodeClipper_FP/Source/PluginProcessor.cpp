/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"



//==============================================================================
DiodeClipperFPAudioProcessor::DiodeClipperFPAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    audioTree(*this, nullptr, juce::Identifier("PARAMETERS"),
        { std::make_unique<juce::AudioParameterFloat>("controlR_ID","ControlR",juce::NormalisableRange<float>(0.0, 10.0, 0.01),1.0)
        }),
    lowPassFilter(juce::dsp::IIR::Coefficients< float >::makeLowPass((48000.0 * 16.0), 20000.0))
#endif
{
    oversampling.reset(new juce::dsp::Oversampling<float>(2, 4, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));
    audioTree.addParameterListener("controlR_ID", this);

    controlledR = 1.0;

}

DiodeClipperFPAudioProcessor::~DiodeClipperFPAudioProcessor()
{
    oversampling.reset();
}

//==============================================================================
const juce::String DiodeClipperFPAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DiodeClipperFPAudioProcessor::acceptsMidi() const
{
    return false;
}

bool DiodeClipperFPAudioProcessor::producesMidi() const
{
    return false;
}

bool DiodeClipperFPAudioProcessor::isMidiEffect() const
{
    return false;
}

double DiodeClipperFPAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DiodeClipperFPAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DiodeClipperFPAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DiodeClipperFPAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DiodeClipperFPAudioProcessor::getProgramName (int index)
{
    return {};
}

void DiodeClipperFPAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DiodeClipperFPAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    oversampling->reset();
    oversampling->initProcessing(static_cast<size_t> (samplesPerBlock));

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate * 16;
    spec.maximumBlockSize = samplesPerBlock * 15;
    spec.numChannels = getTotalNumOutputChannels();

    lowPassFilter.prepare(spec);
    lowPassFilter.reset();
    // Set the constants
    Fs = sampleRate;
    T = 1 / Fs;
    C = 100e-9;
    R = 1e3;
    Vp = 0.17;
    Ve = 0.023;
    Id = 2.52e-9;
    err = 0.1e-3; // err for stopping iterations
    // set controlled values to starting values (redundant maybe delit later)
    controlledR = 1.0;
    voutOld = 0;
}

void DiodeClipperFPAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool DiodeClipperFPAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet()  == juce::AudioChannelSet::disabled()
     || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;
        
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void DiodeClipperFPAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto mainInputOutput = getBusBuffer(buffer, true, 0);
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    /***************************************************************************************/
    // 1. Fill a new array here with upsampled input
    //      a. zeros array of size buffer*N
    //      b. assign input value to every N'th sample in the zeros array
    // 2. Apply low pass
    // 3. Run the VCF
    // 4. Apply low pass again 
    // 5. For loop to downsample

    //R = controlledR;
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());


    juce::dsp::AudioBlock<float> blockInput(buffer);
    juce::dsp::AudioBlock<float> blockOutput = oversampling->processSamplesUp(blockInput);

    updateFilter();
    lowPassFilter.process(juce::dsp::ProcessContextReplacing<float>(blockOutput));
    for (int channel = 0; channel < blockOutput.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < blockOutput.getNumSamples(); ++sample)
        {
            voutTemp = 1;
            vout = 0;
            vin = controlledR *blockOutput.getSample(channel, sample);
            while (std::abs(voutTemp - vout) > err) {
                voutTemp = vout;
                vout = R * T / (R * C + T) * (gdExp(-voutTemp) - gdExp(voutTemp)) + T / (R * C + T) * vin + R * C / (R * C + T) * voutOld;
            }
            
            voutOld = vout;
            //vout = vin;
            vout = limiter(vout);

            blockOutput.setSample(channel, sample, vout);
        }
    }
    updateFilter();
    lowPassFilter.process(juce::dsp::ProcessContextReplacing<float>(blockOutput));

    oversampling->processSamplesDown(blockInput);

    //updateFilter(1);
    //lowPassFilter.process(juce::dsp::ProcessContextReplacing<float>(blockOutput));
}
//==============================================================================
bool DiodeClipperFPAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}
void DiodeClipperFPAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    //Parameters update  when sliders moved
    if (parameterID == "controlR_ID") {
        controlledR = newValue;
    }
}
void DiodeClipperFPAudioProcessor::updateFilter()
{
    float frequency = 48e3 * 16;

    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(frequency, frequency / 16);
}
double DiodeClipperFPAudioProcessor::gdExp(double vc) 
{
    return Id * (std::exp(vc / (2 * Ve)) - 1);
}
double DiodeClipperFPAudioProcessor::gdPoly(double vc)
{
    return Vp * vc*vc*vc*vc * Heaviside(vc);
}
double DiodeClipperFPAudioProcessor::Heaviside(double vc)
{
    if (vc >= 0)
        return 1;
    else return 0;
}
double DiodeClipperFPAudioProcessor::limiter(double val)
{
    if (val < -1)
    {
        val = -1;
        return val;
    }
    else if (val > 1)
    {
        val = 1;
        return val;
    }
    return val;
}
juce::AudioProcessorEditor* DiodeClipperFPAudioProcessor::createEditor()
{
    return new DiodeClipperFPAudioProcessorEditor(*this, audioTree);
}

//==============================================================================
void DiodeClipperFPAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DiodeClipperFPAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DiodeClipperFPAudioProcessor();
}
