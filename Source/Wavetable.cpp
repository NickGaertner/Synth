
#include "Wavetable.h"

namespace wavetable {

	JUCE_IMPLEMENT_SINGLETON(WavetableCache);

	Wavetable::Wavetable(const juce::String& t_name, const juce::MemoryBlock& waveData) : name(t_name) {
		auto channelsPerTable = int(waveData.getBitRange(0, 32));
		auto wtResolution = int(waveData.getBitRange(32, 32));

		//make sure we have exactly the data for 11 tables
		jassert((waveData.getSize() - 8) == 11 * (wtResolution * channelsPerTable) * 4);
		const float* dataPtr = reinterpret_cast<const float*>((waveData.begin() + 8));

		for (int t = 0; t < 11; t++) {
			tables[t] = juce::AudioBuffer<float>{ channelsPerTable + 1, wtResolution + 1 };
			auto& table = tables[t];
			for (int channel = 0; channel < channelsPerTable; channel++) {

				table.copyFrom(channel, 0, dataPtr, wtResolution);
				dataPtr += wtResolution;
				table.setSample(channel, wtResolution, table.getSample(channel, 0));
			}
			// clear last channel
			table.clear(channelsPerTable, 0, table.getNumSamples());
		}
	}

	void Wavetable::toMemoryBlock(juce::MemoryBlock& blockToFill) const
	{
		int numChannels = tables[0].getNumChannels() - 1;
		int numSamples = tables[0].getNumSamples() - 1;
		blockToFill.setSize(2 * 4 + 11 * numChannels * numSamples * 4);
		{
			int* dataPtr = static_cast<int*>(blockToFill.getData());
			dataPtr[0] = numChannels;
			dataPtr[1] = numSamples;
		}
		int memOffset = 8;
		int bytesPerChannel = numSamples * 4;
		for (int t = 0; t < 11; t++) {
			auto& table = tables[t];
			for (int channel = 0; channel < numChannels; channel++) {
				blockToFill.copyFrom(table.getReadPointer(channel), memOffset, bytesPerChannel);
				memOffset += bytesPerChannel;
			}
		}
	}


	const juce::AudioBuffer<float>& Wavetable::getTable(int exponent) const
	{
		exponent = juce::jlimit(4, 14, exponent); // through pitch you could get values outside the normal frequency range
		return tables[exponent - 4];
	}

	const juce::AudioBuffer<float>& Wavetable::getTable(float frequency) const
	{
		int upperBoundExponent;
		static_cast<void>(std::frexpf(frequency, &upperBoundExponent));
		return getTable(upperBoundExponent-1);
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

		juce::AudioBuffer<double> buffer{ maxHarmonics, defaultWtResolution };

		// fill buffer with weighted sine values
		for (int i = 0; i < maxHarmonics; i++) {
			buffer.copyFrom(i, 0, sineValues, i, 0, defaultWtResolution);
			buffer.applyGain(i, 0, defaultWtResolution, weights[i]);
		}

		// calculate the sums in place 
		// add in reverse order in hopes of less rounding errors 
		// (Yes, the complexity is worse)
		// also normalize the values to a [-1,1] range
		for (int i = maxHarmonics - 1; i >= 0; i--) {
			for (auto j = i - 1; j >= 0; j--) {
				buffer.addFrom(i, 0, buffer, j, 0, defaultWtResolution);
			}
			// normalize
			buffer.applyGain(i, 0, defaultWtResolution, 1.0 / buffer.getMagnitude(i, 0, defaultWtResolution));
		}

		for (int exponent = 4; exponent <= 14; exponent++) {
			tables[exponent - 4] = juce::AudioBuffer<float>(63 + 1, defaultWtResolution + 1);
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

				for (int sample = 0; sample < defaultWtResolution; sample++) {
					channel[sample] = (float)((1.f - orderDelta) * srcChannel0[sample] + orderDelta * srcChannel1[sample]);
				}
				// for wrap condition
				channel[defaultWtResolution] = channel[0];

				// normalize
				wt.applyGain(i, 0, defaultWtResolution + 1, 1.0 / wt.getMagnitude(i, 0, defaultWtResolution + 1));
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

		sineValues = juce::AudioBuffer<double>(maxHarmonics, defaultWtResolution);
		for (int harmonic = 0; harmonic < maxHarmonics; harmonic++) {
			auto* channel = sineValues.getWritePointer(harmonic);
			for (int sample = 0; sample < defaultWtResolution; sample++) {
				channel[sample] = std::sin((harmonic + 1) * sample * juce::MathConstants<double>::twoPi / defaultWtResolution);
			}
		}
	}

	WavetableCache::WavetableCache() {
		juce::String userAppDirPath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory)
			.getFullPathName();

		wtFolder = std::make_unique<juce::File>(userAppDirPath + juce::File::getSeparatorString() + configuration::WT_FOLDER_NAME);
		if (!wtFolder->isDirectory()) {
			auto result = wtFolder->createDirectory();
			if (result.failed()) {
				DBG(result.getErrorMessage());
				jassertfalse;
			}
		}
		auto children = wtFolder->findChildFiles(
			juce::File::TypesOfFileToFind::findFiles,
			true,
			"*" + configuration::WT_FILE_EXTENSION,
			juce::File::FollowSymlinks::no);

		wavetableNames.addArray(GENERATABLE_WT_NAMES);

		for (auto& file : children) {
			if (!GENERATABLE_WT_NAMES.contains(file.getFileNameWithoutExtension())) {
				wavetableNames.add(file.getFileNameWithoutExtension());
			}
		}
	};

