/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Synth.h"

//==============================================================================
SynthAudioProcessor::SynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
	)
#endif
{
	initSynths();

}

SynthAudioProcessor::~SynthAudioProcessor()
{
}

//==============================================================================
const juce::String SynthAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool SynthAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool SynthAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool SynthAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double SynthAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int SynthAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
				// so this should be at least 1, even if you're not really implementing programs.
}

int SynthAudioProcessor::getCurrentProgram()
{
	return 0;
}

void SynthAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String SynthAudioProcessor::getProgramName(int index)
{
	return {};
}

void SynthAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void SynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{

	juce::dsp::ProcessSpec spec{ sampleRate, samplesPerBlock, getTotalNumOutputChannels() };
	for (auto& synth : synths)
	{
		synth.prepare(spec);
	}
}

void SynthAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.

}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void SynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	//auto totalNumInputChannels = getTotalNumInputChannels();
	//auto totalNumOutputChannels = getTotalNumOutputChannels();

	updateSettings();
	buffer.clear();
	for (auto& synth : synths) {
		synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
	}
}

//==============================================================================
bool SynthAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SynthAudioProcessor::createEditor()
{
	//return new SynthAudioProcessorEditor (*this);
	return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.
}

void SynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
}

void SynthAudioProcessor::initSynths()
{
	// Init each synth with 'POLYPHONY' voices and one sound
	for (auto synthPos = 0; synthPos < std::size(synths); synthPos++)
	{
		auto& synth = synths[synthPos];

		for (auto i = 0; i < POLYPHONY; i++) {
			// add processors to each voice's processorChain and modulationProcessors
			auto voice = new Synth::SynthVoice{};

			// synth specific
			std::for_each(processorData[synthPos].begin(), processorData[synthPos].end(),
				[&](customDsp::Processor::SharedData* data) {
					voice->processorChain.addProcessor(data->createProcessor());
				});

			// synth independent
			std::for_each(processorData[OSC_NUMBER].begin(), processorData[OSC_NUMBER].end(),
				[&](customDsp::Processor::SharedData* data) {
					voice->processorChain.addProcessor(data->createProcessor());
				});

			// modulation
			std::for_each(processorData[OSC_NUMBER + 1].begin(), processorData[OSC_NUMBER + 1].end(),
				[&](customDsp::Processor::SharedData* data) {
					voice->modulationProcessors.addProcessor(data->createProcessor());
				});
			synth.addVoice(voice);
		}

		synth.addSound(new Synth::SynthSound());
	}
}

juce::AudioProcessorValueTreeState::ParameterLayout SynthAudioProcessor::createParameterDataAndLayout()
{
	// DATA

	// Create Processor::SharedData for the synth specific processors in the right order
	for (auto synthPos = 0; synthPos < OSC_NUMBER; synthPos++)
	{
		auto prefix = "OSC_" + std::to_string(synthPos) + "::";
		processorData[synthPos].add(new customDsp::InterpolationOsc::SharedData{ prefix });
		processorData[synthPos].add(new customDsp::Gain::SharedData{ prefix });

	}

	// Create Processor::SharedData for the synth independent processors in the right order


	// Create Processor::SharedData for the modulation processors in the right order
	for (auto i = 0; i < ENV_NUMBER; i++) {
		auto prefix = "ENV_" + std::to_string(i) + "::";
		processorData[OSC_NUMBER + 1].add(new customDsp::Envelope::SharedData{ prefix });
	}

	for (auto i = 0; i < LFO_NUMBER; i++) {
		auto prefix = "LFO_" + std::to_string(i) + "::";
		processorData[OSC_NUMBER + 1].add(new customDsp::InterpolationOsc::SharedData{ prefix });
	}

	// LAYOUT

	juce::AudioProcessorValueTreeState::ParameterLayout layout;

	for (auto& dataArray : processorData) {
		std::for_each(dataArray.begin(), dataArray.end(),
			[&](customDsp::Processor::SharedData* data) {
				data->addParams(layout);
			});
	}

	return layout;
}

void SynthAudioProcessor::updateSettings()
{
	for (auto& dataArray : processorData) {
		std::for_each(dataArray.begin(), dataArray.end(),
			[&](customDsp::Processor::SharedData* data) {
				data->updateParams(apvts);
			});
	}
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new SynthAudioProcessor();
}
