
#include "Filter.h"

void customDsp::FilterChooser::createFilter(FilterType type) {
	if (type == FilterType::NONE) {
		filter = std::make_unique<DummyFilter>(data);
	}
	else {
		filter = std::make_unique<LadderFilter>(data);
	}

	filter->updateMode();
}

void customDsp::FilterChooser::prepare(const juce::dsp::ProcessSpec& spec) {
	data->sampleRate = spec.sampleRate;
	filter->prepare(spec);
}

void customDsp::FilterChooser::reset() {
	filter->reset();
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

void customDsp::LadderFilter::prepare(const juce::dsp::ProcessSpec& spec) {
	state.resize(spec.numChannels);
	reset();
}

void customDsp::LadderFilter::reset()
{
	for (auto& s : state) {
		s.fill(0.f);
	}
}

void customDsp::LadderFilter::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
{
	if (data->bypassed || context.isBypassed) {
		return;
	}
	setDrive(1.2f);

	const auto& inputBlock = context.getInputBlock();
	auto& outputBlock = context.getOutputBlock();

	float cutoffTransformValue = std::exp(cutoffFreqHz * (-juce::MathConstants<float>::twoPi / data->sampleRate));
	float scaledResonanceValue = juce::jmap(resonance, 0.1f, 1.0f);

	const auto a1 = cutoffTransformValue;
	const auto g = 1.f - a1;
	const auto b0 = g * 0.76923076923f;
	const auto b1 = g * 0.23076923076f;

	const auto A0 = A[0];
	const auto A1 = A[1];
	const auto A2 = A[2];
	const auto A3 = A[3];
	const auto A4 = A[4];

	for (size_t channel = 0; channel < outputBlock.getNumChannels(); channel++) {

		auto dxBlock = workBuffers.getSingleChannelBlock(0);
		dxBlock.replaceWithProductOf(outputBlock.getSingleChannelBlock(channel), drive);
		juce::dsp::FastMathApproximations::tanh(dxBlock.getChannelPointer(0), dxBlock.getNumSamples());
		dxBlock.multiplyBy(gain);
		const auto dx = dxBlock.getChannelPointer(0);

		auto outputChannel = outputBlock.getChannelPointer(channel);
		auto& s = state[channel];

		// use local variables for our calculation to avoid excessive array access
		// (array access was responsible for most of this methods cpu usage)
		auto s0 = s[0];
		auto s1 = s[1];
		auto s2 = s[2];
		auto s3 = s[3];
		auto s4 = s[4];

		for (size_t n = 0; n < outputBlock.getNumSamples(); n++) {

			const auto a = dx[n] + scaledResonanceValue * -4.f * (gain2 * juce::dsp::FastMathApproximations::tanh(drive2 * s4) - dx[n] * comp);

			const auto b = b1 * s0 + a1 * s1 + b0 * a;
			const auto c = b1 * s1 + a1 * s2 + b0 * b;
			const auto d = b1 * s2 + a1 * s3 + b0 * c;
			const auto e = b1 * s3 + a1 * s4 + b0 * d;

			s0 = a;
			s1 = b;
			s2 = c;
			s3 = d;
			s4 = e;

			outputChannel[n] = a * A0 + b * A1 + c * A2 + d * A3 + e * A4;
		}

		s[0] = s0;
		s[1] = s1;
		s[2] = s2;
		s[3] = s3;
		s[4] = s4;
	}
}

void customDsp::LadderFilter::updateMode()
{
	auto newMode = data->filterType;
	if (newMode == mode) {
		return;
	}
	mode = newMode;

	switch (mode) {
	case FilterType::LADDER_LPF12:   A = { { 0.f, 0.f,  1.f, 0.f,  0.f } }; comp = 0.5f;  break;
	case FilterType::LADDER_HPF12:   A = { { 1.f, -2.f, 1.f, 0.f,  0.f } }; comp = 0.f;    break;
	case FilterType::LADDER_BPF12:   A = { { 0.f, 0.f, -1.f, 1.f,  0.f } }; comp = 0.5f;  break;
	case FilterType::LADDER_LPF24:   A = { { 0.f, 0.f,  0.f, 0.f,  1.f } }; comp = 0.5f;  break;
	case FilterType::LADDER_HPF24:   A = { { 1.f, -4.f, 6.f, -4.f, 1.f } }; comp = 0.f;    break;
	case FilterType::LADDER_BPF24:   A = { { 0.f, 0.f,  1.f, -2.f, 1.f } }; comp = 0.5f;  break;
	default: jassertfalse; break;
	}
	for (auto& a : A) {
		a *= 1.2f;
	}

	reset();
}
void customDsp::LadderFilter::setDrive(float newDrive)
{
	jassert(1.f <= newDrive);
	drive = newDrive;
	gain = std::powf(drive, -2.642f) * 0.6103f + 0.3903f;
	drive2 = drive * 0.04f + 0.96f;
	gain2 = std::powf(drive2, -2.642f) * 0.6103f + 0.3903f;
}


