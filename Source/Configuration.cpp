
#include "Configuration.h"

namespace configuration {
	const int POLYPHONY = 16; // 16
	const int OSC_NUMBER = 3; // 3
	const int FILTER_NUMBER = 2; // 2
	const int FX_NUMBER = 2; // 2
	const int ENV_NUMBER = 3; // 3
	const int LFO_NUMBER = 3; // 3

	const int MOD_BLOCK_SIZE = 128;

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
		array.add(juce::String("None"));
		return array;
	}

	const juce::String OSC_PREFIX{ "Osc_" };
	const juce::String FILTER_PREFIX{ "Filter_" };
	const juce::String FX_PREFIX{ "Fx_" };
	const juce::String ENV_PREFIX{ "Env_" };
	const juce::String LFO_PREFIX{ "Lfo_" };
	const juce::String PAN_PREFIX{"Pan"};
	const juce::String MASTER_PREFIX{"Master"};

	const juce::String BYPASSED_SUFFIX{"::BYPASSED"};
	const juce::String MOD_CHANNEL_SUFFIX{ "::MOD_CHANNEL" };
	const juce::String MOD_FACTOR_SUFFIX{ "::MOD_FACTOR" };
	const juce::String ENV_SUFFIX{"::ENV"};
	const juce::String WT_POS_SUFFIX{"::WT_POS"};
	const juce::String WF0_SUFFIX{"::WF0"};
	const juce::String WF1_SUFFIX{"::WF1"};
	const juce::String WT_SUFFIX{"::WT"};
	const juce::String PITCH_SUFFIX{"::PITCH"};
	const juce::String GAIN_SUFFIX{"::GAIN"};
	const juce::String ATTACK_SUFFIX{"::ATTACK"};
	const juce::String DECAY_SUFFIX{"::DECAY"};
	const juce::String SUSTAIN_SUFFIX{"::SUSTAIN"};
	const juce::String RELEASE_SUFFIX{"::RELEASE"};
	const juce::String RATE_SUFFIX{"::RATE"};
	const juce::String MODE_SUFFIX{"::MODE"};
	const juce::String CUTOFF_SUFFIX{"::CUTOFF"};
	const juce::String RESONANCE_SUFFIX{"::RES"};
	const juce::String DRIVE_SUFFIX{"::DRIVE"};
	const juce::String SPECIAL_SUFFIX{"::SPECIAL"};
	const juce::String FILTER_TYPE_SUFFIX{"::FILTER_TYPE"};
	const juce::String FX_TYPE_SUFFIX{"::FX_TYPE"};
	const juce::String DRY_WET_SUFFIX{"::DRY_WET"};
	const juce::String PARAMETER_0_SUFFIX{"::PARAMETER_0"};
	const juce::String PARAMETER_1_SUFFIX{ "::PARAMETER_1" };
	const juce::String PARAMETER_2_SUFFIX{ "::PARAMETER_2" };
	const juce::String PAN_SUFFIX{"::PAN"};

	const juce::String VALUE_TREE_IDENTIFIER{"Parameters"};

	const juce::String MAIN_FOLDER_NAME{"synth"};

	const juce::String WT_FOLDER_NAME{"wavetables"};
	const juce::String WT_FILE_EXTENSION{".wt"};
	extern const int MAX_TABLES_IN_MEM{2};

	const juce::String PRESET_FOLDER_NAME{"presets"};

}