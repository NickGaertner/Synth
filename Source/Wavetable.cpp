
#include "Wavetable.h"

namespace wavetable {
	// TODO let the oscs do all the logic since it should be much faster
	float Wavetable::getSample(int exponent, float wtPos, float phase) const
	{
		jassert(0.f <= phase && phase <= juce::MathConstants<float>::twoPi);
		jassert(-1.f <= wtPos && wtPos <= 2.f); // we can break the [0,1] limit with modulation
		wtPos = std::clamp(wtPos, 0.f, 1.f);

		auto& wt = getTable(exponent);

		auto scaledWtPos = wtPos * (wt.getNumChannels() - 1 - 1);
		int channelIndex = static_cast<int>(scaledWtPos);
		auto channelDelta = scaledWtPos - channelIndex;

		auto channel0 = wt.getReadPointer(channelIndex);
		auto channel1 = wt.getReadPointer(channelIndex + 1);

		auto scaledPhase = (phase / juce::MathConstants<float>::twoPi) * wtResolution;
		int sampleIndex = static_cast<int>(scaledPhase);
		auto phaseDelta = scaledPhase - sampleIndex;

		auto sample0 = (1.f - phaseDelta) * channel0[sampleIndex] + (phaseDelta)*channel0[sampleIndex + 1];
		auto sample1 = (1.f - phaseDelta) * channel1[sampleIndex] + (phaseDelta)*channel1[sampleIndex + 1];

		return (1.f - channelDelta) * sample0 + channelDelta * sample1;
	}

	const juce::AudioBuffer<float>& Wavetable::getTable(int exponent) const
	{
		jassert(4 <= exponent && exponent <= 14);
		return tables[exponent - 4];
	}

	const juce::String& Wavetable::getName()
	{
		return name;
	}

	int Wavetable::getMaxHarmonics(double sampleRate)
	{
		jassert(sampleRate > 0);
		return static_cast<int>(sampleRate / (2. * juce::MidiMessage::getMidiNoteInHertz(21)));
	}

	void Wavetable::createFromHarmonicWeights(double sampleRate, const juce::Array<double>& weights)
	{
		updateSineValues(sampleRate);
		int maxHarmonics = getMaxHarmonics(sampleRate);
		jassert(weights.size() == maxHarmonics);

		juce::AudioBuffer<double> buffer{ maxHarmonics, wtResolution };

		// fill buffer with weighted sine values
		for (int i = 0; i < maxHarmonics; i++) {
			buffer.copyFrom(i, 0, sineValues, i, 0, wtResolution);
			buffer.applyGain(i, 0, wtResolution, weights[i]);
		}

		// calculate the sums in place 
		// add in reverse order in hopes of less rounding errors 
		// (Yes, the complexity is worse)
		// also normalize the values to a [-1,1] range
		for (int i = maxHarmonics - 1; i >= 0; i--) {
			for (auto j = i - 1; j >= 0; j--) {
				buffer.addFrom(i, 0, buffer, j, 0, wtResolution);
			}
			// normalize
			buffer.applyGain(i, 0, wtResolution, 1.0/buffer.getMagnitude(i, 0, wtResolution));
		}

		for (int exponent = 4; exponent <= 14; exponent++) {
			tables[exponent - 4] = juce::AudioBuffer<float>(63 + 1, wtResolution + 1);
			auto& wt = tables[exponent - 4];
			// number of harmonics for upper frequency bound
			auto orderBound = sampleRate / (2 * std::powf(2, exponent + 1));

			for (int i = 0; i < 63; i++) {
				auto* channel = wt.getWritePointer(i);

				auto order = (i / 62.f) * orderBound;
				auto order0 = static_cast<int>(std::floorf(order));
				auto orderDelta = order - order0;
				auto* srcChannel0 = buffer.getReadPointer(order);
				auto* srcChannel1 = buffer.getReadPointer(order + 1); // works since we have calculated some leftover harmonics

				for (int sample = 0; sample < wtResolution; sample++) {
					channel[sample] = (float)((1.f - orderDelta) * srcChannel0[sample] + orderDelta * srcChannel1[sample]);
				}
				// for wrap condition
				channel[wtResolution] = channel[0];

				// normalize
				wt.applyGain(i,0,wtResolution, 1.0 / wt.getMagnitude(i,0,wtResolution));
			}

			// last channel can be left empty/uninitialized since it won't be accessed
		}
	}

	void Wavetable::updateSineValues(double sampleRate)
	{
		jassert(sampleRate > 0.);

		// Nyquist
		int maxHarmonics = getMaxHarmonics(sampleRate);

		// check if already up to date
		if (sineValues.getNumChannels() == maxHarmonics) {
			return;
		}

		sineValues = juce::AudioBuffer<double>(maxHarmonics, wtResolution);
		for (int harmonic = 0; harmonic < maxHarmonics; harmonic++) {
			auto* channel = sineValues.getWritePointer(harmonic);
			for (int sample = 0; sample < wtResolution; sample++) {
				channel[sample] = std::sin((harmonic + 1) * sample * juce::MathConstants<double>::twoPi / wtResolution);
			}
		}
	}

	// the wavetables need to be added in the order of the enum
	void WavetableManager::initAll()
	{	
		if (wavetables.size() != 0) {
			return;
		}

		auto add = [&](Wavetable* wt) {
			wavetables.add(wt);
			wavetableNames.add(wt->getName());
		};

		add(new SawHarmonicsWavetable());
		add(new SquareHarmonicsWavetable());
		add(new TriangleHarmonicsWavetable());
		add(new PWMWavetable());
		add(new TrapezWavetable());

		jassert(wavetables.size() == wavetableNames.size() && wavetables.size() == NUMBER_OF_WT);
	}

