
#include "Filter.h"

namespace customDsp {

	void replaceIdWithFilterName(juce::XmlElement& xml) {
		jassert(xml.getTagName() == configuration::VALUE_TREE_IDENTIFIER);
		for (auto* paramXml : xml.getChildIterator()) {
			if (paramXml->getStringAttribute("id").endsWith(configuration::FILTER_TYPE_SUFFIX)) {
				paramXml->setAttribute("value", FILTER_TYPE_NAMES[static_cast<int>(paramXml->getDoubleAttribute("value"))]);
			}
		}
	}

	void replaceFilterNameWithId(juce::XmlElement& xml) {
		jassert(xml.getTagName() == configuration::VALUE_TREE_IDENTIFIER);
		for (auto* paramXml : xml.getChildIterator()) {
			if (paramXml->getStringAttribute("id").endsWith(configuration::FILTER_TYPE_SUFFIX)) {
				auto filterName = paramXml->getStringAttribute("value");
				auto index = FILTER_TYPE_NAMES.indexOf(filterName);
				if (index != -1) {
					paramXml->setAttribute("value", index);
				}
				else {
					jassertfalse;
				}
			}
		}
	}

	FilterChooser* FilterChooser::SharedData::createProcessor() {
		return new FilterChooser(this);
	}

	void FilterChooser::SharedData::addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {

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

	void FilterChooser::SharedData::registerAsListener(juce::AudioProcessorValueTreeState& apvts) {
		apvts.addParameterListener(prefix + configuration::BYPASSED_SUFFIX, this);
		apvts.addParameterListener(prefix + configuration::FILTER_TYPE_SUFFIX, this);
		apvts.addParameterListener(prefix + configuration::CUTOFF_SUFFIX, this);
		modParams[CUTOFF].registerAsListener(apvts, prefix + configuration::CUTOFF_SUFFIX);
		apvts.addParameterListener(prefix + configuration::RESONANCE_SUFFIX, this);
		modParams[RES].registerAsListener(apvts, prefix + configuration::RESONANCE_SUFFIX);
		apvts.addParameterListener(prefix + configuration::DRIVE_SUFFIX, this);
		modParams[SPECIAL].registerAsListener(apvts, prefix + configuration::SPECIAL_SUFFIX);
	}

	void FilterChooser::SharedData::parameterChanged(const juce::String& parameterID, float newValue) {
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

	void FilterChooser::createFilter(FilterType type) {
		if (type == FilterType::NONE) {
			filter = std::make_unique<DummyFilter>(data);
		}
		else {
			filter = std::make_unique<TPTFilter>(data);
		}

		filter->prepareUpdate();
		if (isNoteOn) {
			filter->noteOn();
		}
	}

	void FilterChooser::noteOn() {
		Processor::noteOn();
		if (filter) {
			filter->noteOn();
		}
	}

	void FilterChooser::noteOff() {
		Processor::noteOff();
		if (filter) {
			filter->noteOff();
		}
	}

	void FilterChooser::prepare(const juce::dsp::ProcessSpec& spec) {
		data->sampleRate = spec.sampleRate;
		data->numChannels = spec.numChannels;
		createFilter(data->filterType);
	}

	void FilterChooser::reset() {
		if (filter != nullptr) {
			filter->reset();
		}
	}

	bool FilterChooser::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{
		if (data->bypassed || context.isBypassed) {
			return false;
		}
		if (data->filterType != filter->mode) {
			if (!isSameFilter(data->filterType, filter->mode)) {
				createFilter(data->filterType);
			}
			else {
				filter->updateMode();
			}
		}
		return filter->process(context, workBuffers);
	};

	void TPTFilter::prepareUpdate() {
		if (data->numChannels > 0) {
			s1.resize(data->numChannels);
			s2.resize(data->numChannels);
			updateMode();
			reset();
		}
	}

	void TPTFilter::reset() {
		for (auto v : { &s1, &s2 }) {
			std::fill(v->begin(), v->end(), 0.f);
		}
	}

	void TPTFilter::updateMode() {
		mode = data->filterType;
	}

	bool TPTFilter::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) {

		if (data->bypassed || context.isBypassed) {
			return false;
		}

		auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
		auto& outputBlock = context.getOutputBlock();

		float cutoffBase = data->cutoff;
		auto cutoffMod = data->modParams[FilterChooser::SharedData::CUTOFF].factor;
		auto cutoffModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FilterChooser::SharedData::CUTOFF].src_channel);

		float resonanceBase = data->resonance;
		auto resonanceMod = data->modParams[FilterChooser::SharedData::RES].factor;
		auto resonanceModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FilterChooser::SharedData::RES].src_channel);

		for (int start = 0; start < outputBlock.getNumSamples(); start += configuration::MOD_BLOCK_SIZE) {

			auto end = juce::jmin(start + configuration::MOD_BLOCK_SIZE, (int)workBuffers.getNumSamples());

			auto currentCutoff = juce::jmap(juce::jlimit(0.f, 1.f, cutoffBase + cutoffMod * cutoffModSrc[start]), 100.f, 18000.f);
			float g = juce::dsp::FastMathApproximations::tan<float>(juce::MathConstants<float>::pi * currentCutoff / static_cast<float>(data->sampleRate));

			auto currentResonance = juce::jmap(juce::jlimit(0.f, 1.f, resonanceBase + resonanceMod * resonanceModSrc[start]), 0.1f, 2.f);
			float R2 = 1.0f / currentResonance;
			float h = 1.0f / (1.0f + R2 * g + g * g);

			for (int channel = 0; channel < outputBlock.getNumChannels(); channel++) {
				auto* channelPtr = outputBlock.getChannelPointer(channel);

				auto ls1 = s1[channel];
				auto ls2 = s2[channel];

				for (int sample = start; sample < end; sample++) {

					auto yHP = h * (channelPtr[sample] - ls1 * (g + R2) - ls2);

					auto yBP = yHP * g + ls1;
					ls1 = yHP * g + yBP;

					auto yLP = yBP * g + ls2;
					ls2 = yBP * g + yLP;

					float value = 0.f;
					switch (mode) { //actually not slow
					case FilterType::TPT_LPF12: value = yLP; break;
					case FilterType::TPT_BPF12: value = yBP; break;
					case FilterType::TPT_HPF12: value = yHP; break;
					default: jassertfalse; break;
					}
					channelPtr[sample] = value;
				}

				s1[channel] = ls1;
				s2[channel] = ls2;
			}
		}
		return isNoteOn;
	}
}
