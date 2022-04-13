
#pragma once
#include <JuceHeader.h>
#include "DSP.h"

namespace customDsp {

	enum class FilterType {
		NONE,
		TPT_LPF12,
		TPT_BPF12,
		TPT_HPF12,
	};

	inline static const juce::StringArray FILTER_TYPE_NAMES{
		"None",
		"TPT_LPF12",
		"TPT_BPF12",
		"TPT_HPF12",
	};

	void replaceIdWithFilterName(juce::XmlElement& xml);

	void replaceFilterNameWithId(juce::XmlElement& xml);

	class Filter;
	class LadderFilter;

	class FilterChooser : public Processor {
	public:

		inline static bool isSameFilter(FilterType a, FilterType b) {
			// could be more efficient (e.g. with bit masks) but that makes the AudioParameterChoice in the apvts awkward
			if (a != FilterType::NONE && b != FilterType::NONE) {
				return true;
			}
			else if (a == FilterType::NONE && b == FilterType::NONE) {
				return true;
			}
			return false;
		}

		struct SharedData : public Processor::SharedData {
			using Processor::SharedData::SharedData;

			bool bypassed{ false };
			FilterType filterType{ FilterType::NONE };
			// these use the range [0,1]. it's up to the actual filter to map these to an useful range
			float cutoff{ 0.5f }, resonance{ 0.5f }, special{ 0.0f };
			ModulationParam modParams[3];
			enum {
				CUTOFF,
				RES,
				SPECIAL,
			};

			int numChannels = -1;

			virtual FilterChooser* createProcessor() override;

			virtual void addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override;

			virtual void parameterChanged(const juce::String& parameterID, float newValue) override;
		private:

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		FilterChooser() = delete;

		FilterChooser(SharedData* t_data) : data(t_data) {}
		virtual ~FilterChooser() override {};

		void createFilter(FilterType type);

		virtual void prepare(const juce::dsp::ProcessSpec& spec) override;

		virtual void reset() override;

		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

		virtual void noteOn() override;

		virtual void noteOff() override;

	private:
		SharedData* data;
		std::unique_ptr<Filter> filter;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterChooser)
	};
	class Filter : public Processor {

	public:
		Filter() = delete;
		Filter(FilterChooser::SharedData* t_data) : data(t_data) {}
		virtual ~Filter() override {}

		virtual void updateMode() = 0;
		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
			juce::ignoreUnused(spec);
			jassertfalse;
		}
		virtual void prepareUpdate() = 0;
		FilterType mode{ FilterType::NONE };

	protected:
		FilterChooser::SharedData* data;
	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Filter)
	};

	class DummyFilter : public Filter {
		using Filter::Filter;
	public:
		virtual ~DummyFilter() override {};

		virtual void reset() override {}
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override {
			juce::ignoreUnused(context, workBuffers);
			return false;
		}
		virtual void prepareUpdate() {}
	private:
		void updateMode() override {}
	};

	// This is basically the StateVariableTPTFilter from the JUCE library
	// credits to the JUCE team and Vadim Zavalishin
	class TPTFilter : public Filter {
		using Filter::Filter;
	public:
		virtual ~TPTFilter() override {};

		virtual void prepareUpdate() override;
		virtual void reset() override;
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:
		void updateMode() override;

		std::vector<float>s1{ 2 }, s2{ 2 };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TPTFilter)
	};
}