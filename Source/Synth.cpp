#include "Synth.h"

namespace Synth {

	void Synth::prepare(const juce::dsp::ProcessSpec& spec)
	{
		setCurrentPlaybackSampleRate(spec.sampleRate);
		for (auto voice : voices) {
			dynamic_cast<SynthVoice*>(voice)->prepare(spec);
		}
	}

	//

	void SynthVoice::prepare(const juce::dsp::ProcessSpec& spec)
	{
		tmpAudioBlock = juce::dsp::AudioBlock<float>(heapBlock, 
			spec.numChannels + modulationProcessors.size() + customDsp::WORK_BUFFERS + 1, // + 1 for empty channel
			spec.maximumBlockSize);
		processorChain.prepare(spec);
		modulationProcessors.prepare(spec);
	}

	bool SynthVoice::canPlaySound(juce::SynthesiserSound*)
	{
		return true;
	}

	void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* t_sound, int currentPitchWheelPosition)
	{
		juce::ignoreUnused(t_sound, currentPitchWheelPosition);

		// OSC
		auto osc = dynamic_cast<customDsp::InterpolationOsc*>(processorChain.getProcessor(0));
		osc->setFrequency((float)juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber), true);
		osc->setVelocity(velocity);

		// ENV & LFO
		modulationProcessors.noteOn();
	}

	void SynthVoice::stopNote(float velocity, bool allowTailOff)
	{
		juce::ignoreUnused(velocity);
		auto* osc = dynamic_cast<customDsp::InterpolationOsc*>(processorChain.getProcessor(0));
		if (!allowTailOff || osc->getEnvChannel() == -1) {
			clearCurrentNote();
			processorChain.reset();
			modulationProcessors.reset();
		}
		else {
			modulationProcessors.noteOff();
		}
	}

	void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
	{
		juce::ignoreUnused(newPitchWheelValue);
	}

	void SynthVoice::controllerMoved(int controllerNumber, int newControllerValue)
	{
		juce::ignoreUnused(controllerNumber, newControllerValue);
	}

	void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
	{	
		if (!isVoiceActive()) {
			return;
		}
		// check if we might exited the release phase of our possible adsr and stop the note if necessary
		auto osc = dynamic_cast<customDsp::InterpolationOsc*>(processorChain.getProcessor(0));
		if (osc->getEnvChannel() != -1) {
			auto env = dynamic_cast<customDsp::Envelope*>(modulationProcessors.getProcessor(osc->getEnvChannel()));
			if (env->isIdle()) {
				stopNote(0.f, false);
			}
		}

		// prepare temporary AudioBlocks to be used as buffers
		auto outputBlock = tmpAudioBlock.getSubsetChannelBlock(0, outputBuffer.getNumChannels())
			.getSubBlock(0, numSamples);
		outputBlock.clear();

		auto workBlock = tmpAudioBlock.getSubsetChannelBlock(outputBuffer.getNumChannels(), customDsp::WORK_BUFFERS)
			.getSubBlock(0, numSamples);
		workBlock.clear();

		auto inputBlock = tmpAudioBlock.getSubsetChannelBlock(outputBuffer.getNumChannels() + customDsp::WORK_BUFFERS,
			modulationProcessors.size() + 1)
			.getSubBlock(0, numSamples);;
		inputBlock.clear();

		// fill inputBlock with modulation signals
		juce::dsp::ProcessContextNonReplacing<float> modulationContext{ juce::dsp::AudioBlock<float>{},inputBlock };
		modulationProcessors.process(modulationContext, workBlock);

		// process with processChain on temporary blocks
		juce::dsp::ProcessContextNonReplacing<float> processContext{ inputBlock, outputBlock };
		processorChain.process(processContext, workBlock);

		// add result from temporary block to the output buffer
		juce::dsp::AudioBlock<float>{outputBuffer}.getSubBlock((size_t)startSample, (size_t)numSamples)
			.add(outputBlock.getSubsetChannelBlock(0, outputBuffer.getNumChannels()));
	}

	//

	bool SynthSound::appliesToNote(int midiNoteNumber)
	{
		juce::ignoreUnused(midiNoteNumber);
		return true;
	}

	bool SynthSound::appliesToChannel(int midiChannel)
	{
		juce::ignoreUnused(midiChannel);
		return true;
	}

}
