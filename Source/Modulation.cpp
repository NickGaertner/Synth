
#include "Modulation.h"

namespace customDsp {

	 Envelope* Envelope::SharedData::createProcessor()  {
		return new Envelope(this);
	};

	 void Envelope::SharedData::addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout)  {
		layout.add(std::make_unique<juce::AudioParameterFloat>(
			prefix + configuration::ATTACK_SUFFIX,
			prefix + configuration::ATTACK_SUFFIX,
			juce::NormalisableRange<float>(0.00f, 10.0f, 0.001f, 0.35f),
			stageValues[(int)Stage::ATTACK]));
		layout.add(std::make_unique<juce::AudioParameterFloat>(
			prefix + configuration::DECAY_SUFFIX,
			prefix + configuration::DECAY_SUFFIX,
			juce::NormalisableRange<float>(0.00f, 10.0f, 0.001f, 0.35f),
			stageValues[(int)Stage::DECAY]));
		layout.add(std::make_unique<juce::AudioParameterFloat>(
			prefix + configuration::SUSTAIN_SUFFIX,
			prefix + configuration::SUSTAIN_SUFFIX,
			juce::NormalisableRange<float>(minLevel, 1.0f, 0.001f, 1.f),
			stageValues[(int)Stage::SUSTAIN]));
		layout.add(std::make_unique<juce::AudioParameterFloat>(
			prefix + configuration::RELEASE_SUFFIX,
			prefix + configuration::RELEASE_SUFFIX,
			juce::NormalisableRange<float>(0.05f, 10.0f, 0.001f, 0.35f),
			stageValues[(int)Stage::RELEASE]));
	}

	 void Envelope::SharedData::registerAsListener(juce::AudioProcessorValueTreeState& apvts)  {
		apvts.addParameterListener(prefix + configuration::ATTACK_SUFFIX, this);
		apvts.addParameterListener(prefix + configuration::DECAY_SUFFIX, this);
		apvts.addParameterListener(prefix + configuration::SUSTAIN_SUFFIX, this);
		apvts.addParameterListener(prefix + configuration::RELEASE_SUFFIX, this);
	}

	 void Envelope::SharedData::parameterChanged(const juce::String& parameterID, float newValue)  {
		if (parameterID.endsWith(configuration::ATTACK_SUFFIX)) {
			stageValues[(int)Stage::ATTACK] = newValue;
		}
		else if (parameterID.endsWith(configuration::DECAY_SUFFIX)) {
			stageValues[(int)Stage::DECAY] = newValue;
		}
		else if (parameterID.endsWith(configuration::SUSTAIN_SUFFIX)) {
			stageValues[(int)Stage::SUSTAIN] = newValue;
		}
		else if (parameterID.endsWith(configuration::RELEASE_SUFFIX)) {
			stageValues[(int)Stage::RELEASE] = newValue;
		}
		else {
			jassertfalse;
		}
	}

	 void Envelope::prepare(const juce::dsp::ProcessSpec& spec)  {
		data->sampleRate = spec.sampleRate;
	};

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

	bool Envelope::isIdle() { return stage == Stage::IDLE; }

	 LFO* LFO::SharedData::createProcessor()  {
		return new LFO(this);
	};

	 void LFO::SharedData::addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout)  {

		layout.add(std::make_unique<juce::AudioParameterFloat>(
			prefix + configuration::WT_POS_SUFFIX,
			prefix + configuration::WT_POS_SUFFIX,
			juce::NormalisableRange<float>(0.0f, 1.0f, 0.025f, 1.f),
			wtPos));

		layout.add(std::make_unique<juce::AudioParameterFloat>(
			prefix + configuration::RATE_SUFFIX,
			prefix + configuration::RATE_SUFFIX,
			juce::NormalisableRange<float>(0.001f, 20.0f, 0.0001f, 0.5f),
			rate));

		layout.add(std::make_unique<juce::AudioParameterChoice>(
			prefix + configuration::WT_SUFFIX,
			prefix + configuration::WT_SUFFIX,
			wavetable::WavetableCache::getInstance()->getWavetableNames(),
			0));
	}

	 void LFO::SharedData::registerAsListener(juce::AudioProcessorValueTreeState& apvts)  {
		apvts.addParameterListener(prefix + configuration::WT_POS_SUFFIX, this);
		apvts.addParameterListener(prefix + configuration::RATE_SUFFIX, this);
		apvts.addParameterListener(prefix + configuration::WT_SUFFIX, this);
	}

	 void LFO::SharedData::parameterChanged(const juce::String& parameterID, float newValue)  {
		if (parameterID.endsWith(configuration::WT_POS_SUFFIX)) {
			wtPos = newValue;
		}
		else if (parameterID.endsWith(configuration::RATE_SUFFIX)) {
			rate = newValue;
		}
		else if (parameterID.endsWith(configuration::WT_SUFFIX)) {
			wt = wavetable::WavetableCache::getInstance()->getWavetable(static_cast<int>(newValue));
		}
		else {
			jassertfalse;
		}
	}

	 void LFO::prepare(const juce::dsp::ProcessSpec& spec)  {
		data->sampleRate = spec.sampleRate;
	};

	 void LFO::reset()  {
		phase.reset();
	}

	bool LFO::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{
		juce::ignoreUnused(workBuffers);
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

	void LFO::advancePhase(float step) {
		jassert(0.f <= step && step < juce::MathConstants<float>::twoPi);
		phase.advance(step);
	}
}