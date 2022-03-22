
#include "Filter.h"

void customDsp::FilterChooser::createFilter(FilterType type) {
	if (type == FilterType::NONE) {
		filter = std::make_unique<DummyFilter>(data);
	}
	else {
		filter = std::make_unique<TPTFilter>(data);
	}

	filter->prepareUpdate();
}

void customDsp::FilterChooser::prepare(const juce::dsp::ProcessSpec& spec) {
	data->sampleRate = spec.sampleRate;
	data->numberOfChannels = spec.numChannels;
	createFilter(data->filterType);
}

void customDsp::FilterChooser::reset() {
	if (filter != nullptr) {
		filter->reset();
	}
}

void customDsp::FilterChooser::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
{
	if (data->bypassed || context.isBypassed) {
		return;
	}
	if (data->filterType != filter->mode) {
		if (!isSameFilter(data->filterType, filter->mode)) {
			createFilter(data->filterType);
		}
		else {
			filter->updateMode();
		}
	}
	filter->process(context, workBuffers);
};

void customDsp::TPTFilter::prepareUpdate() {
	if (data->numberOfChannels > 0) {
		s1.resize(data->numberOfChannels);
		s2.resize(data->numberOfChannels);
		updateMode();
		reset();
	}
}

void customDsp::TPTFilter::reset() {
	for (auto v : { &s1, &s2 }) {
		std::fill(v->begin(), v->end(), 0.f);
	}
}

void customDsp::TPTFilter::updateMode() {
	mode = data->filterType;
}

void customDsp::TPTFilter::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) {

	if (data->bypassed || context.isBypassed) {
		return;
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

		auto currentCutoff = juce::jmap(juce::jlimit(0.f,1.f,cutoffBase + cutoffMod * cutoffModSrc[start]), 100.f, 1900.f);
		float g = juce::dsp::FastMathApproximations::tan(juce::MathConstants<float>::pi * currentCutoff / data->sampleRate);

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

				float value;
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
	snapToZero();
}