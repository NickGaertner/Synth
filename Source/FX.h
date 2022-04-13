
#pragma once

#include <JuceHeader.h>
#include "DSP.h"
#include "Modulation.h"

namespace customDsp {

	enum class FXType {
		NONE,
		REVERB,
		FLANGER,
		CHORUS,
		PHASER,
		DELAY,
		DISTORTION,
		NUMBER_OF_TYPES,
	};

	inline static const juce::StringArray FX_TYPE_NAMES{
		"None",
		"Reverb",
		"Flanger",
		"Chorus",
		"Phaser",
		"Delay",
		"Distortion",
	};

	inline static const juce::StringArray PARAMETER_0_NAMES{
		"-",
		"Size",
		"Rate",
		"Rate",
		"Rate",
		"Damp",
		"Freq Split",
	};

	inline static const juce::StringArray PARAMETER_1_NAMES{
		"-",
		"Damping",
		"Depth",
		"Depth",
		"Depth",
		"Time Left",
		"Freq Emphasis",
	};

	inline static const juce::StringArray PARAMETER_2_NAMES{
		"-",
		"Width",
		"Feedback",
		"Centre Delay",
		"Feedback",
		"Time Right",
		"Distortion",
	};


	void replaceIdWithFXName(juce::XmlElement& xml);

	void replaceFXNameWithId(juce::XmlElement& xml);

	class FX;

	class FXChooser : public Processor {
	public:

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			bool bypassed{ false };
			FXType fxType{ FXType::NONE };
			// these use the range [0,1]. it's up to the actual fx to map these to an useful range
			float dryWet{ 0.0f }, parameter0{ 0.f }, parameter1{ 0.f }, parameter2{ 0.f };
			ModulationParam modParams[4];
			enum {
				DRY_WET,
				PARAMETER_0,
				PARAMETER_1,
				PARAMETER_2,
			};

			int numChannels = -1;

			virtual FXChooser* createProcessor() override;

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override;

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override;

		private:

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		FXChooser() = delete;
		FXChooser(SharedData* t_data) : data(t_data) {}
		virtual ~FXChooser() override {};

		void createFX(FXType type);

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override;

		virtual void reset() override;

		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

		virtual void noteOn() override;

		virtual void noteOff() override;
	private:
		SharedData* data;
		std::unique_ptr<FX> fx;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXChooser)
	};

	class FX : public Processor {
	public:
		FX() = delete;
		FX(FXChooser::SharedData* t_data) : data(t_data) { mode = data->fxType; };
		virtual ~FX() override {}

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			juce::ignoreUnused(spec);
			jassertfalse;
		}

		virtual void prepareUpdate() = 0;

		FXType mode{ FXType::NONE };
	protected:
		FXChooser::SharedData* data;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FX)
	};

	class DummyFX : public FX {
		using FX::FX;
	public:
		virtual ~DummyFX() override {};

		virtual void reset() override {}
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override {
			juce::ignoreUnused(context, workBuffers);
			return false;
		}
		virtual void prepareUpdate() {}

	};

	// This is basically the Reverb from the JUCE library
	// credits to the JUCE team
	class Reverb : public FX {
		using FX::FX;
	public:
		virtual ~Reverb() override {};

		virtual void prepareUpdate() override;
		virtual void reset() override;
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:

		struct CombFilterData {
			int startPos{ -1 };
			int index{ 0 };
			int size{ 0 };
			float last{ 0.f };

			void update(const int t_startPos, const int t_size) {
				startPos = t_startPos;
				index = 0;
				size = t_size;
				last = 0.f;
			}
		};
		struct AllPassData {
			int startPos{ -1 };
			int index{ 0 };
			int size{ 0 };
			void update(const int t_startPos, const int t_size) {
				startPos = t_startPos;
				index = 0;
				size = t_size;
			}
		};;

		enum { numCombs = 4, numAllPasses = 2, numChannels = 2 }; // numCombs=8, numAllPasses = 4 before, but it was kinda slow
		enum {
			LEFT,
			RIGHT,
		};

		CombFilterData combData[numChannels][numCombs];
		juce::HeapBlock<float> combBuffer[2];
		size_t leftCombBufferSize = 0;
		size_t rightCombBufferSize = 0;

		AllPassData allPassData[numChannels][numAllPasses];
		juce::HeapBlock<float> allPassBuffer[2];
		size_t leftAllPassBufferSize = 0;
		size_t rightAllPassBufferSize = 0;

		//**********
		inline static float MIN_LEVEL = juce::Decibels::decibelsToGain(-96.f);

	};

	class DelayLine {
	public:
		void setMaxDelay(int maxDelay);
		
		void reset();
		
		float process(float sample, float delay, float feedback);

		bool isEmpty();

	private:
		juce::HeapBlock<float> buffer;
		int bufferSize = -1;
		int index = -1;
	};

	class Delay : public FX {
		using FX::FX;
	public:
		virtual ~Delay() override {};

		virtual void prepareUpdate() override;
		virtual void reset() override;
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:
		enum {
			LEFT,
			RIGHT
		};
		juce::SmoothedValue<float> delaySmoothed[2];
		DelayLine delays[2]{};
		static constexpr float MAX_DELAY_SEC = 1.f;
	};

	class Flanger : public FX {
		using FX::FX;
	public:
		virtual ~Flanger() override {};

		virtual void prepareUpdate() override;
		virtual void reset() override;
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:
		enum {
			LEFT,
			RIGHT
		};
		DelayLine delays[2];
		LFO::SharedData lfoData{ "ERROR" };
		LFO lfos[2]{ &lfoData,&lfoData };
		juce::SmoothedValue<float> delaySmoothed[2];
		juce::SmoothedValue<float> feedbackSmoothed[2];

		static constexpr float MIN_DELAY_SAMPLES = 2.f;
		static constexpr float MAX_DELAY_MSEC = 20.f;
	};

	class Chorus : public FX {
		using FX::FX;
	public:
		virtual ~Chorus() override {};

		virtual void prepareUpdate() override;
		virtual void reset() override;
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:
		enum {
			LEFT,
			RIGHT
		};
		DelayLine delays[2];
		LFO::SharedData lfoData{ "ERROR" };
		LFO lfos[2]{ &lfoData,&lfoData };
		juce::SmoothedValue<float> delaySmoothed[2];
		juce::SmoothedValue<float> centreDelaySmoothed[2];

		static constexpr float MIN_DELAY_MSEC = 7.f;
		static constexpr float MAX_DELAY_MSEC = 30.f;
	};

	class Phaser : public FX {
		using FX::FX;
	public:
		virtual ~Phaser() override {};

		virtual void prepareUpdate() override;
		virtual void reset() override;
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;
	private:
		enum {
			LEFT,
			RIGHT
		};
		static constexpr int STAGES = 6;
		float minCutoffs[STAGES]{ 16.f,33.f,48.f,98.f,160.f,260.f, };
		float maxCutoffs[STAGES]{ 1600.f,3300.f,4800.f,9800.f,16000.f,22000.f, };

		float last[2]{ 0,0 };
		LFO::SharedData lfoData{ "ERROR" };
		LFO lfos[2]{ &lfoData,&lfoData };
		float allpassS1[2][STAGES]{ 0 };

	};

	class Distortion : public FX {
		using FX::FX;
	public:
		virtual ~Distortion() override {};

		virtual void reset() override;
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;
		virtual void prepareUpdate();
	private:
		float filterS1[2]{ 0 };
		float last[2]{ 0,0 };
	};
}