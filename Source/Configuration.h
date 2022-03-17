
#pragma once
#include <JuceHeader.h>

namespace configuration {
	extern const int POLYPHONY;
	extern const int OSC_NUMBER;
	extern const int FILTER_NUMBER;
	extern const int FX_NUMBER;
	extern const int ENV_NUMBER;
	extern const int LFO_NUMBER;

	extern const int EMPTY_MOD_CHANNEL;

	const juce::StringArray& getModChannelNames();

	extern const juce::String OSC_PREFIX;
	extern const juce::String FILTER_PREFIX;
	extern const juce::String FX_PREFIX;
	extern const juce::String ENV_PREFIX;
	extern const juce::String LFO_PREFIX;
	extern const juce::String PAN_PREFIX;
	extern const juce::String MASTER_PREFIX;

	extern const juce::String BYPASSED_SUFFIX;
	extern const juce::String MOD_CHANNEL_SUFFIX;
	extern const juce::String MOD_FACTOR_SUFFIX;
	extern const juce::String ENV_SUFFIX;
	extern const juce::String WT_POS_SUFFIX;
	extern const juce::String WF0_SUFFIX;
	extern const juce::String WF1_SUFFIX;
	extern const juce::String PITCH_SUFFIX;
	extern const juce::String GAIN_SUFFIX;
	extern const juce::String ATTACK_SUFFIX;
	extern const juce::String DECAY_SUFFIX;
	extern const juce::String SUSTAIN_SUFFIX;
	extern const juce::String RELEASE_SUFFIX;
	extern const juce::String RATE_SUFFIX;

}