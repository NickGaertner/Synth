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

private:

	void initSynths();

	std::vector<Synth::Synth> synths{ configuration::OSC_NUMBER };

	// first OSC_NUMBER entries hold synth specific data (e.g. osc settings), 
	// the second to last entry holds synth independent data (e.g. fx)
	// and the last entry holds modulation parameters (envs and lfos)
	std::vector<juce::OwnedArray<customDsp::Processor::SharedData>> processorData{ configuration::OSC_NUMBER + 2 };

	juce::AudioProcessorValueTreeState::ParameterLayout createParameterDataAndLayout();

	juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterDataAndLayout() };

	juce::PerformanceCounter performanceCounter{ "ProcessingBlogCounter",500 };
	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthAudioProcessor)
};
