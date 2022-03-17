
#include "Configuration.h"

namespace configuration {
	const int POLYPHONY = 16; // 16
	const int OSC_NUMBER = 2; // 2
	const int FILTER_NUMBER = 2; // 2
	const int FX_NUMBER = 2; // 2
	const int ENV_NUMBER = 2; // 2
	const int LFO_NUMBER = 2; // 2

	const int EMPTY_MOD_CHANNEL = ENV_NUMBER + LFO_NUMBER;
	const juce::StringArray& getModChannelNames() {
		static juce::StringArray array;
		if (!array.isEmpty()) {
			return array;
		}
		for (auto i = 0; i < ENV_NUMBER; i++) {
			array.add(ENV_PREFIX + juce::String(i));
		}
		for (auto i = 0; i < LFO_NUMBER; i++) {
			array.add(LFO_PREFIX + juce::String(i));
		}
		array.add(juce::String("NONE"));
		return array;
	}

	const juce::String OSC_PREFIX{ "OSC_" };
	const juce::String FILTER_PREFIX{ "FILTER_" };
	const juce::String FX_PREFIX{ "FX_" };
	const juce::String ENV_PREFIX{ "ENV_" };
	const juce::String LFO_PREFIX{ "LFO_" };
	const juce::String PAN_PREFIX{"PAN"};
	const juce::String MASTER_PREFIX{"MASTER"};

	const juce::String BYPASSED_SUFFIX{"::BYPASSED"};
	const juce::String MOD_CHANNEL_SUFFIX{ "::MOD_CHANNEL" };
	const juce::String MOD_FACTOR_SUFFIX{ "::MOD_FACTOR" };
	const juce::String ENV_SUFFIX{"::ENV"};
	const juce::String WT_POS_SUFFIX{"::WT_POS"};
	const juce::String PITCH_SUFFIX{"::PITCH"};
	const juce::String GAIN_SUFFIX{"::GAIN"};
	const juce::String ATTACK_SUFFIX{"::ATTACK"};
	const juce::String DECAY_SUFFIX{"::DECAY"};
	const juce::String SUSTAIN_SUFFIX{"::SUSTAIN"};
	const juce::String RELEASE_SUFFIX{"::RELEASE"};
}