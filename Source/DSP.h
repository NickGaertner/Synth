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


	struct ModulationParam : public juce::AudioProcessorValueTreeState::Listener {

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

		void addModParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& name, float modRange = 1.f) {
			layout.add(std::make_unique<juce::AudioParameterFloat>(
				name + configuration::MOD_FACTOR_SUFFIX,
				name + configuration::MOD_FACTOR_SUFFIX,
				juce::NormalisableRange<float>(-1.f * modRange, 1.0f * modRange, 0.001f, 1.f),
				factor));
			layout.add(std::make_unique<juce::AudioParameterChoice>(
				name + configuration::MOD_CHANNEL_SUFFIX,
				name + configuration::MOD_CHANNEL_SUFFIX,
				configuration::getModChannelNames(),
				src_channel));
		}

		//void updateModParams(const juce::AudioProcessorValueTreeState& apvts, const juce::String& name) {
		//	factor = apvts.getRawParameterValue(name + configuration::MOD_FACTOR_SUFFIX)->load();
		//	src_channel = (int)apvts.getRawParameterValue(name + configuration::MOD_CHANNEL_SUFFIX)->load();
		//}

		void registerAsListener(juce::AudioProcessorValueTreeState& apvts, const juce::String& name) {
			apvts.addParameterListener(name + configuration::MOD_CHANNEL_SUFFIX, this);
			apvts.addParameterListener(name + configuration::MOD_FACTOR_SUFFIX, this);
		}

		virtual void parameterChanged(const juce::String& parameterID, float newValue) override {
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

	};


	class Processor {
	public:

		struct SharedData : public juce::AudioProcessorValueTreeState::Listener {
			double sampleRate{ 0.f };
			juce::String prefix;

			SharedData() = delete;
			SharedData(const juce::String& t_prefix) : prefix(t_prefix) {};
			virtual ~SharedData() {};
			//virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) = 0;
			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) = 0;
			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) = 0;
			virtual Processor* createProcessor() = 0;

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};
		Processor() {}

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

			//virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) override {
			//	juce::ignoreUnused(apvts);
			//}

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {
				juce::ignoreUnused(layout);
			}

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override {
				juce::ignoreUnused(apvts);
			}

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override {
				juce::ignoreUnused(parameterID, newValue);
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

			bool bypassed{ false };
			float wtPos{ 0.f }, pitch{ 0.f };
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

			//virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) override {
			//	bypassed = apvts.getRawParameterValue(prefix + configuration::BYPASSED_SUFFIX)->load();

			//	modParams[ENV].updateModParams(apvts, prefix + configuration::ENV_SUFFIX);

			//	wtPos = apvts.getRawParameterValue(prefix + configuration::WT_POS_SUFFIX)->load();
			//	modParams[WT_POS].updateModParams(apvts, prefix + configuration::WT_POS_SUFFIX);

			//	pitch = apvts.getRawParameterValue(prefix + configuration::PITCH_SUFFIX)->load();
			//	modParams[PITCH].updateModParams(apvts, prefix + configuration::PITCH_SUFFIX);

			//	wt = wavetable::WavetableManager::getWavetable(
			//		static_cast<int>(apvts.getRawParameterValue(prefix + configuration::WT_SUFFIX)->load()), sampleRate);
			//}

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {

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
				modParams[PITCH].addModParams(layout, prefix + configuration::PITCH_SUFFIX, 24.f);

				layout.add(std::make_unique<juce::AudioParameterChoice>(
					prefix + configuration::WT_SUFFIX,
					prefix + configuration::WT_SUFFIX,
					wavetable::WavetableManager::getWavetableNames(),
					0));
			}

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override {
				apvts.addParameterListener(prefix + configuration::BYPASSED_SUFFIX, this);
				modParams[ENV].registerAsListener(apvts, prefix + configuration::ENV_SUFFIX);
				apvts.addParameterListener(prefix + configuration::WT_POS_SUFFIX, this);
				modParams[WT_POS].registerAsListener(apvts, prefix + configuration::WT_POS_SUFFIX);
				apvts.addParameterListener(prefix + configuration::PITCH_SUFFIX, this);
				modParams[PITCH].registerAsListener(apvts, prefix + configuration::PITCH_SUFFIX);
				apvts.addParameterListener(prefix + configuration::WT_SUFFIX, this);
			}

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override {
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
					wt = wavetable::WavetableManager::getWavetable(static_cast<int>(newValue), sampleRate);
				}
				else {
					jassertfalse;
				}
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

		void setFrequency(float t_frequency, bool force = true) {
			juce::ignoreUnused(force);
			frequency = t_frequency;
		}

		void setVelocity(float t_velocity)
		{
			velocity = t_velocity;
		}

		int getEnvChannel() {
			return data->modParams[SharedData::ENV].src_channel;
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

			float wtPos{ 0.f }, rate{ 1.f };
			const wavetable::Wavetable* wt;

			virtual LFO* createProcessor() override {
				return new LFO(this);
			};

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {

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
					wavetable::WavetableManager::getWavetableNames(),
					0));
			}

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override {
				apvts.addParameterListener(prefix + configuration::WT_POS_SUFFIX, this);
				apvts.addParameterListener(prefix + configuration::RATE_SUFFIX, this);
				apvts.addParameterListener(prefix + configuration::WT_SUFFIX, this);
			}

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override {
				if (parameterID.endsWith(configuration::WT_POS_SUFFIX)) {
					wtPos = newValue;
				}
				else if (parameterID.endsWith(configuration::RATE_SUFFIX)) {
					rate = newValue;
				}
				else if (parameterID.endsWith(configuration::WT_SUFFIX)) {
					wt = wavetable::WavetableManager::getWavetable(static_cast<int>(newValue), sampleRate);
				}
				else {
					jassertfalse;
				}
			}

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		LFO() = delete;

		LFO(SharedData* t_data) : data(t_data) {}

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			data->sampleRate = spec.sampleRate;
		};

		virtual void reset() override {
			phase.reset();
		}

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:
		SharedData* data;

		juce::dsp::Phase<float> phase;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFO);
	};

	class Gain : public Processor {
	public:

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			float gain{ 0.5f };
			ModulationParam modParams[1];
			enum {
				GAIN
			};

			virtual Gain* createProcessor() override {
				return new Gain(this);
			};

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::GAIN_SUFFIX,
					prefix + configuration::GAIN_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.f),
					gain));
				modParams[GAIN].addModParams(layout, prefix + configuration::GAIN_SUFFIX);
			}

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override {
				apvts.addParameterListener(prefix + configuration::GAIN_SUFFIX, this);
				modParams[GAIN].registerAsListener(apvts, prefix + configuration::GAIN_SUFFIX);
			}

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override {
				if (parameterID.endsWith(configuration::GAIN_SUFFIX)) {
					gain = newValue;
				}
				else {
					jassertfalse;
				}
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

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:
		SharedData* data;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Gain);
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

			float stageValues[(int)Stage::STAGE_NUMBER]{ 0.f,0.2f,0.3f,0.9f,0.2f };
			const float minLevel = juce::Decibels::decibelsToGain(-96.f);


			virtual Envelope* createProcessor() override {
				return new Envelope(this);
			};

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::ATTACK_SUFFIX,
					prefix + configuration::ATTACK_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.25f),
					stageValues[(int)Stage::ATTACK]));
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::DECAY_SUFFIX,
					prefix + configuration::DECAY_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.25f),
					stageValues[(int)Stage::DECAY]));
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::SUSTAIN_SUFFIX,
					prefix + configuration::SUSTAIN_SUFFIX,
					juce::NormalisableRange<float>(minLevel, 1.0f, 0.001f, 1.f),
					stageValues[(int)Stage::SUSTAIN]));
				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::RELEASE_SUFFIX,
					prefix + configuration::RELEASE_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.25f),
					stageValues[(int)Stage::RELEASE]));
			}

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override {
				apvts.addParameterListener(prefix + configuration::ATTACK_SUFFIX, this);
				apvts.addParameterListener(prefix + configuration::DECAY_SUFFIX, this);
				apvts.addParameterListener(prefix + configuration::SUSTAIN_SUFFIX, this);
				apvts.addParameterListener(prefix + configuration::RELEASE_SUFFIX, this);
			}

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override {
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