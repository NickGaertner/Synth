#pragma once

#include <JuceHeader.h>
#include "DSP.h"

namespace Synth
{

	class Synth : public juce::Synthesiser {
		using Synthesiser::Synthesiser;
	public:

		void prepare(const juce::dsp::ProcessSpec& spec);

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Synth)
	};


	class SynthVoice : public juce::SynthesiserVoice {
		using SynthesiserVoice::SynthesiserVoice;
	public:

		void prepare(const juce::dsp::ProcessSpec& spec);

		virtual bool canPlaySound(juce::SynthesiserSound*) override;
		virtual void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
		virtual void stopNote(float velocity, bool allowTailOff) override;
		virtual void pitchWheelMoved(int newPitchWheelValue) override;
		virtual void controllerMoved(int controllerNumber, int newControllerValue) override;
		virtual void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

		void reset();

		std::vector<customDsp::ProcessorChain> oscChains{ configuration::OSC_NUMBER };
		customDsp::ProcessorChain monoChain;
		customDsp::ProcessorChain stereoChain;
		customDsp::SplitProcessor modulationProcessors;

	private:

		inline static float pitchWheelToFactor(int pitchWheelPos) {
			auto normalizedPos = (pitchWheelPos - 0x2000) / static_cast<float>(0x2000);
			return std::exp2f(2 * normalizedPos / 12.f);
		}

		juce::HeapBlock<char> heapBlock;
		juce::dsp::AudioBlock<float> tmpAudioBlock;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthVoice)
	};

	class SynthSound : public juce::SynthesiserSound {
	public:
		SynthSound() {}

		virtual bool appliesToNote(int midiNoteNumber) override;
		virtual bool appliesToChannel(int midiChannel) override;

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthSound)
	};

}