	Wavetable::Ptr WavetableCache::getWavetable(const juce::String& wtName) {
		// check if Wt is already in memory
		for (auto& wt : wavetables) {
			if (wt->getName().equalsIgnoreCase(wtName)) {
				return wt;
			}
		}

		if (wavetables.size() >= configuration::MAX_TABLES_IN_MEM) {
			deleteUnused();
		}

		auto wtFile = wtFolder->getChildFile(wtName + configuration::WT_FILE_EXTENSION);
		// try to load from a file
		if (wtFile.existsAsFile()) {
			if (wtFile.getSize() > 10 * 1000 * 1000) { // 10MB
				jassertfalse;
			}
			juce::MemoryBlock data;
			if (!wtFile.loadFileAsData(data)) {
				jassertfalse;
			}

			Wavetable* wtPtr = new Wavetable{ wtName, data };
			wavetables.add(wtPtr);
			return wtPtr;
		}
		// see if it is a default wt which can be generated
		int nameIndex = GENERATABLE_WT_NAMES.indexOf(wtName);
		if (nameIndex != -1) {
			// Note that we only generate wavetables with a sample rate of 44100
			Wavetable* wtPtr;
			switch (static_cast<GeneratableWt>(nameIndex)) {
			case GeneratableWt::SawHarmonics:  wtPtr = new SawHarmonicsWavetable(GENERATABLE_WT_NAMES[nameIndex], 44100.0); break;
			case GeneratableWt::TriangleHarmonics:  wtPtr = new TriangleHarmonicsWavetable(GENERATABLE_WT_NAMES[nameIndex], 44100.0); break;
			case GeneratableWt::SquareHarmonics:  wtPtr = new SquareHarmonicsWavetable(GENERATABLE_WT_NAMES[nameIndex], 44100.0); break;
			case GeneratableWt::PWM:  wtPtr = new PWMWavetable(GENERATABLE_WT_NAMES[nameIndex], 44100.0); break;
			case GeneratableWt::Trapez:  wtPtr = new TrapezWavetable(GENERATABLE_WT_NAMES[nameIndex], 44100.0); break;
			default: jassertfalse; break;
			}
			// write the wavetable to the file so it can be easily loaded the next time it's needed
			juce::MemoryBlock data;
			wtPtr->toMemoryBlock(data);
			if (!wtFile.replaceWithData(data.getData(), data.getSize())) {
				jassertfalse;
			}
			wavetables.add(*wtPtr);
			return wtPtr;
		}

		// at this point the wt couldn't be found or generated
		jassertfalse;
	}

