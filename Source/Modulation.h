
#pragma once
#include "DSP.h"

namespace customDsp {

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

			float stageValues[(int)Stage::STAGE_NUMBER]{ 0.f,0.1f,0.3f,0.9f,0.05f };
			const float minLevel = juce::Decibels::decibelsToGain(-96.f);


			virtual Envelope* createProcessor() override;

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override;

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override;
		private:

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		Envelope() = delete;

		Envelope(SharedData* t_data) : data(t_data) {}
		virtual ~Envelope() override {};


		virtual void prepare(const juce::dsp::ProcessSpec& spec) override;

		virtual void reset() override;

		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

		virtual void noteOn() override;

		virtual void noteOff() override;

		bool isIdle();

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


	class LFO : public Processor {
	public:

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			float wtPos{ 0.f }, rate{ 1.f };
			wavetable::Wavetable::Ptr wt;

			virtual LFO* createProcessor() override;

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override;

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override;

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		LFO() = delete;

		LFO(SharedData* t_data) : data(t_data) {}
		virtual ~LFO() override {};

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override;

		virtual void reset() override;

		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

		void advancePhase(float step);

	private:
		SharedData* data;

		juce::dsp::Phase<float> phase;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFO);
	};
}