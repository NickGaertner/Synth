/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Synth.h"
#include "Configuration.h"
#include "DSP.h"

//==============================================================================
/**
*/
class SynthAudioProcessor : public juce::AudioProcessor
{
public:
	//==============================================================================
	SynthAudioProcessor();
	~SynthAudioProcessor() override;

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

	juce::AudioProcessorValueTreeState& getApvts() {
		return apvts;
	}

	std::function<void(const juce::dsp::AudioBlock<float>& audioBlock)> observationCallback{};

private:

	void initSynths();

	// TODO refactor this whole thing
	Synth::Synth synth;
	// first OSC_NUMBER entries hold osc specific data, 
	// the next entry holds mono processor/filter data,
	// the next entry holds stereo processor/fx data
	// the second to last entry holds master data (pan, master gain)
	// and the last entry holds modulation parameters (envs and lfos)
	std::vector<juce::OwnedArray<customDsp::Processor::SharedData>> processorData{configuration::OSC_NUMBER+4};

	juce::AudioProcessorValueTreeState::ParameterLayout createParameterDataAndLayout();

	juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterDataAndLayout() };

	juce::PerformanceCounter performanceCounter{ "ProcessingBlockCounter",500 };
	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthAudioProcessor)
};
