#pragma once

#include "JuceHeader.h"

namespace customDsp {

	struct ModulationParam {
		enum class Channel {
			ENV_0,
			ENV_1,
			LFO_0,
			LFO_1,
			NONE
		};

		float modifier = 0.f;
		Channel src_channel = Channel::NONE;
		std::function<float(float, float)> modulation; // only updated if isActive()

		bool isActive() {
			return src_channel != Channel::NONE;
		}

		bool isLfo() {
			return isActive() && src_channel >= Channel::LFO_0;
		}

		bool isEnv() {
			return src_channel <= Channel::ENV_1;
		};

		static void addModParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& name) {
			layout.add(std::make_unique<juce::AudioParameterChoice>(
				name + "::MOD_CHANNEL",
				name + "::MOD_CHANNEL",
				juce::StringArray{ "Env0", "Env1", "Lfo0", "Lfo1", "None" },
				Channel::NONE));
			layout.add(std::make_unique<juce::AudioParameterFloat>(
				name + "::MOD_MODIFIER",
				name + "::MOD_MODIFIER",
				juce::NormalisableRange<float>(-1.f, 1.0f, 0.025f, 1.f),
				0.f));
		}

		void updateModParams(const juce::AudioProcessorValueTreeState& apvts, const juce::String& name) {
			modifier = apvts.getRawParameterValue(name + "::MOD_MODIFIER")->load();
			src_channel = static_cast<Channel>(apvts.getRawParameterValue(name + "::MOD_CHANNEL")->load());
			if (isActive()) {
				modulation = [&](float x, float modValue) { return x + modifier * modValue; };
			}
		}

	};


	class Processor {
	public:

		struct SharedData {
			float sampleRate{ 0.f };
			juce::String prefix;

			SharedData() = delete;
			SharedData(const juce::String& t_prefix) : prefix(t_prefix) {};
			virtual ~SharedData() {};
			virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) = 0;
			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) = 0;
			virtual Processor* createProcessor() = 0;

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		virtual ~Processor() {};

		virtual void prepare(const juce::dsp::ProcessSpec&) = 0;

		virtual void reset() = 0;

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>&) = 0;

		virtual void noteOn() {};

		virtual void noteOff() {};

	private:
		JUCE_LEAK_DETECTOR(Processor)
	};


	class ProcessorChain : public Processor {
	public:
		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->prepare(spec); });
		};

		virtual void reset() override {
			std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->reset(); });
		};

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context) override {
			std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->process(context); });
		};

		virtual void noteOn() override {
			std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->noteOn(); });
		}

		virtual void noteOff() override {
			std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->noteOff(); });
		};

		virtual void addProcessor(Processor* processor) {
			processors.add(processor);
		}

		virtual Processor* getProcessor(int index) {
			return processors[index];
		}

		int size() {
			return processors.size();
		}

	protected:
		juce::OwnedArray<Processor> processors;

	private:

		JUCE_LEAK_DETECTOR(ProcessorChain)
	};


	class SplitProcessor : public ProcessorChain {
	public:

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context) override {
			jassert(processors.size() <= context.getOutputBlock().getNumChannels());
			for (size_t i = 0; i < processors.size(); i++) {
				auto singleOutputChannel = context.getOutputBlock().getSingleChannelBlock(i);
				juce::dsp::ProcessContextNonReplacing<float> tmp_context{ context.getInputBlock(), singleOutputChannel };
				processors[i]->process(tmp_context);
			}
		};

	private:
		JUCE_LEAK_DETECTOR(SplitProcessor)
	};


	class InterpolationOsc : public Processor {
	public:

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			float wtPos, pitch;
			std::function<float(float)> wf0, wf1;
			ModulationParam modParams[2];
			enum {
				ENV,
				WT_POS,
			};

			virtual InterpolationOsc* createProcessor() override {
				return new InterpolationOsc(this);
			};

			virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) override {

				modParams[ENV].updateModParams(apvts, prefix + "ENV");

				wtPos = apvts.getRawParameterValue(prefix + "WT_POS")->load();
				modParams[WT_POS].updateModParams(apvts, prefix + "WT_POS");

				pitch = apvts.getRawParameterValue(prefix + "PITCH")->load();

				wf0 = waveFormFunctions[static_cast<Waveform>(apvts.getRawParameterValue(prefix + "WF_0")->load())];
				wf1 = waveFormFunctions[static_cast<Waveform>(apvts.getRawParameterValue(prefix + "WF_1")->load())];

			}

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {
				
				ModulationParam::addModParams(layout, prefix + "ENV");

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + "WT_POS",
					prefix + "WT_POS",
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.025f, 1.f),
					0.f));
				ModulationParam::addModParams(layout, prefix + "WT_POS");

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + "PITCH",
					prefix + "PITCH",
					juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f, 1.f),
					0.f));

				layout.add(std::make_unique<juce::AudioParameterChoice>(
					prefix + "WF_0",
					prefix + "WF_0",
					waveFormStrings,
					0));
				layout.add(std::make_unique<juce::AudioParameterChoice>(
					prefix + "WF_1",
					prefix + "WF_1",
					waveFormStrings,
					0));

			}

			enum Waveform {
				Sine,
				Saw,
				Square,
				Triangle
			};

			inline static const float SINE_FUNC(float x) {
				return std::sinf(x);
			}

			inline static const float SAW_FUNC(float x) {
				jassert(0.f <= x && x <= juce::MathConstants<float>::twoPi);
				return (x / juce::MathConstants<float>::pi) - 1;
			}

			inline static const float SQUARE_FUNC(float x) {
				jassert(0.f <= x && x <= juce::MathConstants<float>::twoPi);
				return x >= juce::MathConstants<float>::pi ? -1.f : 1.f;
			}

			inline static const float TRIANGLE_FUNC(float x) {
				jassert(0.f <= x && x <= juce::MathConstants<float>::twoPi);
				float norm_x = x / juce::MathConstants<float>::pi; // range [0,2]
				if (norm_x < 1.f) {
					return -1 + 2 * norm_x;
				}
				else {
					return 3 - 2 * norm_x;
				}
			}

			const std::function<float(float)> waveFormFunctions[4]{ SINE_FUNC, SAW_FUNC, SQUARE_FUNC, TRIANGLE_FUNC };
			juce::StringArray waveFormStrings{ "Sine", "Saw", "Square", "Triangle" };

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		InterpolationOsc() = delete;

		InterpolationOsc(SharedData* t_data) : data(t_data) {};

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			data->sampleRate = spec.sampleRate;
		};

		virtual void reset() override {
			phase.reset();
		}

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context) override {
			auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
			auto& outputBlock = context.getOutputBlock();

			if (context.isBypassed) {
				jassertfalse; // normally shouldn't land here
			}

			// prepare for modulation

			// pitch
			auto phaseStep = (juce::MathConstants<float>::twoPi * (frequency * std::powf(2.f, data->pitch / 12.f))) / data->sampleRate;

			// wtPosition and waveform in general
			std::function<float(float, float)> wf;

			if (data->modParams[SharedData::WT_POS].isActive()) {
				auto modChannel = data->modParams[SharedData::WT_POS].src_channel;
				auto& modulate = data->modParams[SharedData::WT_POS].modulation;
				wf = [&](float phase, size_t sample) {
					auto modWtPos = modulate(data->wtPos, inputBlock.getSample((int)modChannel, sample));
					return ((1.f - modWtPos) * data->wf0(phase) + modWtPos * data->wf1(phase)) * 0.0625; };
			}
			else {
				wf = [&](float phase, size_t sample) {
					return ((1.f - data->wtPos) * data->wf0(phase) + data->wtPos * data->wf1(phase)) * 0.0625; };
			}


			// envelope
			std::function<float(size_t)> env;
			auto envChannel = getEnvChannel();

			if (envChannel == -1) { // no envelope connected
				env = [&](size_t sample) { return 1.f; };
			}
			else {
				env = [&](size_t sample) { return inputBlock.getSample(envChannel, sample); };
			}

			auto tmp_phase = phase;

			for (size_t channel = 0; channel < outputBlock.getNumChannels(); channel++) {
				tmp_phase = phase;
				auto channelPtr = outputBlock.getChannelPointer(channel);

				for (size_t i = 0; i < outputBlock.getNumSamples(); i++) {
					channelPtr[i] += wf(tmp_phase.advance(phaseStep), i) * velocity * env(i);
				}

			}

			phase = tmp_phase;
		};

		int getEnvChannel() {
			auto& envModParams = data->modParams[SharedData::ENV];
			return envModParams.isEnv() ? (int)envModParams.src_channel : -1;
		}

		void setFrequency(float t_frequency, bool force = true) {
			frequency = t_frequency;
		}

		void setVelocity(float t_velocity)
		{
			velocity = t_velocity;
		}

	private:
		SharedData* data;

		float frequency = 440.f;
		float velocity = 0.f;
		juce::dsp::Phase<float> phase;

		JUCE_LEAK_DETECTOR(InterpolationOsc);
	};


	class Envelope : public Processor {
	public:

		enum class Stage {
			IDLE,
			ATTACK,
			DECAY,
			SUSTAIN,
			RELEASE,
			STAGE_NUMBER
		};

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			float stageValues[(int)Stage::STAGE_NUMBER];
			const float minLevel = 0.001f;


			virtual Envelope* createProcessor() override {
				return new Envelope(this);
			};

			virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) override {
				stageValues[(int)Stage::ATTACK] = apvts.getRawParameterValue(prefix + "ATTACK")->load();
				stageValues[(int)Stage::DECAY] = apvts.getRawParameterValue(prefix + "DECAY")->load();
				stageValues[(int)Stage::SUSTAIN] = apvts.getRawParameterValue(prefix + "SUSTAIN")->load();
				stageValues[(int)Stage::RELEASE] = apvts.getRawParameterValue(prefix + "RELEASE")->load();
			}

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + "ATTACK",
					prefix + "ATTACK",
					juce::NormalisableRange<float>(0.0f, 10.0f, 0.025f, 0.5f),
					0.5f));
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + "DECAY",
					prefix + "DECAY",
					juce::NormalisableRange<float>(0.0f, 10.0f, 0.025f, 0.5f),
					0.2f));
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + "SUSTAIN",
					prefix + "SUSTAIN",
					juce::NormalisableRange<float>(minLevel, 1.0f, 0.025f, 0.5f),
					0.8f));
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + "RELEASE",
					prefix + "RELEASE",
					juce::NormalisableRange<float>(0.0f, 10.0f, 0.025f, 0.5f),
					0.4f));
			}

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		Envelope() = delete;

		Envelope(SharedData* t_data) : data(t_data) {};


		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			data->sampleRate = spec.sampleRate;
		};

		virtual void reset() override {
			stage = Stage::IDLE;
			level = 0.f;
			multiplier = 0.f;
			samplesUntilTransition = -1;
		}

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context) override {
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
						tmp_level *= multiplier;
					}
				}

				level = tmp_level;
				currentPos += samplesThisStep;
				samplesUntilTransition -= samplesThisStep;
			}
		};


		virtual void noteOn() override {
			reset();
			transition();
		}

		virtual void noteOff() override {
			if (stage != Stage::IDLE) {
				stage = Stage::SUSTAIN;
				transition();
			}
		}

		bool isIdle() { return stage == Stage::IDLE; }

	private:

		void transition() {
			do {
				advanceStage();
			} while (stage != Stage::IDLE && data->stageValues[(int)stage] == 0.f);

			if (stage == Stage::ATTACK) {
				samplesUntilTransition = data->sampleRate * data->stageValues[(int)Stage::ATTACK];
				level = data->minLevel;
				multiplier = getMultiplier(level, 1.f, samplesUntilTransition);
			}
			else if (stage == Stage::DECAY) {
				samplesUntilTransition = data->sampleRate * data->stageValues[(int)Stage::DECAY];
				level = 1.f;
				multiplier = getMultiplier(level, data->stageValues[(int)Stage::SUSTAIN], samplesUntilTransition);
			}
			else if (stage == Stage::SUSTAIN) {
				samplesUntilTransition = -1;
				level = data->stageValues[(int)Stage::SUSTAIN];
				multiplier = 1.f;
			}
			else if (stage == Stage::RELEASE) {
				samplesUntilTransition = data->sampleRate * data->stageValues[(int)Stage::RELEASE];
				multiplier = getMultiplier(level, data->minLevel, samplesUntilTransition);
			}
			else {
				jassert(stage == Stage::IDLE);
				reset();
			}
		}

		static inline float getMultiplier(float startLevel, float targetLevel, size_t samples) {
			jassert(targetLevel > 0);
			return std::powf(targetLevel / startLevel, 1.f / samples);
		}

		inline void advanceStage() {
			stage = static_cast<Stage> (((int)stage + 1) % (int)Stage::STAGE_NUMBER);
		}

		SharedData* data;
		Stage stage = Stage::IDLE;
		float level = 0.f;
		float multiplier = 0.f;
		size_t samplesUntilTransition = 0;

		JUCE_LEAK_DETECTOR(Envelope);
	};
}