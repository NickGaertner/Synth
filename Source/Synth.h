#pragma once

#include <JuceHeader.h>
#include "CustomDSP.h"

namespace Synth
{

	class Synth : public juce::Synthesiser {
	public:
		
		void prepare(const juce::dsp::ProcessSpec& spec);
	
	private:
		JUCE_LEAK_DETECTOR(Synth)
	};


	class SynthVoice : public juce::SynthesiserVoice {
	
	public:

		void prepare(const juce::dsp::ProcessSpec& spec);

		virtual bool canPlaySound(juce::SynthesiserSound*) override;
		virtual void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
		virtual void stopNote(float velocity, bool allowTailOff) override;
		virtual void pitchWheelMoved(int newPitchWheelValue) override;
		virtual void controllerMoved(int controllerNumber, int newControllerValue) override;
		virtual void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

		customDsp::ProcessorChain processorChain;
		customDsp::SplitProcessor modulationProcessors;

	private:

		juce::HeapBlock<char> heapBlock;
		juce::dsp::AudioBlock<float> tmpAudioBlock;

		JUCE_LEAK_DETECTOR(SynthVoice)
	};

	class SynthSound : public juce::SynthesiserSound {
	
	public:

		virtual bool appliesToNote(int midiNoteNumber) override;
		virtual bool appliesToChannel(int midiChannel) override;

	private:
		JUCE_LEAK_DETECTOR(SynthSound)
	};

}