
#include "DSP.h"

namespace customDsp {

	bool ModulationParam::isActive() {
		return src_channel != configuration::EMPTY_MOD_CHANNEL;
	}

	bool ModulationParam::isLfo() {
		return isActive() && src_channel >= configuration::ENV_NUMBER;
	}

	bool ModulationParam::isEnv() {
		return src_channel < configuration::ENV_NUMBER;
	}

	void ModulationParam::addModParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& name,
		float modRange, float intervalValue)
	{
		layout.add(std::make_unique<juce::AudioParameterFloat>(
			name + configuration::MOD_FACTOR_SUFFIX,
			name + configuration::MOD_FACTOR_SUFFIX,
			juce::NormalisableRange<float>(-1.f * modRange, 1.0f * modRange, intervalValue, 1.f),
			factor));
		layout.add(std::make_unique<juce::AudioParameterChoice>(
			name + configuration::MOD_CHANNEL_SUFFIX,
			name + configuration::MOD_CHANNEL_SUFFIX,
			configuration::getModChannelNames(),
			src_channel));
	}

	void ModulationParam::registerAsListener(juce::AudioProcessorValueTreeState& apvts, const juce::String& name) {
		apvts.addParameterListener(name + configuration::MOD_CHANNEL_SUFFIX, this);
		apvts.addParameterListener(name + configuration::MOD_FACTOR_SUFFIX, this);
	}

	void ModulationParam::parameterChanged(const juce::String& parameterID, float newValue) {
		if (parameterID.endsWith(configuration::MOD_CHANNEL_SUFFIX)) {
			src_channel = (int)newValue;
		}
		else if (parameterID.endsWith(configuration::MOD_FACTOR_SUFFIX)) {
			factor = newValue;
		}
		else {
			jassertfalse;
		}
	}

	void Processor::noteOn() {
		isNoteOn = true;
	}

	void Processor::noteOff() {
		isNoteOn = false;
	}

	void ProcessorChain::prepare(const juce::dsp::ProcessSpec& spec) {
		std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->prepare(spec); });
	};

	void ProcessorChain::reset() {
		std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->reset(); });
	};

	bool ProcessorChain::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) {
		bool needMoreTime = false;
		std::for_each(processors.begin(), processors.end(), [&](Processor* p) {
			needMoreTime |= p->process(context, workBuffers);
			});
		return needMoreTime;
	}

	void ProcessorChain::noteOn() {
		Processor::noteOn();
		std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->noteOn(); });
	}

	void ProcessorChain::noteOff() {
		Processor::noteOff();
		std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->noteOff(); });
	};

	void ProcessorChain::addProcessor(Processor* processor) {
		processors.add(processor);
	}

	Processor* ProcessorChain::getProcessor(int index) {
		return processors[index];
	}

	int ProcessorChain::size() {
		return processors.size();
	}

	bool SplitProcessor::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) {
		jassert(processors.size() < context.getOutputBlock().getNumChannels());
		bool needMoreTime = false;
		for (int i = 0; i < processors.size(); i++) {
			auto singleOutputChannel = context.getOutputBlock().getSingleChannelBlock(i);
			juce::dsp::ProcessContextNonReplacing<float> tmp_context{ context.getInputBlock(), singleOutputChannel };
			needMoreTime |= processors[i]->process(tmp_context, workBuffers);
		}
		return needMoreTime;
	};

	InterpolationOsc* InterpolationOsc::SharedData::createProcessor() {
		return new InterpolationOsc(this);
	};

	void InterpolationOsc::SharedData::addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {

		layout.add(std::make_unique<juce::AudioParameterBool>(
			prefix + configuration::BYPASSED_SUFFIX,
			prefix + configuration::BYPASSED_SUFFIX,
			bypassed));

		modParams[ENV].addModParams(layout, prefix + configuration::ENV_SUFFIX);

		layout.add(std::make_unique<juce::AudioParameterFloat>(
			prefix + configuration::WT_POS_SUFFIX,
			prefix + configuration::WT_POS_SUFFIX,
			juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.f),
			wtPos));
		modParams[WT_POS].addModParams(layout, prefix + configuration::WT_POS_SUFFIX);

		layout.add(std::make_unique<juce::AudioParameterFloat>(
			prefix + configuration::PITCH_SUFFIX,
			prefix + configuration::PITCH_SUFFIX,
			juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f, 1.f),
			pitch));
		modParams[PITCH].addModParams(layout, prefix + configuration::PITCH_SUFFIX, 24.f, 0.01f);

		layout.add(std::make_unique<juce::AudioParameterChoice>(
			prefix + configuration::WT_SUFFIX,
			prefix + configuration::WT_SUFFIX,
			wavetable::WavetableCache::getInstance()->getWavetableNames(),
			0));
	}

	void InterpolationOsc::SharedData::registerAsListener(juce::AudioProcessorValueTreeState& apvts) {
		apvts.addParameterListener(prefix + configuration::BYPASSED_SUFFIX, this);
		modParams[ENV].registerAsListener(apvts, prefix + configuration::ENV_SUFFIX);
		apvts.addParameterListener(prefix + configuration::WT_POS_SUFFIX, this);
		modParams[WT_POS].registerAsListener(apvts, prefix + configuration::WT_POS_SUFFIX);
		apvts.addParameterListener(prefix + configuration::PITCH_SUFFIX, this);
		modParams[PITCH].registerAsListener(apvts, prefix + configuration::PITCH_SUFFIX);
		apvts.addParameterListener(prefix + configuration::WT_SUFFIX, this);
	}

	void InterpolationOsc::SharedData::parameterChanged(const juce::String& parameterID, float newValue) {
		if (parameterID.endsWith(configuration::BYPASSED_SUFFIX)) {
			bypassed = (bool)newValue;
		}
		else if (parameterID.endsWith(configuration::WT_POS_SUFFIX)) {
			wtPos = newValue;
		}
		else if (parameterID.endsWith(configuration::PITCH_SUFFIX)) {
			pitch = newValue;
		}
		else if (parameterID.endsWith(configuration::WT_SUFFIX)) {
			wt = wavetable::WavetableCache::getInstance()->getWavetable(static_cast<int>(newValue));
		}
		else {
			jassertfalse;
		}
	}

	void InterpolationOsc::prepare(const juce::dsp::ProcessSpec& spec) {
		data->sampleRate = spec.sampleRate;
	};

	void InterpolationOsc::reset() {
		phase.reset();
		shouldStopCleanly = false;
		currentReleaseSamples = 0;
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
			multiplier *= velocity / configuration::OSC_NUMBER;
			workBuffers.getSingleChannelBlock(0).getSubBlock(start, end - start).multiplyBy(multiplier);
		}

		// prevent clicking when stopping without an envelope
		if (shouldStopCleanly) {
			auto numSamples = workBuffers.getNumSamples();
			auto samplesToFade = juce::jmin(currentReleaseSamples, static_cast<int>(numSamples));
			auto* channelPtr = workBuffers.getChannelPointer(0);
			const float maxReleaseSamples2f = MAX_RELEASE_SAMPLES * MAX_RELEASE_SAMPLES;
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

	void InterpolationOsc::setFrequency(float t_frequency, bool force) {
		juce::ignoreUnused(force);
		frequency = t_frequency;
	}

	void InterpolationOsc::setVelocity(float t_velocity)
	{
		velocity = t_velocity;
	}

	int InterpolationOsc::getEnvChannel() {
		return data->modParams[SharedData::ENV].src_channel;
	}

	void InterpolationOsc::noteOff() {
		if (isNoteOn && !data->modParams[SharedData::ENV].isActive()) {
			shouldStopCleanly = true;
			currentReleaseSamples = MAX_RELEASE_SAMPLES;
		}
		Processor::noteOff();
	};

	Gain* Gain::SharedData::createProcessor() {
		return new Gain(this);
	};

	void Gain::SharedData::addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
		layout.add(std::make_unique<juce::AudioParameterFloat>(
			prefix + configuration::GAIN_SUFFIX,
			prefix + configuration::GAIN_SUFFIX,
			juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.f),
			gain));
		modParams[GAIN].addModParams(layout, prefix + configuration::GAIN_SUFFIX);
	}

	void Gain::SharedData::registerAsListener(juce::AudioProcessorValueTreeState& apvts) {
		apvts.addParameterListener(prefix + configuration::GAIN_SUFFIX, this);
		modParams[GAIN].registerAsListener(apvts, prefix + configuration::GAIN_SUFFIX);
	}

	void Gain::SharedData::parameterChanged(const juce::String& parameterID, float newValue) {
		if (parameterID.endsWith(configuration::GAIN_SUFFIX)) {
			gain = newValue;
		}
		else {
			jassertfalse;
		}
	}

	void Gain::prepare(const juce::dsp::ProcessSpec& spec) {
		data->sampleRate = spec.sampleRate;
	};

	void Gain::reset() {
		gainSmoothed.setCurrentAndTargetValue(data->gain);
	}

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
			gainSmoothed.setTargetValue(juce::jlimit(0.f, 1.f, baseGain + gainMod * gainModSrc[end]));

			for (int i = start; i < end; i++) {
				workPtr[i] = gainSmoothed.getNextValue();
			}
		}

		for (size_t channel = 0; channel < outputBlock.getNumChannels(); channel++) {
			outputBlock.getSingleChannelBlock(channel).multiplyBy(workBuffers.getSingleChannelBlock(0));
		}

		return isNoteOn;
	};

	Pan* Pan::SharedData::createProcessor() {
		return new Pan(this);
	};

	void Pan::SharedData::addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
		layout.add(std::make_unique<juce::AudioParameterFloat>(
			prefix + configuration::PAN_SUFFIX,
			prefix + configuration::PAN_SUFFIX,
			juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f, 1.f),
			pan));
		modParams[PAN].addModParams(layout, prefix + configuration::PAN_SUFFIX);
	}

	void Pan::SharedData::registerAsListener(juce::AudioProcessorValueTreeState& apvts) {
		apvts.addParameterListener(prefix + configuration::PAN_SUFFIX, this);
		modParams[PAN].registerAsListener(apvts, prefix + configuration::PAN_SUFFIX);
	}

	void Pan::SharedData::parameterChanged(const juce::String& parameterID, float newValue) {
		if (parameterID.endsWith(configuration::PAN_SUFFIX)) {
			pan = newValue;
		}
		else {
			jassertfalse;
		}
	}

	void Pan::prepare(const juce::dsp::ProcessSpec& spec) {
		data->sampleRate = spec.sampleRate;
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