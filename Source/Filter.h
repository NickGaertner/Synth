
#pragma once
#include <JuceHeader.h>
#include "DSP.h"

namespace customDsp {

	enum class FilterType {
		NONE,
		LADDER_LPF12,
		LADDER_LPF24,
		LADDER_HPF12,
		LADDER_HPF24,
		LADDER_BPF12,
		LADDER_BPF24,

	};

	inline static const juce::StringArray FILTER_TYPE_NAMES{
			"NONE",
			"Ladder LPF12",
			"Ladder LPF24",
			"Ladder HPF12",
			"Ladder HPF24",
			"Ladder BPF12",
			"Ladder BPF24",
	};

	class Filter;
	class LadderFilter;

	class FilterChooser : public Processor {
	public:

		inline static bool isSameFilter(FilterType a, FilterType b) {
			// could be more efficient (e.g. with bit masks) but that makes the AudioParameterChoice in the apvts awkward
			if (a != FilterType::NONE && b != FilterType::NONE) {
				return true;
			}
			else if(a == FilterType::NONE && b == FilterType::NONE) {
				return true;
			}
			return false;
		}

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			bool bypassed{ false };
			FilterType filterType{ FilterType::LADDER_LPF12 };
			// these use the range [0,1]. it's up to the actual filter to map these to an useful range
			float cutoff{ 0.5 }, resonance{ 0.5 }, drive{ 0.0 };

			ModulationParam modParams[3];
			enum {
				CUTOFF,
				RES,
				DRIVE,
			};

			virtual FilterChooser* createProcessor() override {
				return new FilterChooser(this);
			};

			//virtual void updateParams(const juce::AudioProcessorValueTreeState& apvts) override {
			//	bypassed = apvts.getRawParameterValue(prefix + configuration::BYPASSED_SUFFIX)->load();

			//	filterType = static_cast<FilterType>(apvts.getRawParameterValue(prefix + configuration::FILTER_TYPE_SUFFIX)->load());

			//	cutoff = apvts.getRawParameterValue(prefix + configuration::CUTOFF_SUFFIX)->load();
			//	modParams[CUTOFF].updateModParams(apvts, prefix + configuration::CUTOFF_SUFFIX);

			//	resonance = apvts.getRawParameterValue(prefix + configuration::RESONANCE_SUFFIX)->load();
			//	modParams[RES].updateModParams(apvts, prefix + configuration::RESONANCE_SUFFIX);

			//	drive = apvts.getRawParameterValue(prefix + configuration::DRIVE_SUFFIX)->load();
			//	modParams[DRIVE].updateModParams(apvts, prefix + configuration::DRIVE_SUFFIX);
			//}

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override {

				layout.add(std::make_unique<juce::AudioParameterBool>(
					prefix + configuration::BYPASSED_SUFFIX,
					prefix + configuration::BYPASSED_SUFFIX,
					bypassed));

				layout.add(std::make_unique<juce::AudioParameterChoice>(
					prefix + configuration::FILTER_TYPE_SUFFIX,
					prefix + configuration::FILTER_TYPE_SUFFIX,
					FILTER_TYPE_NAMES,
					(int)filterType));

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::CUTOFF_SUFFIX,
					prefix + configuration::CUTOFF_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.f),
					cutoff));
				modParams[CUTOFF].addModParams(layout, prefix + configuration::CUTOFF_SUFFIX);

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::RESONANCE_SUFFIX,
					prefix + configuration::RESONANCE_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.f),
					resonance));
				modParams[RES].addModParams(layout, prefix + configuration::RESONANCE_SUFFIX);

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::DRIVE_SUFFIX,
					prefix + configuration::DRIVE_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.f),
					drive));
				modParams[DRIVE].addModParams(layout, prefix + configuration::DRIVE_SUFFIX);
			}

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override {
				apvts.addParameterListener(prefix + configuration::BYPASSED_SUFFIX, this);
				apvts.addParameterListener(prefix + configuration::FILTER_TYPE_SUFFIX, this);
				apvts.addParameterListener(prefix + configuration::CUTOFF_SUFFIX, this);
				modParams[CUTOFF].registerAsListener(apvts, prefix + configuration::CUTOFF_SUFFIX);
				apvts.addParameterListener(prefix + configuration::RESONANCE_SUFFIX, this);
				modParams[RES].registerAsListener(apvts, prefix + configuration::RESONANCE_SUFFIX);
				apvts.addParameterListener(prefix + configuration::DRIVE_SUFFIX, this);
				modParams[DRIVE].registerAsListener(apvts, prefix + configuration::DRIVE_SUFFIX);

			}

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override {
				if (parameterID.endsWith(prefix + configuration::BYPASSED_SUFFIX)) {
					bypassed = (bool)newValue;
				}
				else if (parameterID.endsWith(prefix + configuration::FILTER_TYPE_SUFFIX)) {
					filterType = static_cast<FilterType>(newValue);
				}
				else if (parameterID.endsWith(prefix + configuration::CUTOFF_SUFFIX)) {
					cutoff = newValue;
				}
				else if (parameterID.endsWith(prefix + configuration::RESONANCE_SUFFIX)) {
					resonance = newValue;
				}
				else if (parameterID.endsWith(prefix + configuration::DRIVE_SUFFIX)) {
					drive = newValue;
				}
			}
		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		FilterChooser() = delete;

		FilterChooser(SharedData* t_data) : data(t_data) {
			createFilter(data->filterType);
		}

		void createFilter(FilterType type);

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override;

		virtual void reset() override;

		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:
		SharedData* data;
		std::unique_ptr<Filter> filter;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterChooser)
	};
	class Filter : public Processor {

	public:
		Filter() = delete;
		Filter(FilterChooser::SharedData* t_data) : data(t_data) {};
		virtual ~Filter() {};

		virtual void updateMode() = 0;

		FilterType mode{ FilterType::NONE };


	protected:
		FilterChooser::SharedData* data;
	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Filter)
	};

	class DummyFilter : public Filter {
		using Filter::Filter;
	public:
		virtual void prepare(const juce::dsp::ProcessSpec& spec) override{}
		virtual void reset() override{}
		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override{}

	private:
		void updateMode() override{}
	};

	// This is basically the juce::LadderFilter without smoothers
	// credits to the JUCE team!
	class LadderFilter : public Filter {
		using Filter::Filter;
	public:
		virtual void prepare(const juce::dsp::ProcessSpec& spec) override;
		virtual void reset() override;
		virtual void process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:
		void updateMode() override;
		void setDrive(float newDrive);

		// TODO get rid of variables that could be local in our case
		float drive, drive2, gain, gain2, comp;
		std::vector<std::array<float, 5>> state{ 2 };
		std::array<float, 5> A;

		float cutoffFreqHz{ 4000.f };
		float resonance{ 0.f };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LadderFilter)
	};

}