	void SawHarmonicsWavetable::create(double sampleRate)
	{
		auto maxHarmonics = getMaxHarmonics(sampleRate);
		juce::Array<double> weights;
		for (int i = 0; i < maxHarmonics; i++) {
			weights.add((i % 2 == 0) ? 1. / (i + 1) : -1. / (i + 1));
		}
		createFromHarmonicWeights(sampleRate, weights);
	}

	void SquareHarmonicsWavetable::create(double sampleRate)
	{
		auto maxHarmonics = getMaxHarmonics(sampleRate);
		juce::Array<double> weights;
		for (int i = 0; i < maxHarmonics; i++) {
			weights.add((i % 2 == 0) ? 1. / (i + 1) : 0.0);
		}
		createFromHarmonicWeights(sampleRate, weights);
	}

	void TriangleHarmonicsWavetable::create(double sampleRate)
	{
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
	}

	void PWMWavetable::create(double sampleRate)
	{
		updateSineValues(sampleRate);
		auto saw = WavetableCache::getInstance()->getWavetable("SawHarmonics");
		// TODO make 63 a constant and refactor code
		for (int exponent = 4; exponent <= 14; exponent++) {
			tables[exponent - 4] = juce::AudioBuffer<float>(63 + 1, defaultWtResolution + 1);
			auto& wt = tables[exponent - 4];

			// the second to last channel (62) has a saw wave with all available harmonics
			auto sawChannel = saw->getTable(exponent).getReadPointer(62);
			const float delta = 2;
			for (int i = 0; i < 63; i++) {
				auto* channel = wt.getWritePointer(i);
				// we dont want the case wtPosition = 0 since this would result in a constant wavefunction
				float wtPosition = (i + delta) / (62.0 + delta);

				for (int sample = 0; sample < defaultWtResolution; sample++) {
					channel[sample] = sawChannel[sample]
						- sawChannel[(sample + (int)(wtPosition * defaultWtResolution / 2.f)) % defaultWtResolution]
						+ (wtPosition - 1);
				}
				// for wrap condition
				channel[defaultWtResolution] = channel[0];

				// normalize
				wt.applyGain(i, 0, defaultWtResolution + 1, 1.0 / wt.getMagnitude(i, 0, defaultWtResolution + 1));
			}

			// last channel can be left empty/uninitialized since it won't be accessed
		}
	}

	void TrapezWavetable::create(double sampleRate)
	{
		updateSineValues(sampleRate);
		auto triangle = WavetableCache::getInstance()->getWavetable("TriangleHarmonics");

		// TODO make 63 a constant and refactor code
		for (int exponent = 4; exponent <= 14; exponent++) {
			tables[exponent - 4] = juce::AudioBuffer<float>(63 + 1, defaultWtResolution + 1);
			auto& wt = tables[exponent - 4];

			// the second to last channel (62) has a triangle wave with all available harmonics
			auto triangleChannel = triangle->getTable(exponent).getReadPointer(62);
			const float delta = 1;
			for (int i = 0; i < 63; i++) {
				auto* channel = wt.getWritePointer(i);
				// we dont want the case wtPosition = 1 since we divide by (wtPosition - 1)
				float wtPosition = (i) / (62.0 + delta);

				for (int sample = 0; sample < defaultWtResolution; sample++) {
					jassert(1.f - wtPosition != 0.f);
					channel[sample] = (triangleChannel[sample]
						+ triangleChannel[(sample + (int)(wtPosition * defaultWtResolution / 2.f)) % defaultWtResolution])
						/ (2.f * (1.f - wtPosition));
				}
				// for wrap condition
				channel[defaultWtResolution] = channel[0];

				// normalize
				wt.applyGain(i, 0, defaultWtResolution + 1, 1.0 / wt.getMagnitude(i, 0, defaultWtResolution + 1));
			}

			// last channel can be left empty/uninitialized since it won't be accessed
		}
	}
}