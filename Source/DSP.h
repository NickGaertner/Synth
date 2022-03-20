#pragma once

#include <JuceHeader.h>
#include "Configuration.h"
#include "Wavetable.h"

// TODO rework skew values

// TODO split between header and cpp file

namespace customDsp {

	// number of channels to append to the outputBlock which can be freely used by processors to store
	// temporary data
	constexpr int WORK_BUFFERS = 2;


	struct ModulationParam {

		float factor = 0.f;
		int src_channel = configuration::EMPTY_MOD_CHANNEL;

		bool isActive() {
			return src_channel != configuration::EMPTY_MOD_CHANNEL;
		}

		bool isLfo() {
			return isActive() && src_channel >= configuration::ENV_NUMBER;
		}

		bool isEnv() {
			return src_channel < configuration::ENV_NUMBER;
		};

		static void addModParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& name, float modRange = 1.f) {
			layout.add(std::make_unique<juce::AudioParameterChoice>(
				name + configuration::MOD_CHANNEL_SUFFIX,
				name + configuration::MOD_CHANNEL_SUFFIX,
				configuration::getModChannelNames(),
				configuration::EMPTY_MOD_CHANNEL));
			layout.add(std::make_unique<juce::AudioParameterFloat>(
				name + configuration::MOD_FACTOR_SUFFIX,
				name + configuration::MOD_FACTOR_SUFFIX,
				juce::NormalisableRange<float>(-1.f * modRange, 1.0f * modRange, 0.01f, 1.f),
				0.f));
		}

