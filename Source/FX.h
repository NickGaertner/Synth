
#pragma once

#include <JuceHeader.h>
#include "DSP.h"

namespace customDsp {

	enum class FXType {
		NONE,
		REVERB,
		FLANGER,
		CHORUS,
		PHASER,
		DELAY,
		TUBE,
		NUMBER_OF_TYPES,
	};

	inline static const juce::StringArray FX_TYPE_NAMES{
		"None",
		"Reverb",
		"Flanger",
		"Chorus",
		"Phaser",
		"Delay",
		"Tube",
	};

	inline static const juce::StringArray PARAMETER_0_NAMES{
		"-",
		"Size",
		"Rate",
		"Rate",
		"Rate",
		"Damp",
		"Tube",
	};

	inline static const juce::StringArray PARAMETER_1_NAMES{
		"-",
		"Damping",
		"Depth",
		"Depth",
		"Depth",
		"Time Left",
		"Tube",
	};

	inline static const juce::StringArray PARAMETER_2_NAMES{
		"-",
		"Width",
		"Feedback",
		"Centre Delay",
		"Feedback",
		"Time Right",
		"Tube",
	};

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

			virtual FXChooser* createProcessor() override {
				return new FXChooser(this);
			}

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {

				layout.add(std::make_unique<juce::AudioParameterBool>(
					prefix + configuration::BYPASSED_SUFFIX,
					prefix + configuration::BYPASSED_SUFFIX,
					bypassed));

				layout.add(std::make_unique<juce::AudioParameterChoice>(
					prefix + configuration::FX_TYPE_SUFFIX,
					prefix + configuration::FX_TYPE_SUFFIX,
					FX_TYPE_NAMES,
					(int)fxType));

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::DRY_WET_SUFFIX,
					prefix + configuration::DRY_WET_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f, 1.f),
					dryWet));
				modParams[DRY_WET].addModParams(layout, prefix + configuration::DRY_WET_SUFFIX);

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::PARAMETER_0_SUFFIX,
					prefix + configuration::PARAMETER_0_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f, 1.f),
					parameter0));
				modParams[PARAMETER_0].addModParams(layout, prefix + configuration::PARAMETER_0_SUFFIX);

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::PARAMETER_1_SUFFIX,
					prefix + configuration::PARAMETER_1_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f, 1.f),
					parameter1));
				modParams[PARAMETER_1].addModParams(layout, prefix + configuration::PARAMETER_1_SUFFIX);

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::PARAMETER_2_SUFFIX,
					prefix + configuration::PARAMETER_2_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f, 1.f),
					parameter2));
				modParams[PARAMETER_2].addModParams(layout, prefix + configuration::PARAMETER_2_SUFFIX);
			}

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override {
				apvts.addParameterListener(prefix + configuration::BYPASSED_SUFFIX, this);
				apvts.addParameterListener(prefix + configuration::FX_TYPE_SUFFIX, this);
				apvts.addParameterListener(prefix + configuration::DRY_WET_SUFFIX, this);
				modParams[DRY_WET].registerAsListener(apvts, prefix + configuration::DRY_WET_SUFFIX);
				apvts.addParameterListener(prefix + configuration::PARAMETER_0_SUFFIX, this);
				modParams[PARAMETER_0].registerAsListener(apvts, prefix + configuration::PARAMETER_0_SUFFIX);
				apvts.addParameterListener(prefix + configuration::PARAMETER_1_SUFFIX, this);
				modParams[PARAMETER_1].registerAsListener(apvts, prefix + configuration::PARAMETER_1_SUFFIX);
				apvts.addParameterListener(prefix + configuration::PARAMETER_2_SUFFIX, this);
				modParams[PARAMETER_2].registerAsListener(apvts, prefix + configuration::PARAMETER_2_SUFFIX);
			}

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override {
				if (parameterID.endsWith(prefix + configuration::BYPASSED_SUFFIX)) {
					bypassed = static_cast<bool>(newValue);
				}
				else if (parameterID.endsWith(prefix + configuration::FX_TYPE_SUFFIX)) {
					fxType = static_cast<FXType>(newValue);
				}
				else if (parameterID.endsWith(prefix + configuration::DRY_WET_SUFFIX)) {
					dryWet = newValue;
				}
				else if (parameterID.endsWith(prefix + configuration::PARAMETER_0_SUFFIX)) {
					parameter0 = newValue;
				}
				else if (parameterID.endsWith(prefix + configuration::PARAMETER_1_SUFFIX)) {
					parameter1 = newValue;
				}
				else if (parameterID.endsWith(prefix + configuration::PARAMETER_2_SUFFIX)) {
					parameter2 = newValue;
				}
			}

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		FXChooser() = delete;
		FXChooser(SharedData* t_data) : data(t_data) {}

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
		virtual ~FX() {}

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
		void setMaxDelay(int maxDelay) {
			buffer.allocate(maxDelay, true);
			bufferSize = maxDelay;
			index = 0;
		}
		void reset() {
			buffer.clear(bufferSize);
		}
		float process(float sample, float delay, float feedback) {
			jassert(1.f <= delay && delay <= bufferSize);
			jassert(-1.f < feedback && feedback < 1.f);
			auto output = buffer[index];
			buffer[index] = 0;
			auto input = /*(1.f - std::abs(feedback)) **/ sample + feedback * output;
			int delayOffset = static_cast<int>(delay);
			float delayFrac = delay - delayOffset;
			jassert(0.f <= delayFrac && delayFrac < 1.f);
			buffer[(index + delayOffset) % bufferSize] += (1.f - delayFrac) * input;
			buffer[(index + delayOffset + 1) % bufferSize] += delayFrac * input;
			index = (index + 1) % bufferSize;
			return output;
		}
	private:
		juce::HeapBlock<float> buffer;
		int bufferSize = -1;
		int index = -1;
	};

	class Delay : public FX {
		using FX::FX;
	public:
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
		static constexpr float maxDelaySec = 1.f;
	};

	class Flanger : public FX {
		using FX::FX;
	public:
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

		static constexpr float minDelaySamples = 2.f;
		static constexpr float maxDelayMSec = 20.f;
	};

	class Chorus : public FX {
		using FX::FX;
	public:
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

		static constexpr float minDelayMSec = 7.f;
		static constexpr float maxDelayMSec = 30.f;
	};

	class Phaser : public FX {
		using FX::FX;
	public:
		virtual void prepareUpdate() override;
		virtual void reset() override;
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;
	private:
		enum {
			LEFT,
			RIGHT
		};
		static constexpr int stages = 6;
		float minCutoffs[stages]{ 16.f,33.f,48.f,98.f,160.f,260.f, };
		float maxCutoffs[stages]{ 1600.f,3300.f,4800.f,9800.f,16000.f,22000.f, };
		
		float last[2]{ 0,0 };
		LFO::SharedData lfoData{ "ERROR" };
		LFO lfos[2]{ &lfoData,&lfoData };
		float allpassS1[2][stages]{ 0 };

		// FirstOrderTPTFilter from JUCE/Vadim Zavalishin
		class AllpassFilter {
		public:
			float processSample(float input, float cutoff, float sampleRate) {
				auto g = juce::dsp::FastMathApproximations::tan<float>(juce::MathConstants<float>::pi * cutoff / sampleRate);
				auto G = g / (1 + g);
				auto v = G * (input - s1);
				auto y = v + s1;
				s1 = y + v;
				return 2 * y - input;
			}

		private:
			float s1 = 0;

		};
	};


}