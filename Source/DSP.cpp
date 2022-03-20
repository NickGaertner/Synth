
#include "DSP.h"

namespace customDsp {
	// TODO delete
	juce::dsp::LadderFilter<float> a;
	juce::dsp::StateVariableFilter::Filter<float> b;

	void Envelope::reset() {
		stage = Stage::IDLE;
		level = 0.f;
		summand = 0.f;
		samplesUntilTransition = (size_t)-1;
	}

	void Envelope::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) {
		juce::ignoreUnused(workBuffers);
		if (stage == Stage::IDLE) {
			return;
		}
		auto& outputBlock = context.getOutputBlock();
		size_t currentPos = 0;
		size_t samplesToProcess = outputBlock.getNumSamples();

		while (currentPos < samplesToProcess) {
			if (samplesUntilTransition == 0) {
				transition();
			}
			auto samplesThisStep = samplesUntilTransition > 0 ?
				juce::jmin(samplesUntilTransition, samplesToProcess) : samplesToProcess;

			auto tmp_level = level;
			for (size_t channel = 0; channel < outputBlock.getNumChannels(); channel++) {
				tmp_level = level;
				auto channelPtr = outputBlock.getChannelPointer(channel);
				for (size_t i = currentPos; i < samplesThisStep; i++) {
					channelPtr[i] = tmp_level;
					tmp_level += summand;
				}
			}

			level = tmp_level;
			currentPos += samplesThisStep;
			samplesUntilTransition -= samplesThisStep;
		}
	};

	void Envelope::noteOn() {
		reset();
		transition();
	}
	void Envelope::noteOff() {
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

	void InterpolationOsc::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{
		if (data->bypassed) {
			return;
		}

		if (context.isBypassed) {
			jassertfalse; // normally shouldn't land here
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

		// envelope
		std::function<float(int)> env; // TODO refactor with if
		auto envChannel = getEnvChannel();

		if (envChannel == -1) { // no envelope connected
			env = [&](int) { return 1.f; };
		}
		else {
			env = [&](int sample) { return inputBlock.getSample(envChannel, sample); };
		}

		// calculate wave only once on a work buffer and then add it to all actual output buffers
		jassert(WORK_BUFFERS >= 1);
		jassert(outputBlock.getNumSamples() == workBuffers.getNumSamples());
		auto tmpPtr = workBuffers.getChannelPointer(0);

		// disregard pitch modulation when considering harmonics
		int exponent;
		std::frexpf(frequency * std::powf(2.f, (basePitch + pitchMod * pitchModSrc[0]) / 12.f), &exponent);

		for (int i = 0; i < workBuffers.getNumSamples(); i++) {
			auto pitch = basePitch + pitchMod * pitchModSrc[i];
			auto actualFrequency = (frequency * std::powf(2.f, pitch / 12.f));
			float phaseStep = (juce::MathConstants<float>::twoPi * actualFrequency) / (float)data->sampleRate;
			auto wtPos = baseWtPos + wtPosMod * wtPosModSrc[i];
			auto x = phase.advance(phaseStep);
			// important to replace instead of add here
			tmpPtr[i] = (data->wt)->getSample(exponent, wtPos, x) * 0.125f * velocity * env(i);
		}

		for (size_t channel = 0; channel < outputBlock.getNumChannels(); channel++) {
			outputBlock.getSingleChannelBlock(channel).add(workBuffers.getSingleChannelBlock(0));
		}
	};


}