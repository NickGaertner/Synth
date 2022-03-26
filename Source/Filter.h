
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
		"NONE",
		"TPT_LPF12",
		"TPT_BPF12",
		"TPT_HPF12",
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

			virtual FilterChooser* createProcessor() override {
				return new FilterChooser(this);
			}

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
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.00001f, 0.3f),
					cutoff));
				modParams[CUTOFF].addModParams(layout, prefix + configuration::CUTOFF_SUFFIX);

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::RESONANCE_SUFFIX,
					prefix + configuration::RESONANCE_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f, 1.f),
					resonance));
				modParams[RES].addModParams(layout, prefix + configuration::RESONANCE_SUFFIX);

				layout.add(std::make_unique<juce::AudioParameterFloat>(
					prefix + configuration::SPECIAL_SUFFIX,
					prefix + configuration::SPECIAL_SUFFIX,
					juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.f),
					special));
				modParams[SPECIAL].addModParams(layout, prefix + configuration::SPECIAL_SUFFIX);
			}

			virtual void registerAsListener(juce::AudioProcessorValueTreeState& apvts) override {
				apvts.addParameterListener(prefix + configuration::BYPASSED_SUFFIX, this);
				apvts.addParameterListener(prefix + configuration::FILTER_TYPE_SUFFIX, this);
				apvts.addParameterListener(prefix + configuration::CUTOFF_SUFFIX, this);
				modParams[CUTOFF].registerAsListener(apvts, prefix + configuration::CUTOFF_SUFFIX);
				apvts.addParameterListener(prefix + configuration::RESONANCE_SUFFIX, this);
				modParams[RES].registerAsListener(apvts, prefix + configuration::RESONANCE_SUFFIX);
				apvts.addParameterListener(prefix + configuration::DRIVE_SUFFIX, this);
				modParams[SPECIAL].registerAsListener(apvts, prefix + configuration::SPECIAL_SUFFIX);

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
				else if (parameterID.endsWith(prefix + configuration::SPECIAL_SUFFIX)) {
					special = newValue;
				}
			}
		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedData)
		};

		FilterChooser() = delete;

		FilterChooser(SharedData* t_data) : data(t_data) {}

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
		virtual ~Filter() {}

		virtual void updateMode() = 0;
		virtual void prepare(const juce::dsp::ProcessSpec& spec) override {
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
		virtual void reset() override {}
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override { return false; }
		virtual void prepareUpdate() {}
	private:
		void updateMode() override {}
	};

	// This is basically the StateVariableTPTFilter from the JUCE library
	// credits to the JUCE team and Vadim Zavalishin
	class TPTFilter : public Filter {
		using Filter::Filter;
	public:
		virtual void prepareUpdate() override;
		virtual void reset() override;
		virtual bool process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) override;

	private:
		void updateMode() override;

		void snapToZero() {
			for (auto v : { &s1, &s2 }) {
				for (auto& element : *v) {
					juce::dsp::util::snapToZero(element);
				}
			}
		}
		std::vector<float>s1{ 2 }, s2{ 2 };
	};
} 