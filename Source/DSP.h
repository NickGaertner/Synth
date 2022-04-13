#pragma once

#include <JuceHeader.h>
#include "Configuration.h"
#include "Wavetable.h"

namespace customDsp {

	// number of channels to append to the outputBlock which can be freely used by processors to store
	// temporary data
	constexpr int WORK_BUFFERS = 2;

	struct ModulationParam : public juce::AudioProcessorValueTreeState::Listener {

		float factor = 0.f;
		int src_channel = configuration::EMPTY_MOD_CHANNEL;

		bool isActive();

		bool isLfo();

		bool isEnv();

		void addModParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& name,
			float modRange = 1.f, float intervalValue = 0.001f);

		void registerAsListener(juce::AudioProcessorValueTreeState& apvts, const juce::String& name);

		virtual void parameterChanged(const juce::String& parameterID, float newValue) override;
	};


	class Processor {
	public:

		struct SharedData : public juce::AudioProcessorValueTreeState::Listener {
			double sampleRate{ 0.f };
			juce::String prefix;

			SharedData() = delete;
			SharedData(const juce::String& t_prefix) : prefix(t_prefix) {};
			virtual ~SharedData() {};
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

		// returns wether the processor still needs more time
		// this is useful for effects like reverbs
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>&, juce::dsp::AudioBlock<float>&) = 0;

		virtual void noteOn();

		virtual void noteOff();

	protected:
		bool isNoteOn = false;

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
		virtual ~DummyProcessor() override {};

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			juce::ignoreUnused(spec);
		}

		virtual void reset() override {}

		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override {
			juce::ignoreUnused(context, workBuffers);
			return false;
		}

	private:
		SharedData* data;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DummyProcessor);
	};


	class ProcessorChain : public Processor {
		using Processor::Processor;
	public:
		virtual ~ProcessorChain() override {};

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override;

		virtual void reset() override;

		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

		virtual void noteOn() override;

		virtual void noteOff() override;

		virtual void addProcessor(Processor* processor);

		virtual Processor* getProcessor(int index);

		int size();

	protected:
		juce::OwnedArray<Processor> processors;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorChain)
	};


	class SplitProcessor : public ProcessorChain {
		using ProcessorChain::ProcessorChain;
	public:
		virtual ~SplitProcessor() override {};

		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitProcessor)
	};

	class InterpolationOsc : public Processor {
	public:

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			bool bypassed{ false };
			float wtPos{ 0.f }, pitch{ 0.f };
			wavetable::Wavetable::Ptr wt;
			ModulationParam modParams[3];
			enum {
				ENV,
				WT_POS,
				PITCH,
			};

			virtual InterpolationOsc* createProcessor() override;

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override;

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override;
		private:

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		InterpolationOsc() = delete;

		InterpolationOsc(SharedData* t_data) : data(t_data) {}
		virtual ~InterpolationOsc() override {};

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override;

		virtual void reset() override;

		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

		void setFrequency(float t_frequency, bool force = true);

		void setVelocity(float t_velocity);

		int getEnvChannel();

		virtual void noteOff();

	private:
		SharedData* data;

		float frequency = 440.f;
		float velocity = 0.f;
		juce::dsp::Phase<float> phase;
		bool shouldStopCleanly = false;
		static constexpr int MAX_RELEASE_SAMPLES = 500;
		int currentReleaseSamples = 0;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InterpolationOsc);
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

			virtual Gain* createProcessor() override;

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override;

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override;

		private:

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		Gain() = delete;

		Gain(SharedData* t_data) : data(t_data) {}
		virtual ~Gain() override {};


		virtual void prepare(const juce::dsp::ProcessSpec& spec) override;

		virtual void reset() override;

		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:
		SharedData* data;
		juce::SmoothedValue<float> gainSmoothed;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Gain);
	};


	class Pan : public Processor {
	public:

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			float pan{ 0.f };
			ModulationParam modParams[1];
			enum {
				PAN
			};

			virtual Pan* createProcessor() override;

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override;

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override;

		private:

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		Pan() = delete;

		Pan(SharedData* t_data) : data(t_data) {}
		virtual ~Pan() override {};

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override;

		virtual void reset() override {}

		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:
		SharedData* data;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Pan);
	};
}