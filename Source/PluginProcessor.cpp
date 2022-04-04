/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Synth.h"
#include "Wavetable.h"
#include "Filter.h"
#include "FX.h"

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
	juce::ignoreUnused(index);
}

const juce::String SynthAudioProcessor::getProgramName(int index)
{
	juce::ignoreUnused(index);
	return {};
}

void SynthAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
	juce::ignoreUnused(index, newName);
}

//==============================================================================
void SynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)getTotalNumOutputChannels() };

	synth.prepare(spec);

	for (auto i = 0; i < configuration::OSC_NUMBER; i++)
	{
		auto prefix = configuration::OSC_PREFIX + std::to_string(i) + configuration::WT_SUFFIX;
		apvts.getParameter(prefix)->sendValueChangedMessageToListeners(0);
	}
	for (auto lfoPos = 0; lfoPos < configuration::LFO_NUMBER; lfoPos++)
	{
		auto prefix = configuration::LFO_PREFIX + std::to_string(lfoPos) + configuration::WT_SUFFIX;
		apvts.getParameter(prefix)->sendValueChangedMessageToListeners(0);
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

	performanceCounter.start();
	juce::ScopedNoDenormals noDenormals;

	buffer.clear();

	synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

	auto masterGain = apvts.getRawParameterValue(configuration::MASTER_PREFIX + configuration::GAIN_SUFFIX)->load();
	buffer.applyGain(masterGain);
	auto magnitude = buffer.getMagnitude(0, buffer.getNumSamples());
	if (magnitude >= 1.f) {
		DBG("Hit 0dB :(");
		buffer.applyGain(1.f / magnitude);
	}
	if (masterLevelCallback) {
		masterLevelCallback(buffer.getMagnitude(0,0,buffer.getNumSamples()), buffer.getMagnitude(1, 0, buffer.getNumSamples()));
	}
	if (observationCallback) {
		observationCallback(buffer);
	}
	performanceCounter.stop();

}

//==============================================================================
bool SynthAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SynthAudioProcessor::createEditor()
{
	return new SynthAudioProcessorEditor(*this);
	//return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.
	juce::ignoreUnused(destData);
}

void SynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
	juce::ignoreUnused(data, sizeInBytes);
}

void SynthAudioProcessor::initSynths()
{
	// Init each synth with 'POLYPHONY' voices and one sound
	for (auto i = 0; i < configuration::POLYPHONY; i++) {
		// add processors to each voice's processorChain and modulationProcessors
		auto voice = new Synth::SynthVoice{};

		// osc
		for (int j = 0; j < configuration::OSC_NUMBER; j++) {
			std::for_each(processorData[j].begin(), processorData[j].end(),
				[&](customDsp::Processor::SharedData* data) {
					voice->oscChains[j].addProcessor(data->createProcessor());
				});
		}

		// filter
		std::for_each(processorData[configuration::OSC_NUMBER].begin(), processorData[configuration::OSC_NUMBER].end(),
			[&](customDsp::Processor::SharedData* data) {
				voice->monoChain.addProcessor(data->createProcessor());
			});

		// fx & pan
		std::for_each(processorData[configuration::OSC_NUMBER + 1].begin(), processorData[configuration::OSC_NUMBER + 1].end(),
			[&](customDsp::Processor::SharedData* data) {
				voice->stereoChain.addProcessor(data->createProcessor());
			});

		// modulation
		std::for_each(processorData[configuration::OSC_NUMBER + 3].begin(), processorData[configuration::OSC_NUMBER + 3].end(),
			[&](customDsp::Processor::SharedData* data) {
				voice->modulationProcessors.addProcessor(data->createProcessor());
			});

		synth.addVoice(voice);
	}

	synth.addSound(new Synth::SynthSound{});
	for (auto& dataArray : processorData) {
		std::for_each(dataArray.begin(), dataArray.end(),
			[&](customDsp::Processor::SharedData* data) {
				data->registerAsListener(apvts);
			});
	}
}

juce::AudioProcessorValueTreeState::ParameterLayout SynthAudioProcessor::createParameterDataAndLayout()
{
	// DATA

	// Create Processor::SharedData for the osc specific processors in the right order
	for (auto i = 0; i < configuration::OSC_NUMBER; i++)
	{
		auto prefix = configuration::OSC_PREFIX + std::to_string(i);
		processorData[i].add(new customDsp::InterpolationOsc::SharedData{ prefix });
		processorData[i].add(new customDsp::Gain::SharedData{ prefix });
	}

	// Create Processor::SharedData for the osc independent processors in the right order
	// Mono
	for (auto i = 0; i < configuration::FILTER_NUMBER; i++) {
		auto prefix = configuration::FILTER_PREFIX + std::to_string(i);
		processorData[configuration::OSC_NUMBER].add(new customDsp::FilterChooser::SharedData{ prefix });
	}
	// stereo
	for (auto i = 0; i < configuration::FX_NUMBER; i++) {
		auto prefix = configuration::FX_PREFIX + std::to_string(i);
		processorData[configuration::OSC_NUMBER + 1].add(new customDsp::FXChooser::SharedData{ prefix });
	}

	processorData[configuration::OSC_NUMBER + 1].add(new customDsp::Pan::SharedData{ configuration::PAN_PREFIX });

	// Master
	//processorData[configuration::OSC_NUMBER + 2].add(new customDsp::Gain::SharedData{ configuration::MASTER_PREFIX });

	// Create Processor::SharedData for the modulation processors in the right order
	for (auto i = 0; i < configuration::ENV_NUMBER; i++) {
		auto prefix = configuration::ENV_PREFIX + std::to_string(i);
		processorData[configuration::OSC_NUMBER + 3].add(new customDsp::Envelope::SharedData{ prefix });
	}

	for (auto i = 0; i < configuration::LFO_NUMBER; i++) {
		auto prefix = configuration::LFO_PREFIX + std::to_string(i);
		processorData[configuration::OSC_NUMBER + 3].add(new customDsp::LFO::SharedData{ prefix });
	}

	// LAYOUT

	juce::AudioProcessorValueTreeState::ParameterLayout layout;

	for (auto& dataArray : processorData) {
		std::for_each(dataArray.begin(), dataArray.end(),
			[&](customDsp::Processor::SharedData* data) {
				data->addParams(layout);
			});
	}
	// Master
	layout.add(std::make_unique<juce::AudioParameterFloat>(
		configuration::MASTER_PREFIX + configuration::GAIN_SUFFIX,
		configuration::MASTER_PREFIX + configuration::GAIN_SUFFIX,
		juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.f),
		0.5f));

	return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new SynthAudioProcessor();
}