		void updateModParams(const juce::AudioProcessorValueTreeState& apvts, const juce::String& name) {
			factor = apvts.getRawParameterValue(name + configuration::MOD_FACTOR_SUFFIX)->load();
			src_channel = (int)apvts.getRawParameterValue(name + configuration::MOD_CHANNEL_SUFFIX)->load();
		}

	};


	class Processor {
	public:

		struct SharedData {
			double sampleRate{ 0.f };
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
		Processor(){}

		virtual ~Processor() {}

		virtual void prepare(const juce::dsp::ProcessSpec&) = 0;

		virtual void reset() = 0;

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>&, juce::dsp::AudioBlock<float>&) = 0;

		virtual void noteOn() {};

		virtual void noteOff() {};

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
	};

	class DummyProcessor : public Processor {
	public:

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			virtual DummyProcessor* createProcessor() override {
				return new DummyProcessor(this);
			};

			virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) override {
				juce::ignoreUnused(apvts);
			}

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {
				juce::ignoreUnused(layout);
			}

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		DummyProcessor() = delete;

		DummyProcessor(SharedData* t_data) : data(t_data) {}


		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			data->sampleRate = spec.sampleRate;
		};

		virtual void reset() override {
		}

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override {
			juce::ignoreUnused(context, workBuffers);
		};

	private:
		SharedData* data;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DummyProcessor);
	};


	class ProcessorChain : public Processor {
		using Processor::Processor;
	public:
		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->prepare(spec); });
		};

		virtual void reset() override {
			std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->reset(); });
		};

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override {
			std::for_each(processors.begin(), processors.end(), [&](Processor* p) {p->process(context, workBuffers); });
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

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorChain)
	};


	class SplitProcessor : public ProcessorChain {
		using ProcessorChain::ProcessorChain;
	public:

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override {
			jassert(processors.size() < context.getOutputBlock().getNumChannels());
			for (int i = 0; i < processors.size(); i++) {
				auto singleOutputChannel = context.getOutputBlock().getSingleChannelBlock(i);
				juce::dsp::ProcessContextNonReplacing<float> tmp_context{ context.getInputBlock(), singleOutputChannel };
				processors[i]->process(tmp_context, workBuffers);
			}
		};

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitProcessor)
	};


	class InterpolationOsc : public Processor {
	public:

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			bool bypassed;
			float wtPos, pitch;
			const wavetable::Wavetable* wt;
			ModulationParam modParams[3];
			enum {
				ENV,
				WT_POS,
				PITCH,
			};

			virtual InterpolationOsc* createProcessor() override {
				return new InterpolationOsc(this);
			};

			virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) override {
				bypassed = apvts.getRawParameterValue(prefix + configuration::BYPASSED_SUFFIX)->load();

				modParams[ENV].updateModParams(apvts, prefix + configuration::ENV_SUFFIX);

				wtPos = apvts.getRawParameterValue(prefix + configuration::WT_POS_SUFFIX)->load();
				modParams[WT_POS].updateModParams(apvts, prefix + configuration::WT_POS_SUFFIX);

				pitch = apvts.getRawParameterValue(prefix + configuration::PITCH_SUFFIX)->load();
				modParams[PITCH].updateModParams(apvts, prefix + configuration::PITCH_SUFFIX);

				// TODO this should be slow. try to only update if necessary
				wt = wavetable::WavetableManager::getWavetable(
					static_cast<int>(apvts.getRawParameterValue(prefix + configuration::WT_SUFFIX)->load()), sampleRate);
			}

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {

				layout.add(std::make_unique<juce::AudioParameterBool>(
					prefix + configuration::BYPASSED_SUFFIX,
					prefix + configuration::BYPASSED_SUFFIX,
					false));

				ModulationParam::addModParams(layout, prefix + configuration::ENV_SUFFIX);

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::WT_POS_SUFFIX,
					prefix + configuration::WT_POS_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.f),
					0.f));
				ModulationParam::addModParams(layout, prefix + configuration::WT_POS_SUFFIX);

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::PITCH_SUFFIX,
					prefix + configuration::PITCH_SUFFIX,
					juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f, 1.f),
					0.f));
				ModulationParam::addModParams(layout, prefix + configuration::PITCH_SUFFIX, 24.f);

				layout.add(std::make_unique<juce::AudioParameterChoice>(
					prefix + configuration::WT_SUFFIX,
					prefix + configuration::WT_SUFFIX,
					wavetable::WavetableManager::getWavetableNames(),
					0));

			}

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		InterpolationOsc() = delete;

		InterpolationOsc(SharedData* t_data) : data(t_data) {}

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			data->sampleRate = spec.sampleRate;
		};

		virtual void reset() override {
			phase.reset();
		}

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

		inline int getEnvChannel() {
			auto& envModParams = data->modParams[SharedData::ENV];
			return envModParams.isEnv() ? (int)envModParams.src_channel : -1;
		}

		void setFrequency(float t_frequency, bool force = true) {
			juce::ignoreUnused(force);
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

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InterpolationOsc);
	};

	class LFO : public Processor {
	public:

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			float wtPos, rate;
			std::function<float(float)> wf0, wf1;
			

			virtual LFO* createProcessor() override {
				return new LFO(this);
			};

			virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) override {

				wtPos = apvts.getRawParameterValue(prefix + configuration::WT_POS_SUFFIX)->load();
				rate = apvts.getRawParameterValue(prefix + configuration::RATE_SUFFIX)->load();
				// will be changed later for (sample based) wavetables
				wf0 = waveFormFunctions[static_cast<Waveform>(
					apvts.getRawParameterValue(prefix + configuration::WF0_SUFFIX)->load())];
				wf1 = waveFormFunctions[static_cast<Waveform>(
					apvts.getRawParameterValue(prefix + configuration::WF1_SUFFIX)->load())];

			}

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::WT_POS_SUFFIX,
					prefix + configuration::WT_POS_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.025f, 1.f),
					0.f));
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::RATE_SUFFIX,
					prefix + configuration::RATE_SUFFIX,
					juce::NormalisableRange<float>(0.001f, 20.0f, 0.001f, 0.5f),
					2.f));

				layout.add(std::make_unique<juce::AudioParameterChoice>(
					prefix + configuration::WF0_SUFFIX,
					prefix + configuration::WF0_SUFFIX,
					waveFormStrings,
					0));
				layout.add(std::make_unique<juce::AudioParameterChoice>(
					prefix + configuration::WF1_SUFFIX,
					prefix + configuration::WF1_SUFFIX,
					waveFormStrings,
					1));
			}

			enum Waveform {
				Sine,
				Square,
				Triangle,
				Saw
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

			const std::function<float(float)> waveFormFunctions[4]{ SINE_FUNC, SQUARE_FUNC, TRIANGLE_FUNC, SAW_FUNC };
			juce::StringArray waveFormStrings{ "Sine", "Square", "Triangle", "Saw" };

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		LFO() = delete;

		LFO(SharedData* t_data) : data(t_data){}

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			data->sampleRate = spec.sampleRate;
		};

		virtual void reset() override {
			phase.reset();
		}

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override {
			auto& outputBlock = context.getOutputBlock();

			// calculate wave only once on a work buffer and then add it to all actual output buffers
			// (since this is a lfo, this is theoretically not needed)
			jassert(WORK_BUFFERS >= 1);
			jassert(outputBlock.getNumSamples() == workBuffers.getNumSamples());
			auto tmpPtr = workBuffers.getChannelPointer(0);

			float phaseStep = (juce::MathConstants<float>::twoPi * data->rate) / (float)data->sampleRate;
			auto wtPos = data->wtPos;
			for (size_t i = 0; i < workBuffers.getNumSamples(); i++) {
				auto x = phase.advance(phaseStep);
				// important to replace instead of add here
				tmpPtr[i] = ((1.f - wtPos) * data->wf0(x) + wtPos * data->wf1(x));
			}

			for (size_t channel = 0; channel < outputBlock.getNumChannels(); channel++) {
				outputBlock.getSingleChannelBlock(channel).add(workBuffers.getSingleChannelBlock(0));
			}
		};

	private:
		SharedData* data;

		juce::dsp::Phase<float> phase;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFO);
	};

	class Gain : public Processor {
	public:

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			float gain = 1.f;
			ModulationParam modParams[1];
			enum {
				GAIN
			};

			virtual Gain* createProcessor() override {
				return new Gain(this);
			};

			virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) override {
				gain = apvts.getRawParameterValue(prefix + configuration::GAIN_SUFFIX)->load();
				modParams[GAIN].updateModParams(apvts, prefix + configuration::GAIN_SUFFIX);
			}

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::GAIN_SUFFIX,
					prefix + configuration::GAIN_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.f),
					1.f));
				ModulationParam::addModParams(layout, prefix + configuration::GAIN_SUFFIX);
			}

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		Gain() = delete;

		Gain(SharedData* t_data) : data(t_data) {}


		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			data->sampleRate = spec.sampleRate;
		};

		virtual void reset() override {
		}

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override {
			auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
			auto& outputBlock = context.getOutputBlock();

			auto workChannel = workBuffers.getSingleChannelBlock(0);
			workChannel.replaceWithProductOf(
				inputBlock.getSingleChannelBlock((int)data->modParams[SharedData::GAIN].src_channel), data->modParams[SharedData::GAIN].factor);
			workChannel.add(data->gain);


			for (size_t channel = 0; channel < outputBlock.getNumChannels(); channel++) {
				outputBlock.getSingleChannelBlock(channel).multiplyBy(workChannel);
			}
		};

	private:
		SharedData* data;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Gain);
	};

	// TODO change function! this sounds like shit
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
			const float minLevel = juce::Decibels::decibelsToGain(-96.f);


			virtual Envelope* createProcessor() override {
				return new Envelope(this);
			};

			virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) override {
				stageValues[(int)Stage::ATTACK] = apvts.getRawParameterValue(prefix + configuration::ATTACK_SUFFIX)->load();
				stageValues[(int)Stage::DECAY] = apvts.getRawParameterValue(prefix + configuration::DECAY_SUFFIX)->load();
				stageValues[(int)Stage::SUSTAIN] = apvts.getRawParameterValue(prefix + configuration::SUSTAIN_SUFFIX)->load();
				stageValues[(int)Stage::RELEASE] = apvts.getRawParameterValue(prefix + configuration::RELEASE_SUFFIX)->load();
			}

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::ATTACK_SUFFIX,
					prefix + configuration::ATTACK_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.25f),
					0.5f));
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::DECAY_SUFFIX,
					prefix + configuration::DECAY_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.25f),
					0.2f));
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::SUSTAIN_SUFFIX,
					prefix + configuration::SUSTAIN_SUFFIX,
					juce::NormalisableRange<float>(minLevel, 1.0f, 0.001f, 1.f),
					0.8f));
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::RELEASE_SUFFIX,
					prefix + configuration::RELEASE_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.25f),
					0.4f));
			}

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		Envelope() = delete;

		Envelope(SharedData* t_data) : data(t_data) {}


		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			data->sampleRate = spec.sampleRate;
		};

		virtual void reset() override;

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

		virtual void noteOn() override;

		virtual void noteOff() override;

		bool isIdle() { return stage == Stage::IDLE; }

	private:

		void transition();

		//static inline float getMultiplier(float startLevel, float targetLevel, size_t samples) {
		//	jassert(targetLevel > 0);
		//	return std::powf(targetLevel / startLevel, 1.f / samples);
		//}

		inline void advanceStage() {
			stage = static_cast<Stage> (((int)stage + 1) % (int)Stage::STAGE_NUMBER);
		}

		SharedData* data;
		Stage stage = Stage::IDLE;
		float level = 0.f;
		float summand = 0.f;
		size_t samplesUntilTransition = 0;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Envelope);
	};
}