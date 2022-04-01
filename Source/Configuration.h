
#pragma once
#include <JuceHeader.h>

namespace configuration {
	extern const int POLYPHONY;
	extern const int OSC_NUMBER;
	extern const int FILTER_NUMBER;
	extern const int FX_NUMBER;
	extern const int ENV_NUMBER;
	extern const int LFO_NUMBER;

	extern const int MOD_BLOCK_SIZE;

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
	extern const juce::String WT_SUFFIX;
	extern const juce::String PITCH_SUFFIX;
	extern const juce::String GAIN_SUFFIX;
	extern const juce::String ATTACK_SUFFIX;
	extern const juce::String DECAY_SUFFIX;
	extern const juce::String SUSTAIN_SUFFIX;
	extern const juce::String RELEASE_SUFFIX;
	extern const juce::String RATE_SUFFIX;
	extern const juce::String MODE_SUFFIX;
	extern const juce::String CUTOFF_SUFFIX;
	extern const juce::String RESONANCE_SUFFIX;
	extern const juce::String DRIVE_SUFFIX;
	extern const juce::String SPECIAL_SUFFIX;
	extern const juce::String FILTER_TYPE_SUFFIX;
	extern const juce::String FX_TYPE_SUFFIX;
	extern const juce::String DRY_WET_SUFFIX;
	extern const juce::String PARAMETER_0_SUFFIX;
	extern const juce::String PARAMETER_1_SUFFIX;
	extern const juce::String PARAMETER_2_SUFFIX;
	extern const juce::String PAN_SUFFIX;

	extern const juce::String WT_FOLDER_NAME;
	extern const juce::String WT_FILE_EXTENSION;
	extern const int MAX_TABLES_IN_MEM;

}