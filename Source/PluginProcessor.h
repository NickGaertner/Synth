/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Synth.h"
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

private:

	void initSynths();

	static const int POLYPHONY = 16; // 16
	static const int OSC_NUMBER = 1; // 2
	static const int ENV_NUMBER = 2; // 2
	static const int LFO_NUMBER = 0; // 2
	Synth::Synth synths[OSC_NUMBER];

	// first OSC_NUMBER entries hold synth specific data (e.g. osc settings), 
	// the second to last entry holds synth independent data (e.g. fx)
	// and the last entry holds modulation parameters (envs and lfos)
	juce::OwnedArray<customDsp::Processor::SharedData> processorData[OSC_NUMBER + 2];

	juce::AudioProcessorValueTreeState::ParameterLayout createParameterDataAndLayout();
	void updateSettings();

	juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterDataAndLayout() };

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthAudioProcessor)
};
