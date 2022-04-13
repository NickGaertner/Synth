#include "Synth.h"

namespace Synth {

	void Synth::prepare(const juce::dsp::ProcessSpec& spec)
	{
		setCurrentPlaybackSampleRate(spec.sampleRate);
		for (auto voice : voices) {
			dynamic_cast<SynthVoice*>(voice)->prepare(spec);
		}
	}

	void Synth::reset()
	{
		for (auto* voice : voices) {
			dynamic_cast<SynthVoice*>(voice)->reset();
		}
	}

	void SynthVoice::prepare(const juce::dsp::ProcessSpec& spec)
	{
		tmpAudioBlock = juce::dsp::AudioBlock<float>(heapBlock,
			spec.numChannels
			+ modulationProcessors.size()
			+ customDsp::WORK_BUFFERS + 1, // + 1 for empty channel
			spec.maximumBlockSize);
		for (auto& p : oscChains) {
			p.prepare(spec);
		}
		monoChain.prepare(spec);
		stereoChain.prepare(spec);
		modulationProcessors.prepare(spec);
	}

	bool SynthVoice::canPlaySound(juce::SynthesiserSound*)
	{
		return true;
	}

	void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* t_sound, int currentPitchWheelPosition)
	{
		juce::ignoreUnused(t_sound);
		auto freq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber))
			* pitchWheelToFactor(currentPitchWheelPosition);
		for (auto& p : oscChains) {
			auto osc = dynamic_cast<customDsp::InterpolationOsc*>(p.getProcessor(0));
			osc->setFrequency(freq, true);
			osc->setVelocity(velocity);
			p.noteOn();
		}
		monoChain.noteOn();
		stereoChain.noteOn();
		modulationProcessors.noteOn();
	}

	void SynthVoice::stopNote(float velocity, bool allowTailOff)
	{
		juce::ignoreUnused(velocity);
		for (auto& p : oscChains) {
			p.noteOff();
		}
		monoChain.noteOff();
		stereoChain.noteOff();
		modulationProcessors.noteOff();
		if (!allowTailOff) {
			reset();
		}
	}

	void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
	{
		auto midiNoteNumber = getCurrentlyPlayingNote();
		auto freq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber)) 
					* pitchWheelToFactor(newPitchWheelValue);
		for (auto& p : oscChains) {
			auto osc = dynamic_cast<customDsp::InterpolationOsc*>(p.getProcessor(0));
			osc->setFrequency(freq, false);
		}
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

		auto numChannels = outputBuffer.getNumChannels();

		// prepare temporary AudioBlocks to be used as buffers
		auto outputBlock = tmpAudioBlock.getSubsetChannelBlock(0, numChannels)
			.getSubBlock(0, numSamples);
		outputBlock.clear();
		auto monoBlock = outputBlock.getSingleChannelBlock(0);
		auto tmpMonoBlock = outputBlock.getSingleChannelBlock(1);

		auto workBlock = tmpAudioBlock.getSubsetChannelBlock(numChannels, customDsp::WORK_BUFFERS)
			.getSubBlock(0, numSamples);
		workBlock.clear();

		auto inputBlock = tmpAudioBlock.getSubsetChannelBlock(numChannels + customDsp::WORK_BUFFERS,
			modulationProcessors.size() + 1)
			.getSubBlock(0, numSamples);;
		inputBlock.clear();

		juce::dsp::ProcessContextNonReplacing<float> modulationContext{ juce::dsp::AudioBlock<float>{},inputBlock };
		juce::dsp::ProcessContextNonReplacing<float> oscContext{ inputBlock, tmpMonoBlock };
		juce::dsp::ProcessContextNonReplacing<float> monoContext{ inputBlock, monoBlock };
		juce::dsp::ProcessContextNonReplacing<float> stereoContext{ inputBlock, outputBlock };

		// fill inputBlock with modulation signals
		bool needMoreTime = modulationProcessors.process(modulationContext, workBlock);

		// process oscillators and add results to next context
		for (auto& p : oscChains) {
			needMoreTime |= p.process(oscContext, workBlock);
			monoBlock.add(tmpMonoBlock);
			tmpMonoBlock.clear();
		}

		//// process mono stuff (filters)
		needMoreTime |= monoChain.process(monoContext, workBlock);

		// copy the mono signal to all other channels (which should only be one)
		for (int channel = 1; channel < numChannels; channel++) {
			outputBlock.getSingleChannelBlock(channel).copyFrom(monoBlock);
		}
		//// process stereo stuff (fx)
		needMoreTime |= stereoChain.process(stereoContext, workBlock);

		// add result from temporary block to the output buffer
		juce::dsp::AudioBlock<float>{outputBuffer}.getSubBlock((size_t)startSample, (size_t)numSamples)
			.add(outputBlock.getSubsetChannelBlock(0, numChannels));

		if (!needMoreTime) {
			reset();
		}
	}

	void SynthVoice::reset()
	{
		for (auto& p : oscChains) {
			p.reset();
		}
		monoChain.reset();
		stereoChain.reset();
		modulationProcessors.reset();
		clearCurrentNote();
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