	const Wavetable* WavetableManager::getWavetable(int index, double sampleRate)
	{
		initAll();
		jassert(0 <= index && index < wavetables.size());
		wavetables[index]->create(sampleRate);
		return wavetables[index];
	}

	const juce::StringArray& WavetableManager::getWavetableNames()
	{
		initAll();
		return wavetableNames;
	}

	void WavetableManager::cleanUp()
	{
		wavetables.clear();
		wavetableNames.clear();
	}


	void SawHarmonicsWavetable::create(double sampleRate)
	{
		// check if up to date
		if (oldSampleRate == sampleRate) {
			return;
		}
		auto maxHarmonics = getMaxHarmonics(sampleRate);
		juce::Array<double> weights;
		for (int i = 0; i < maxHarmonics; i++) {
			weights.add((i % 2 == 0) ? 1. / (i + 1) : -1. / (i + 1));
		}
		createFromHarmonicWeights(sampleRate, weights);
		oldSampleRate = sampleRate;
	}

	void SquareHarmonicsWavetable::create(double sampleRate)
	{
		// check if up to date
		if (oldSampleRate == sampleRate) {
			return;
		}
		auto maxHarmonics = getMaxHarmonics(sampleRate);
		juce::Array<double> weights;
		for (int i = 0; i < maxHarmonics; i++) {
			weights.add((i % 2 == 0) ? 1. / (i + 1) : 0.0);
		}
		createFromHarmonicWeights(sampleRate, weights);
		oldSampleRate = sampleRate;
	}

	void TriangleHarmonicsWavetable::create(double sampleRate)
	{
		// check if up to date
		if (oldSampleRate == sampleRate) {
			return;
		}
		auto maxHarmonics = getMaxHarmonics(sampleRate);
		juce::Array<double> weights;
		double sign = 1.0;
		for (int i = 0; i < maxHarmonics; i++) {
			if (i % 2 == 0) {
				weights.add(sign / ((i + 1) * (i + 1)));
				sign *= -1.0;
			}
			else {
				weights.add(0.0);
			}
		}
		createFromHarmonicWeights(sampleRate, weights);
		oldSampleRate = sampleRate;
	}

	void PWMWavetable::create(double sampleRate)
	{
		// check if up to date
		if (oldSampleRate == sampleRate) {
			return;
		}

		updateSineValues(sampleRate);
		auto saw = WavetableManager::getWavetable(WavetableManager::SAW_HARMONICS, sampleRate);

		// TODO make 63 a constant and refactor code
		for (int exponent = 4; exponent <= 14; exponent++) {
			tables[exponent - 4] = juce::AudioBuffer<float>(63 + 1, wtResolution + 1);
			auto& wt = tables[exponent - 4];

			// the second to last channel (62) has a saw wave with all available harmonics
			auto sawChannel = saw->getTable(exponent).getReadPointer(62);
			const float delta = 1;
			for (int i = 0; i < 63; i++) {
				auto* channel = wt.getWritePointer(i);
				// we dont want the case wtPosition = 0 since this would result in a constant wavefunction
				float wtPosition = (i + delta) / (62.0+delta);

				for (int sample = 0; sample < wtResolution; sample++) {
					channel[sample] = sawChannel[sample] - sawChannel[(sample + (int)(wtPosition * wtResolution / 2.f))%wtResolution] + (wtPosition - 1);
				}
				// for wrap condition
				channel[wtResolution] = channel[0];

				// normalize
				wt.applyGain(i, 0, wtResolution, 1.0 / wt.getMagnitude(i, 0, wtResolution));
			}

			// last channel can be left empty/uninitialized since it won't be accessed
		}
		oldSampleRate = sampleRate;
	}

	void TrapezWavetable::create(double sampleRate)
	{
		// check if up to date
		if (oldSampleRate == sampleRate) {
			return;
		}

		updateSineValues(sampleRate);
		auto triangle = WavetableManager::getWavetable(WavetableManager::TRIANGLE_HARMONICS, sampleRate);

		// TODO make 63 a constant and refactor code
		for (int exponent = 4; exponent <= 14; exponent++) {
			tables[exponent - 4] = juce::AudioBuffer<float>(63 + 1, wtResolution + 1);
			auto& wt = tables[exponent - 4];

			// the second to last channel (62) has a triangle wave with all available harmonics
			auto triangleChannel = triangle->getTable(exponent).getReadPointer(62);
			const float delta = 1;
			for (int i = 0; i < 63; i++) {
				auto* channel = wt.getWritePointer(i);
				// we dont want the case wtPosition = 1 since we divide by (wtPosition - 1)
				float wtPosition = (i ) / (62.0 + delta);

				for (int sample = 0; sample < wtResolution; sample++) {
					jassert(1.f - wtPosition != 0.f);
					channel[sample] = (triangleChannel[sample] + triangleChannel[(sample + (int)(wtPosition * wtResolution / 2.f)) % wtResolution])/(2.f*(1.f-wtPosition));
				}
				// for wrap condition
				channel[wtResolution] = channel[0];

				// normalize
				wt.applyGain(i, 0, wtResolution, 1.0 / wt.getMagnitude(i, 0, wtResolution));
			}

			// last channel can be left empty/uninitialized since it won't be accessed
		}
		oldSampleRate = sampleRate;
	}
}