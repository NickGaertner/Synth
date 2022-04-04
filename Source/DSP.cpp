
#include "DSP.h"

namespace customDsp {

	void Envelope::reset() {
		stage = Stage::IDLE;
		level = 0.f;
		summand = 0.f;
		samplesUntilTransition = (size_t)-1;
	}

	bool Envelope::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) {
		juce::ignoreUnused(workBuffers);
		if (stage == Stage::IDLE || context.isBypassed) {
			return false;
		}
		auto& outputBlock = context.getOutputBlock();
		auto currentPos = 0;
		auto samplesRemaining = outputBlock.getNumSamples();

		while (samplesRemaining) {
			if (samplesUntilTransition == 0) {
				transition();
			}
			auto samplesThisStep = samplesUntilTransition > 0 ? // "Do I need to transition at some point (>0)?"
				juce::jmin((int)juce::jmin(samplesUntilTransition, samplesRemaining), configuration::MOD_BLOCK_SIZE)
				: juce::jmin((int)samplesRemaining, configuration::MOD_BLOCK_SIZE);

			outputBlock.getSubBlock(currentPos, samplesThisStep).add(level);
			level += summand * samplesThisStep;
			currentPos += samplesThisStep;
			samplesUntilTransition -= samplesThisStep;
			samplesRemaining -= samplesThisStep;
		}

		return true;
	};

	void Envelope::noteOn() {
		Processor::noteOn();
		reset();
		transition();
	}
	void Envelope::noteOff() {
		Processor::noteOff();
		if (stage != Stage::IDLE) {
			stage = Stage::SUSTAIN;
			transition();
		}
	}
	void Envelope::transition() {
		do {
			advanceStage();
		} while (stage != Stage::IDLE && data->stageValues[(int)stage] == 0.f);

		if (stage == Stage::ATTACK) {
			samplesUntilTransition = (size_t)(data->sampleRate * data->stageValues[(int)Stage::ATTACK]);
			level = data->minLevel;
			summand = (1.f - level) / samplesUntilTransition;
		}
		else if (stage == Stage::DECAY) {
			samplesUntilTransition = (size_t)(data->sampleRate * data->stageValues[(int)Stage::DECAY]);
			level = 1.f;
			summand = (data->stageValues[(int)Stage::SUSTAIN] - level) / samplesUntilTransition;
		}
		else if (stage == Stage::SUSTAIN) {
			samplesUntilTransition = (size_t)-1;
			level = data->stageValues[(int)Stage::SUSTAIN];
			summand = 0.f;
		}
		else if (stage == Stage::RELEASE) {
			samplesUntilTransition = (size_t)(data->sampleRate * data->stageValues[(int)Stage::RELEASE]);
			summand = (data->minLevel - level) / samplesUntilTransition;
		}
		else {
			jassert(stage == Stage::IDLE);
			reset();
		}
	}

	bool InterpolationOsc::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{
		if (data->bypassed || context.isBypassed) {
			return false;
		}

		if (!data->modParams[SharedData::ENV].isActive() && !isNoteOn && !shouldStopCleanly) {
			return false;
		}
		auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
		auto& outputBlock = context.getOutputBlock();

		// prepare for modulation

		//pitch 
		auto basePitch = data->pitch;
		auto pitchMod = data->modParams[SharedData::PITCH].factor;
		auto pitchModSrc = inputBlock.getChannelPointer((size_t)data->modParams[SharedData::PITCH].src_channel);

		// wtPosition and waveform in general
		auto baseWtPos = data->wtPos;
		auto wtPosMod = data->modParams[SharedData::WT_POS].factor;
		auto wtPosModSrc = inputBlock.getChannelPointer((size_t)data->modParams[SharedData::WT_POS].src_channel);

		// calculate wave only once on a work buffer and then add it to all actual output buffers
		jassert(WORK_BUFFERS >= 1);
		jassert(outputBlock.getNumSamples() == workBuffers.getNumSamples());
		auto tmpPtr = workBuffers.getChannelPointer(0);

		// disregard pitch modulation when considering harmonics
		auto& wt = data->wt->getTable(frequency * std::exp2f((basePitch + pitchMod * pitchModSrc[0]) / 12.f));

		for (int start = 0; start < workBuffers.getNumSamples(); start += configuration::MOD_BLOCK_SIZE) {

			auto end = juce::jmin(start + configuration::MOD_BLOCK_SIZE, (int)workBuffers.getNumSamples());

			// calculate modulated arguments for the next ~MOD_BLOCK_SIZE samples
			auto pitch = basePitch + pitchMod * pitchModSrc[start];
			auto actualFrequency = (frequency * std::exp2f(pitch / 12.f));
			float phaseStep = (juce::MathConstants<float>::twoPi * actualFrequency) / (float)data->sampleRate;
			auto wtPos = juce::jlimit(0.f, 1.f, baseWtPos + wtPosMod * wtPosModSrc[start]);

			auto scaledWtPos = wtPos * (wt.getNumChannels() - 1 - 1);
			int channelIndex = static_cast<int>(scaledWtPos);
			auto channelDelta = scaledWtPos - channelIndex;

			auto channel0 = wt.getReadPointer(channelIndex);
			auto channel1 = wt.getReadPointer(channelIndex + 1);

			for (int i = start; i < end; i++) {

				auto x = phase.advance(phaseStep);

				auto scaledX = (x / juce::MathConstants<float>::twoPi) * (wt.getNumSamples() - 1);
				int sampleIndex = static_cast<int>(scaledX);
				auto xDelta = scaledX - sampleIndex;
				auto sample0 = juce::jmap(xDelta, channel0[sampleIndex], channel0[sampleIndex + 1]);
				auto sample1 = juce::jmap(xDelta, channel1[sampleIndex], channel1[sampleIndex + 1]);

				auto sample = juce::jmap(channelDelta, sample0, sample1);

				// important to replace instead of add here
				tmpPtr[i] = sample;
			}
			// apply envelope, velocity and a multiplier so that more than one voice without reaching 0dB
			auto multiplier = data->modParams[SharedData::ENV].isActive() ?
				inputBlock.getSample((size_t)data->modParams[SharedData::ENV].src_channel, start)
				: 1.f;
			multiplier *= 0.25f * velocity;
			workBuffers.getSingleChannelBlock(0).getSubBlock(start, end - start).multiplyBy(multiplier);
		}

		// prevent clicking when stopping without an envelope
		if (shouldStopCleanly) {
			auto numSamples = workBuffers.getNumSamples();
			auto samplesToFade = juce::jmin(currentReleaseSamples, static_cast<int>(numSamples));
			auto* channelPtr = workBuffers.getChannelPointer(0);
			float maxReleaseSamples2f = maxReleaseSamples * maxReleaseSamples;
			for (int i = 0; i < samplesToFade; i++) {
				channelPtr[i] *= currentReleaseSamples * currentReleaseSamples / maxReleaseSamples2f;
				currentReleaseSamples--;
			}
			jassert(0 <= currentReleaseSamples);
			if (currentReleaseSamples == 0) {
				shouldStopCleanly = false;
				for (int i = samplesToFade; i < numSamples; i++) {
					channelPtr[i] = 0.f;
				}
			}

		}

		for (size_t channel = 0; channel < outputBlock.getNumChannels(); channel++) {
			outputBlock.getSingleChannelBlock(channel).add(workBuffers.getSingleChannelBlock(0));
		}

		return isNoteOn || shouldStopCleanly;
	};

	bool LFO::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{
		auto& outputBlock = context.getOutputBlock();

		float phaseStep = (juce::MathConstants<float>::twoPi * data->rate) / (float)data->sampleRate;

		auto& wt = data->wt->getTable(30.f); // just get a low freq table

		auto wtPos = data->wtPos;
		auto scaledWtPos = wtPos * (wt.getNumChannels() - 1 - 1);
		int channelIndex = static_cast<int>(scaledWtPos);
		auto channelDelta = scaledWtPos - channelIndex;

		auto channel0 = wt.getReadPointer(channelIndex);
		auto channel1 = wt.getReadPointer(channelIndex + 1);

		for (int start = 0; start < outputBlock.getNumSamples(); start += configuration::MOD_BLOCK_SIZE) {
			auto length = juce::jmin(start + configuration::MOD_BLOCK_SIZE, (int)outputBlock.getNumSamples()) - start;

			auto x = phase.advance(phaseStep * length);

			auto scaledX = (x / juce::MathConstants<float>::twoPi) * (wt.getNumSamples() - 1);
			int sampleIndex = static_cast<int>(scaledX);
			auto xDelta = scaledX - sampleIndex;
			auto sample0 = juce::jmap(xDelta, channel0[sampleIndex], channel0[sampleIndex + 1]);
			auto sample1 = juce::jmap(xDelta, channel1[sampleIndex], channel1[sampleIndex + 1]);

			auto sample = juce::jmap(channelDelta, sample0, sample1);
			outputBlock.getSubBlock(start, length).add(sample);
		}

		return isNoteOn;
	};


	bool Gain::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{

		if (context.isBypassed) {
			return false;
		}

		auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
		auto& outputBlock = context.getOutputBlock();

		auto baseGain = data->gain;
		auto gainMod = data->modParams[SharedData::GAIN].factor;
		auto gainModSrc = inputBlock.getChannelPointer((size_t)data->modParams[SharedData::GAIN].src_channel);

		auto workPtr = workBuffers.getChannelPointer(0);
		for (int start = 0; start < workBuffers.getNumSamples(); start += configuration::MOD_BLOCK_SIZE) {
			auto end = juce::jmin(start + configuration::MOD_BLOCK_SIZE, (int)workBuffers.getNumSamples());

			gainSmoothed.reset(end - start);
			gainSmoothed.setTargetValue(juce::jlimit(0.f,1.f,baseGain + gainMod * gainModSrc[end]));

			for (int i = start; i < end; i++) {
				workPtr[i] = gainSmoothed.getNextValue();
			}
		}

		for (size_t channel = 0; channel < outputBlock.getNumChannels(); channel++) {
			outputBlock.getSingleChannelBlock(channel).multiplyBy(workBuffers.getSingleChannelBlock(0));
		}

		return isNoteOn;
	};

	bool Pan::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{
		if (context.isBypassed) {
			return false;
		}

		auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
		auto& outputBlock = context.getOutputBlock();

		jassert(outputBlock.getNumChannels() == 2);
		auto left = outputBlock.getSingleChannelBlock(0);
		auto right = outputBlock.getSingleChannelBlock(1);
		// prepare for modulation

		// pan
		auto basePan = data->pan;
		auto panMod = data->modParams[SharedData::PAN].factor;
		auto panModSrc = inputBlock.getChannelPointer((size_t)data->modParams[SharedData::PAN].src_channel);

		for (int start = 0; start < workBuffers.getNumSamples(); start += configuration::MOD_BLOCK_SIZE) {

			auto end = juce::jmin(start + configuration::MOD_BLOCK_SIZE, (int)workBuffers.getNumSamples());

			auto currentPan = juce::jlimit(-1.f, 1.f, basePan + panMod * panModSrc[start]);

			left.getSubBlock(start, end - start).multiplyBy(1.f - currentPan);
			right.getSubBlock(start, end - start).multiplyBy(1.f + currentPan);
		}

		return false;
	};

}