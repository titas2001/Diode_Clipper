/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <fstream>


//==============================================================================
DiodeClipperNRAudioProcessor::DiodeClipperNRAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::mono(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::mono(), true)
#endif
    ),
    audioTree(*this, nullptr, juce::Identifier("PARAMETERS"),
        { std::make_unique<juce::AudioParameterFloat>("controlR_ID","ControlR",juce::NormalisableRange<float>(0.0, 130.0, 0.1),1.0)
        }),
    lowPassFilter(juce::dsp::IIR::Coefficients< float >::makeLowPass((48000.0 * 16.0), 20000.0))
#endif
{
    oversampling.reset(new juce::dsp::Oversampling<float>(2, 4, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));
    audioTree.addParameterListener("controlR_ID", this);

    controlledR = 1.0;
    
    myfile.open("example.csv");
}

DiodeClipperNRAudioProcessor::~DiodeClipperNRAudioProcessor()
{
    myfile.close();
    oversampling.reset();
}

//==============================================================================
const juce::String DiodeClipperNRAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DiodeClipperNRAudioProcessor::acceptsMidi() const
{
    return false;
}

bool DiodeClipperNRAudioProcessor::producesMidi() const
{
    return false;
}

bool DiodeClipperNRAudioProcessor::isMidiEffect() const
{
    return false;
}

double DiodeClipperNRAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DiodeClipperNRAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DiodeClipperNRAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DiodeClipperNRAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String DiodeClipperNRAudioProcessor::getProgramName(int index)
{
    return {};
}

void DiodeClipperNRAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void DiodeClipperNRAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{

    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    //oversampling->reset();
    //oversampling->initProcessing(static_cast<size_t> (samplesPerBlock));
    oversample = 16;
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
    beta = 0.125;
    betaM1 = 1 - beta;
}

void DiodeClipperNRAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DiodeClipperNRAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
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
void DiodeClipperNRAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    /***************************************************************************************/

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    /***************************************************************************************/
    // 1. Fill a new array here with upsampled input
    //      a. zeros array of size buffer*N
    //      b. assign input value to every N'th sample in the zeros array
    // 2. Apply low pass
    // 3. Run the VCF
    // 4. Apply low pass again 
    // 5. For loop to downsample


    blockInput.resize(buffer.getNumSamples() * oversample, 0);
    blockOutput.resize(buffer.getNumSamples() * oversample, 0);
    blockOutputDownsampled.resize(buffer.getNumSamples(), 0);


    // Upsample and filter upsampled input
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        
        //blockInput[sample * oversample] = buffer.getSample(0, sample);
        for (int i = 0; i < oversample; ++i)
        {
            blockOutput[sample*oversample + i] = ((buffer.getSample(0, sample) * (16.0 - i) + buffer.getSample(0, sample + 1) * (0.0 + i) ) / 16.0);
        }
    }
    
    // Process
    for (int sample = 0; sample < blockOutput.size(); ++sample)
    {
        voutTemp = 1;
        vout = 0;
        vin = controlledR * blockOutput[sample];
        //juce::Logger::getCurrentLogger()->outputDebugString("START");
        //myfile << "0 \n";
        int itter = 0;
        while (std::abs(voutTemp - vout) > err && itter < 20) {
            voutTemp = vout;
            vNom = T * voutTemp * R * gdExpDiff(-voutTemp) + T * R * gdExp(-voutTemp) + voutOld * R * C + T * (gdExpDiff(voutTemp) * R * voutTemp - R * gdExp(voutTemp) + vin);
            vDenom = T * R * gdExpDiff(voutTemp) + T * R * gdExpDiff(-voutTemp) + R * C + T;
            vout = vNom / vDenom;
            itter++;
        }
        //myfile << "1 \n";
        //juce::Logger::getCurrentLogger()->outputDebugString(std::to_string(itter));
        voutOld = vout;
        
        blockOutput[sample] = limiter(vout);

    }
    // Downsample and filter output from process
    for (int sample = 0; sample < blockOutputDownsampled.size(); ++sample)
    {
        blockOutputDownsampled[sample] = blockOutput[sample * oversample]; 
    }

    // Output to buffer
    for (int sample = 0; sample < blockOutputDownsampled.size(); ++sample)
    {
        buffer.setSample(0, sample, blockOutputDownsampled[sample]);
        //buffer.setSample(1, sample, blockOutputDownsampled[sample]);
    }

}
//==============================================================================
bool DiodeClipperNRAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}
void DiodeClipperNRAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    //Parameters update  when sliders moved
    if (parameterID == "controlR_ID") {
        controlledR = newValue;
    }
}

void DiodeClipperNRAudioProcessor::updateFilter()
{
    float frequency = 48e3 * 16.0;

    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(frequency, frequency / 16.0);
}
float DiodeClipperNRAudioProcessor::gdExp(float vc)
{
    return Id * (std::exp(vc / (2.0 * Ve)) - 1.0);
}
float DiodeClipperNRAudioProcessor::gdExpDiff(float vc)
{
    return (Id * std::exp(vc / (2.0 * Ve))) / (2.0 * Ve);
}
float DiodeClipperNRAudioProcessor::limiter(float val)
{
    if (val < -1.0)
    {
        val = -1.0;
        return val;
    }
    else if (val > 1.0)
    {
        val = 1.0;
        return val;
    }
    return val;
}
juce::AudioProcessorEditor* DiodeClipperNRAudioProcessor::createEditor()
{
    return new DiodeClipperNRAudioProcessorEditor(*this, audioTree);
}

//==============================================================================
void DiodeClipperNRAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DiodeClipperNRAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DiodeClipperNRAudioProcessor();
}
