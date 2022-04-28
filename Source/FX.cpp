
#include "FX.h"

namespace customDsp {
	void replaceIdWithFXName(juce::XmlElement& xml)
	{
		jassert(xml.getTagName() == configuration::VALUE_TREE_IDENTIFIER);
		for (auto* paramXml : xml.getChildIterator()) {
			if (paramXml->getStringAttribute("id").endsWith(configuration::FX_TYPE_SUFFIX)) {
				paramXml->setAttribute("value", FX_TYPE_NAMES[static_cast<int>(paramXml->getDoubleAttribute("value"))]);
			}
		}
	}

	void replaceFXNameWithId(juce::XmlElement& xml)
	{
		jassert(xml.getTagName() == configuration::VALUE_TREE_IDENTIFIER);
		for (auto* paramXml : xml.getChildIterator()) {
			if (paramXml->getStringAttribute("id").endsWith(configuration::FX_TYPE_SUFFIX)) {
				auto filterName = paramXml->getStringAttribute("value");
				auto index = FX_TYPE_NAMES.indexOf(filterName);
				if (index != -1) {
					paramXml->setAttribute("value", index);
				}
				else {
					jassertfalse;
				}
			}
		}
	}

	FXChooser* FXChooser::SharedData::createProcessor() {
		return new FXChooser(this);
	}

	void FXChooser::SharedData::addParams(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {

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

	void FXChooser::SharedData::registerAsListener(juce::AudioProcessorValueTreeState& apvts) {
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

	void FXChooser::SharedData::parameterChanged(const juce::String& parameterID, float newValue) {
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

	void FXChooser::createFX(FXType type)
	{
		switch (type) {
		case FXType::CHORUS: fx = std::make_unique<Chorus>(data); break;
		case FXType::DELAY: fx = std::make_unique<Delay>(data); break;
		case FXType::FLANGER: fx = std::make_unique<Flanger>(data); break;
		case FXType::REVERB: fx = std::make_unique<Reverb>(data); break;
		case FXType::PHASER: fx = std::make_unique<Phaser>(data); break;
		case FXType::DISTORTION: fx = std::make_unique<Distortion>(data); break;
		case FXType::NONE: fx = std::make_unique<DummyFX>(data); break;
		default: jassertfalse; break;
		}

		fx->prepareUpdate();
		if (isNoteOn) {
			fx->noteOn();
		}
	}

	void FXChooser::prepare(const juce::dsp::ProcessSpec& spec)
	{
		data->sampleRate = spec.sampleRate;
		data->numChannels = spec.numChannels;
		createFX(data->fxType);
	}

	void FXChooser::reset()
	{
		if (fx != nullptr) {
			fx->reset();
		}
	}

	bool FXChooser::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{
		if (data->bypassed || context.isBypassed) {
			return false;
		}
		if (data->fxType != fx->mode) {
			createFX(data->fxType);
		}
		return fx->process(context, workBuffers);
	}

	void FXChooser::noteOn() {
		Processor::noteOn();
		if (fx) {
			fx->noteOn();
		}
	}

	void FXChooser::noteOff() {
		Processor::noteOff();
		if (fx) {
			fx->noteOff();
		}
	}

	void Reverb::prepareUpdate()
	{
		// SampleRate
		static const short combTunings[] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }; // (at 44100Hz)
		static const short allPassTunings[] = { 556, 441, 341, 225 };
		const int stereoSpread = 23;
		const int intSampleRate = static_cast<int>(data->sampleRate);

		leftCombBufferSize = 0;
		rightCombBufferSize = 0;
		for (int i = 0; i < numCombs; i++) {
			auto lSize = (intSampleRate * combTunings[i]) / 44100;
			combData[LEFT][i].update(static_cast<int>(leftCombBufferSize), static_cast<int>(lSize));
			leftCombBufferSize += lSize;

			auto rSize = (intSampleRate * (combTunings[i] + stereoSpread)) / 44100;
			combData[RIGHT][i].update(static_cast<int>(rightCombBufferSize), static_cast<int>(rSize));
			rightCombBufferSize += rSize;
		}
		combBuffer[LEFT].allocate(leftCombBufferSize, true);
		combBuffer[RIGHT].allocate(rightCombBufferSize, true);

		leftAllPassBufferSize = 0;
		rightAllPassBufferSize = 0;
		for (int i = 0; i < numAllPasses; ++i)
		{
			auto lSize = (intSampleRate * allPassTunings[i]) / 44100;
			allPassData[LEFT][i].update(static_cast<int>(leftAllPassBufferSize), static_cast<int>(lSize));
			leftAllPassBufferSize += lSize;

			auto rSize = (intSampleRate * (allPassTunings[i] + stereoSpread)) / 44100;
			allPassData[RIGHT][i].update(static_cast<int>(rightAllPassBufferSize), static_cast<int>(rSize));
			rightAllPassBufferSize += rSize;
		}
		allPassBuffer[LEFT].allocate(leftAllPassBufferSize, true);
		allPassBuffer[RIGHT].allocate(rightAllPassBufferSize, true);

		// Here was SmoothedValue stuff
	}
	void Reverb::reset() {
		combBuffer[LEFT].clear(leftCombBufferSize);
		combBuffer[RIGHT].clear(rightCombBufferSize);

		allPassBuffer[LEFT].clear(leftAllPassBufferSize);
		allPassBuffer[RIGHT].clear(rightAllPassBufferSize);
	}

	bool Reverb::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{
		if (data->bypassed || context.isBypassed) {
			return false;
		}

		auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
		auto& outputBlock = context.getOutputBlock();
		const auto numSamples = outputBlock.getNumSamples();

		jassert(workBuffers.getNumChannels() >= 2);

		// prepare for modulation
		auto dryWetBase = data->dryWet;
		auto dryWetMod = data->modParams[FXChooser::SharedData::DRY_WET].factor;
		auto dryWetModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::DRY_WET].src_channel);

		// parameter2 = width
		auto widthBase = data->parameter2;
		auto widthMod = data->modParams[FXChooser::SharedData::PARAMETER_2].factor;
		auto widthModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_2].src_channel);

		// parameter0 = roomSize
		// parameter1 = damping
		auto roomSizeBase = data->parameter0;
		auto roomSizeMod = data->modParams[FXChooser::SharedData::PARAMETER_0].factor;
		auto roomSizeModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_0].src_channel);

		auto dampingBase = data->parameter1;
		auto dampingMod = data->modParams[FXChooser::SharedData::PARAMETER_1].factor;
		auto dampingModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_1].src_channel);

		const float roomScaleFactor = 0.28f;
		const float roomOffset = 0.7f;
		const float dampScaleFactor = 1.f;

		jassert(data->numChannels == 2);
		jassert(outputBlock.getNumChannels() == 2);

		// Calculations
		auto outWB = workBuffers.getSingleChannelBlock(0);
		auto* outWBPtr = outWB.getChannelPointer(0);
		auto mixedInput = workBuffers.getSingleChannelBlock(1);
		auto* mixedInputPtr = mixedInput.getChannelPointer(0);
		mixedInput.replaceWithSumOf(outputBlock.getSingleChannelBlock(LEFT), outputBlock.getSingleChannelBlock(RIGHT));

		const float gain = 0.025f;// 0.015f;
		mixedInput.multiplyBy(gain);

		for (int blockStart = 0; blockStart < numSamples; blockStart += configuration::MOD_BLOCK_SIZE) {
			auto end = juce::jmin(blockStart + configuration::MOD_BLOCK_SIZE, (int)numSamples);
			auto length = end - blockStart;

			// modulation update
			auto currentWidth = juce::jlimit(0.f, 1.f, widthBase + widthMod * widthModSrc[blockStart]);
			auto currentFeedback = juce::jlimit(0.f, 1.f, roomSizeBase + roomSizeMod * roomSizeModSrc[blockStart]) * roomScaleFactor + roomOffset;
			auto currentDamping = juce::jlimit(0.f, 1.f, dampingBase + dampingMod * dampingModSrc[blockStart]) * dampScaleFactor;
			auto currentDryWet = juce::jlimit(0.f, 1.f, dryWetBase + dryWetMod * dryWetModSrc[blockStart]);

			const float wetScaleFactor = 2.f;// 3.0f;

			const float wet = currentDryWet * wetScaleFactor;
			const float dryGain = (1.f - currentDryWet);
			const float wetGain1 = 0.5f * wet * (1.f + currentWidth);
			const float wetGain2 = 0.5f * wet * (1.f - currentWidth);

			outputBlock.getSubBlock(blockStart, length).multiplyBy(dryGain);
			// Do calculations first for the left and then for the right side to increase data locality
			// and decrease the needed work buffers
			for (int n = LEFT; n <= RIGHT; n++) {
				auto currentOutWB = outWB.getSubBlock(blockStart, length);

				currentOutWB.clear();
				// accumulate comb filter in parallel
				for (int c = 0; c < numCombs; c++) {

					float output;
					auto* buffer = combBuffer[n].get();
					buffer += combData[n][c].startPos;
					auto bufferSize = combData[n][c].size;

					auto bufferIndex = combData[n][c].index;
					auto last = combData[n][c].last;

					// comb filter calculations
					for (int i = blockStart; i < end; i++) {
						output = buffer[bufferIndex];
						outWBPtr[i] += output;
						last = output + (last - output) * currentDamping;
						buffer[bufferIndex] = mixedInputPtr[i] + last * currentFeedback;
						bufferIndex = (bufferIndex + 1) % bufferSize;
					}

					combData[n][c].index = bufferIndex;
					combData[n][c].last = last;
				}

				// apply allpass in series
				for (int a = 0; a < numAllPasses; a++) {
					float bufferedValue, outValue;
					auto* buffer = allPassBuffer[n].get();
					buffer += allPassData[n][a].startPos;
					auto bufferSize = allPassData[n][a].size;

					auto bufferIndex = allPassData[n][a].index;

					// allpass filter calculations
					for (int i = blockStart; i < end; i++) {
						bufferedValue = buffer[bufferIndex];
						outValue = outWBPtr[i];
						buffer[bufferIndex] = outValue + bufferedValue * 0.5f;
						outWBPtr[i] = bufferedValue - outValue;
						bufferIndex = (bufferIndex + 1) % bufferSize;
					}

					allPassData[n][a].index = bufferIndex;
				}
				// mix down

				outputBlock.getSingleChannelBlock(n).getSubBlock(blockStart, length).addProductOf(currentOutWB, wetGain1);
				outputBlock.getSingleChannelBlock((n + 1) % 2).getSubBlock(blockStart, length).addProductOf(currentOutWB, wetGain2);
			}
		}

		return true;
	}

	void Delay::prepareUpdate()
	{
		for (auto& d : delays) {
			d.setMaxDelay(static_cast<int>(MAX_DELAY_SEC * data->sampleRate));
		}
		reset();
	}

	void Delay::reset()
	{
		for (auto& d : delays) {
			d.reset();
		}
	}

	bool Delay::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{
		juce::ignoreUnused(workBuffers);
		if (data->bypassed || context.isBypassed) {
			return false;
		}

		auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
		auto& outputBlock = context.getOutputBlock();
		const auto numSamples = outputBlock.getNumSamples();

		//jassert(workBuffers.getNumChannels() >= 1);
		jassert(data->numChannels == 2);
		jassert(outputBlock.getNumChannels() == 2);

		// prepare modulation
		auto dryWetBase = data->dryWet;
		auto dryWetMod = data->modParams[FXChooser::SharedData::DRY_WET].factor;
		auto dryWetModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::DRY_WET].src_channel);
		// parameter0 = damp
		auto dampBase = data->parameter0;
		auto dampMod = data->modParams[FXChooser::SharedData::PARAMETER_0].factor;
		auto dampModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_0].src_channel);
		// parameter1 = timeLeft
		auto timeLeftBase = data->parameter1;
		auto timeLeftMod = data->modParams[FXChooser::SharedData::PARAMETER_1].factor;
		auto timeLeftModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_1].src_channel);
		// parameter0 = timeRight
		auto timeRightBase = data->parameter2;
		auto timeRightMod = data->modParams[FXChooser::SharedData::PARAMETER_2].factor;
		auto timeRightModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_2].src_channel);

		juce::SmoothedValue<float> delayInSamples[2];
		delayInSamples[LEFT].setCurrentAndTargetValue(
			juce::jmap(juce::jlimit(0.f, 1.f, timeLeftBase + timeLeftMod * timeLeftModSrc[0]),
				2.f, MAX_DELAY_SEC * static_cast<float>(data->sampleRate)));
		delayInSamples[RIGHT].setCurrentAndTargetValue(
			juce::jmap(juce::jlimit(0.f, 1.f, timeRightBase + timeRightMod * timeRightModSrc[0]),
				2.f, MAX_DELAY_SEC * static_cast<float>(data->sampleRate)));

		for (int blockStart = 0; blockStart < numSamples; blockStart += configuration::MOD_BLOCK_SIZE) {
			auto end = juce::jmin(blockStart + configuration::MOD_BLOCK_SIZE, (int)numSamples);
			auto length = end - blockStart;

			auto dryWet = juce::jlimit(0.f, 1.f, dryWetBase + dryWetMod * dryWetModSrc[blockStart]);
			auto damp = juce::jlimit(0.f, 1.f, dampBase + dampMod * dampModSrc[blockStart]);

			delayInSamples[LEFT].setTargetValue(
				juce::jmap(juce::jlimit(0.f, 1.f, timeLeftBase + timeLeftMod * timeLeftModSrc[end-1]),
					2.f, MAX_DELAY_SEC * static_cast<float>(data->sampleRate)));
			delayInSamples[RIGHT].setTargetValue(
				juce::jmap(juce::jlimit(0.f, 1.f, timeRightBase + timeRightMod * timeRightModSrc[end-1]),
					2.f, MAX_DELAY_SEC * static_cast<float>(data->sampleRate)));
			delayInSamples[LEFT].reset(length);
			delayInSamples[RIGHT].reset(length);


			for (int channel = LEFT; channel <= RIGHT; channel++) {
				auto& delay = delays[channel];
				auto outputChannel = outputBlock.getChannelPointer(channel);
				for (int i = blockStart; i < end; i++) {
					outputChannel[i] += dryWet * ((1.f - damp) * delay.process(outputChannel[i], delayInSamples[channel].getNextValue(), 0.f)
						- outputChannel[i]);
				}
			}
		}

		if (isNoteOn) {
			return true;
		}
		else {
			return !delays[LEFT].isEmpty() || !delays[RIGHT].isEmpty();
		}
	}

	void Flanger::prepareUpdate()
	{
		for (auto& d : delays) {
			d.setMaxDelay(static_cast<int>(1 + MAX_DELAY_MSEC * data->sampleRate / 1000.f));
		}
		lfoData.sampleRate = data->sampleRate;
		lfoData.wt = wavetable::WavetableCache::getInstance()->getWavetable(0);
		reset();
	}

	void Flanger::reset()
	{
		for (auto& d : delays) {
			d.reset();
		}
		for (auto& l : lfos) {
			l.reset();
		}
		lfos[RIGHT].advancePhase(juce::MathConstants<float>::halfPi);
		for (auto& s : delaySmoothed) {
			s.setCurrentAndTargetValue(1);
		}
		for (auto& s : feedbackSmoothed) {
			s.setCurrentAndTargetValue(0);
		}

	}

	bool Flanger::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{
		if (data->bypassed || context.isBypassed) {
			return false;
		}

		auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
		auto& outputBlock = context.getOutputBlock();
		const auto numSamples = outputBlock.getNumSamples();

		jassert(workBuffers.getNumChannels() >= 1);
		jassert(data->numChannels == 2);
		jassert(outputBlock.getNumChannels() == 2);

		// prepare modulation
		auto dryWetBase = data->dryWet;
		auto dryWetMod = data->modParams[FXChooser::SharedData::DRY_WET].factor;
		auto dryWetModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::DRY_WET].src_channel);

		// parameter0 = rate
		auto rateBase = data->parameter0;
		auto rateMod = data->modParams[FXChooser::SharedData::PARAMETER_0].factor;
		auto rateModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_0].src_channel);
		// parameter1 = depth
		auto depthBase = data->parameter1;
		auto depthMod = data->modParams[FXChooser::SharedData::PARAMETER_1].factor;
		auto depthModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_1].src_channel);
		// parameter2 = feedback
		auto feedbackBase = data->parameter2;
		auto feedbackMod = data->modParams[FXChooser::SharedData::PARAMETER_2].factor;
		auto feedbackModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_2].src_channel);

		auto emptyBlock = juce::dsp::AudioBlock<float>();

		for (int blockStart = 0; blockStart < numSamples; blockStart += configuration::MOD_BLOCK_SIZE) {
			auto end = juce::jmin(blockStart + configuration::MOD_BLOCK_SIZE, (int)numSamples);
			auto length = end - blockStart;

			auto dryWet = juce::jlimit(0.f, 1.f, dryWetBase + dryWetMod * dryWetModSrc[blockStart]);
			auto normalizedRate = juce::jlimit(0.f, 1.f, rateBase + rateMod * rateModSrc[blockStart]);
			auto depth = juce::jlimit(0.f, 1.f, depthBase + depthMod * depthModSrc[blockStart]);
			auto normalizedFeedback = juce::jlimit(0.f, 1.f, feedbackBase + feedbackMod * feedbackModSrc[blockStart]);

			lfoData.rate = std::powf(3, 2 * normalizedRate) - 1;
			auto feedback = juce::jmap(normalizedFeedback, -0.95f, 0.95f);
			float maxDelay = juce::jmax(depth * MAX_DELAY_MSEC * static_cast<float>(data->sampleRate) / 1000.f, MIN_DELAY_SAMPLES); // in samples

			auto lfoValues = workBuffers.getSingleChannelBlock(0).getSubBlock(0, length);
			auto lfoContext = juce::dsp::ProcessContextNonReplacing<float>(emptyBlock, lfoValues);
			auto lfoWorkBuffer = workBuffers.getSingleChannelBlock(1).getSubBlock(0, length);

			for (int channel = LEFT; channel <= RIGHT; channel++) {
				lfoValues.clear();
				lfos[channel].process(lfoContext, lfoWorkBuffer);

				delaySmoothed[channel].reset(length);
				feedbackSmoothed[channel].reset(length);

				delaySmoothed[channel].setTargetValue(MIN_DELAY_SAMPLES + (maxDelay - MIN_DELAY_SAMPLES)
					* (lfoValues.getSample(0, length - 1) + 1.f) / 2.f);
				feedbackSmoothed[channel].setTargetValue(feedback);

				auto outputChannel = outputBlock.getChannelPointer(channel);
				auto& delay = delays[channel];
				for (int i = blockStart; i < end; i++) {
					outputChannel[i] +=
						dryWet * (delay.process(outputChannel[i], delaySmoothed[channel].getNextValue(), feedbackSmoothed[channel].getNextValue())
							- outputChannel[i]);
				}
			}
		}

		return isNoteOn;
	}

	void Chorus::prepareUpdate()
	{
		for (auto& d : delays) {
			d.setMaxDelay(static_cast<int>(1 + MAX_DELAY_MSEC * data->sampleRate / 1000.f));
		}
		lfoData.sampleRate = data->sampleRate;
		lfoData.wt = wavetable::WavetableCache::getInstance()->getWavetable(0);
		reset();
	}

	void Chorus::reset()
	{
		for (auto& d : delays) {
			d.reset();
		}
		for (auto& l : lfos) {
			l.reset();
		}
		lfos[RIGHT].advancePhase(juce::MathConstants<float>::halfPi);
		for (auto& s : centreDelaySmoothed) {
			s.setCurrentAndTargetValue(static_cast<float>(((MAX_DELAY_MSEC + MIN_DELAY_MSEC) / 2.f) * data->sampleRate / 1000.f));
		}
		for (auto& s : delaySmoothed) {
			s.setCurrentAndTargetValue(static_cast<float>(((MAX_DELAY_MSEC + MIN_DELAY_MSEC) / 2.f) * data->sampleRate / 1000.f));
		}
	}

	bool Chorus::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
	{
		if (data->bypassed || context.isBypassed) {
			return false;
		}

		auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
		auto& outputBlock = context.getOutputBlock();
		const auto numSamples = outputBlock.getNumSamples();

		jassert(workBuffers.getNumChannels() >= 1);
		jassert(data->numChannels == 2);
		jassert(outputBlock.getNumChannels() == 2);

		// prepare modulation
		auto dryWetBase = data->dryWet;
		auto dryWetMod = data->modParams[FXChooser::SharedData::DRY_WET].factor;
		auto dryWetModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::DRY_WET].src_channel);

		// parameter0 = rate
		auto rateBase = data->parameter0;
		auto rateMod = data->modParams[FXChooser::SharedData::PARAMETER_0].factor;
		auto rateModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_0].src_channel);
		// parameter1 = depth
		auto depthBase = data->parameter1;
		auto depthMod = data->modParams[FXChooser::SharedData::PARAMETER_1].factor;
		auto depthModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_1].src_channel);
		// parameter2 = centreDelay
		auto centreDelayBase = data->parameter2;
		auto centreDelayMod = data->modParams[FXChooser::SharedData::PARAMETER_2].factor;
		auto centreDelayModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_2].src_channel);

		auto emptyBlock = juce::dsp::AudioBlock<float>();

		const auto minDelaySamples = MIN_DELAY_MSEC * static_cast<float>(data->sampleRate) / 1000.f;
		const auto maxDelaySamples = MAX_DELAY_MSEC * static_cast<float>(data->sampleRate) / 1000.f;
		const auto delayRange = maxDelaySamples - minDelaySamples;

		for (int blockStart = 0; blockStart < numSamples; blockStart += configuration::MOD_BLOCK_SIZE) {
			auto end = juce::jmin(blockStart + configuration::MOD_BLOCK_SIZE, (int)numSamples);
			auto length = end - blockStart;

			auto dryWet = juce::jlimit(0.f, 1.f, dryWetBase + dryWetMod * dryWetModSrc[blockStart]);
			auto normalizedRate = juce::jlimit(0.f, 1.f, rateBase + rateMod * rateModSrc[blockStart]);
			auto normalizedDepth = juce::jlimit(0.f, 1.f, depthBase + depthMod * depthModSrc[blockStart]);
			auto normalizedCentreDelay = juce::jlimit(0.f, 1.f, centreDelayBase + centreDelayMod * centreDelayModSrc[blockStart]);

			lfoData.rate = std::powf(3, 2 * normalizedRate) - 1;
			auto centreDelay = minDelaySamples + normalizedCentreDelay * delayRange;

			auto lfoValues = workBuffers.getSingleChannelBlock(0).getSubBlock(0, length);
			auto lfoContext = juce::dsp::ProcessContextNonReplacing<float>(emptyBlock, lfoValues);
			auto lfoWorkBuffer = workBuffers.getSingleChannelBlock(1).getSubBlock(0, length);

			for (int channel = LEFT; channel <= RIGHT; channel++) {
				lfoValues.clear();
				lfos[channel].process(lfoContext, lfoWorkBuffer);

				centreDelaySmoothed[channel].reset(length);
				delaySmoothed[channel].reset(length);

				centreDelaySmoothed[channel].setTargetValue(centreDelay);
				delaySmoothed[channel].setTargetValue(juce::jlimit(minDelaySamples - centreDelay, maxDelaySamples - centreDelay,
					normalizedDepth * (delayRange / 2.f) * lfoValues.getSample(0, length - 1)));

				auto outputChannel = outputBlock.getChannelPointer(channel);
				auto& delay = delays[channel];
				for (int i = blockStart; i < end; i++) {
					auto currentDelay = juce::jlimit(minDelaySamples, maxDelaySamples,
						centreDelaySmoothed[channel].getNextValue() + delaySmoothed[channel].getNextValue());
					outputChannel[i] +=
						dryWet * (delay.process(outputChannel[i], currentDelay, 0.f) - outputChannel[i]);
				}
			}
		}

		return isNoteOn;
	}


	void Phaser::prepareUpdate() {
		lfoData.sampleRate = data->sampleRate;
		lfoData.wt = wavetable::WavetableCache::getInstance()->getWavetable(0);
		reset();
	}
	void Phaser::reset() {
		for (auto& l : lfos) {
			l.reset();
		}
		lfos[RIGHT].advancePhase(juce::MathConstants<float>::halfPi);
		for (int i = 0; i < STAGES; i++) {
			allpassS1[LEFT][i] = 0.f;
			allpassS1[RIGHT][i] = 0.f;
		}
		last[LEFT] = 0.f;
		last[RIGHT] = 0.f;
	}

	bool Phaser::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) {

		if (data->bypassed || context.isBypassed) {
			return false;
		}

		auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
		auto& outputBlock = context.getOutputBlock();
		const auto numSamples = outputBlock.getNumSamples();

		//jassert(workBuffers.getNumChannels() >= 1);
		jassert(data->numChannels == 2);
		jassert(outputBlock.getNumChannels() == 2);

		// prepare modulation
		auto dryWetBase = data->dryWet;
		auto dryWetMod = data->modParams[FXChooser::SharedData::DRY_WET].factor;
		auto dryWetModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::DRY_WET].src_channel);

		// parameter0 = rate
		auto rateBase = data->parameter0;
		auto rateMod = data->modParams[FXChooser::SharedData::PARAMETER_0].factor;
		auto rateModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_0].src_channel);
		// parameter1 = depth
		auto depthBase = data->parameter1;
		auto depthMod = data->modParams[FXChooser::SharedData::PARAMETER_1].factor;
		auto depthModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_1].src_channel);
		// parameter2 = feedback
		auto feedbackBase = data->parameter2;
		auto feedbackMod = data->modParams[FXChooser::SharedData::PARAMETER_2].factor;
		auto feedbackModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_2].src_channel);

		auto emptyBlock = juce::dsp::AudioBlock<float>();

		for (int blockStart = 0; blockStart < numSamples; blockStart += configuration::MOD_BLOCK_SIZE) {
			auto end = juce::jmin(blockStart + configuration::MOD_BLOCK_SIZE, (int)numSamples);
			auto length = end - blockStart;

			auto dryWet = juce::jlimit(0.f, 1.f, dryWetBase + dryWetMod * dryWetModSrc[blockStart]);
			auto normalizedRate = juce::jlimit(0.f, 1.f, rateBase + rateMod * rateModSrc[blockStart]);
			auto normalizedDepth = juce::jlimit(0.f, 1.f, depthBase + depthMod * depthModSrc[blockStart]);
			auto feedback = juce::jlimit(0.f, 0.9f, feedbackBase + feedbackMod * feedbackModSrc[blockStart]);

			lfoData.rate = std::powf(3, 2 * normalizedRate) - 1;

			auto lfoValues = workBuffers.getSingleChannelBlock(0).getSubBlock(0, length);
			auto lfoContext = juce::dsp::ProcessContextNonReplacing<float>(emptyBlock, lfoValues);
			auto lfoWorkBuffer = workBuffers.getSingleChannelBlock(1).getSubBlock(0, length);

			// coefficients for allpass calculation
			float G[STAGES];

			for (int channel = LEFT; channel <= RIGHT; channel++) {
				lfoValues.clear();
				lfos[channel].process(lfoContext, lfoWorkBuffer);
				auto normalizedLfo = normalizedDepth * (lfoValues.getSample(0, length - 1) + 1.f) / 2.f;
				for (int i = 0; i < STAGES; i++) {
					auto cutoff = minCutoffs[i] * std::powf(maxCutoffs[i] / minCutoffs[i], normalizedLfo);
					auto g = juce::dsp::FastMathApproximations::tan<float>(static_cast<float>(juce::MathConstants<float>::pi * cutoff / data->sampleRate));
					G[i] = g / (g + 1);
				}
				auto outputChannel = outputBlock.getChannelPointer(channel);
				for (int i = blockStart; i < end; i++) {
					auto input = outputChannel[i] + feedback * last[channel];
					// allpass calculations in series
					// for reference see FirstOrderTptFilter from JUCE or directly Vadim Zavalishin's 
					// literature on TPT filter design
					for (int s = 0; s < STAGES; s++) {
						auto v = G[s] * (input - allpassS1[channel][s]);
						auto y = v + allpassS1[channel][s];
						allpassS1[channel][s] = y + v;
						input = 2 * y - input;
					}
					last[channel] = input;

					outputChannel[i] += dryWet * (input - outputChannel[i]);
				}
			}
		}

		return isNoteOn;
	}

	void Distortion::prepareUpdate() {
	}
	void Distortion::reset() {
		filterS1[0] = 0.f;
		filterS1[1] = 0.f;
	}

	bool Distortion::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) {
		juce::ignoreUnused(workBuffers);
		if (data->bypassed || context.isBypassed) {
			return false;
		}

		auto& inputBlock = context.getInputBlock(); // holds envelopes and lfos for modulation
		auto& outputBlock = context.getOutputBlock();
		const auto numSamples = outputBlock.getNumSamples();

		//jassert(workBuffers.getNumChannels() >= 1);
		jassert(data->numChannels == 2);
		jassert(outputBlock.getNumChannels() == 2);

		// prepare modulation
		auto dryWetBase = data->dryWet;
		auto dryWetMod = data->modParams[FXChooser::SharedData::DRY_WET].factor;
		auto dryWetModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::DRY_WET].src_channel);

		// parameter0 = cutoff
		auto cutoffBase = data->parameter0;
		auto cutoffMod = data->modParams[FXChooser::SharedData::PARAMETER_0].factor;
		auto cutoffModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_0].src_channel);
		// parameter1 = filterPos
		auto filterPosBase = data->parameter1;
		auto filterPosMod = data->modParams[FXChooser::SharedData::PARAMETER_1].factor;
		auto filterPosModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_1].src_channel);
		// parameter2 = steepness
		auto kBase = data->parameter2;
		auto kMod = data->modParams[FXChooser::SharedData::PARAMETER_2].factor;
		auto kModSrc = inputBlock.getChannelPointer((size_t)data->modParams[FXChooser::SharedData::PARAMETER_2].src_channel);

		auto emptyBlock = juce::dsp::AudioBlock<float>();

		auto minMax = outputBlock.findMinAndMax();
		auto originalMagnitude = juce::jmax(std::abs(minMax.getStart()), std::abs(minMax.getEnd()));
		if (originalMagnitude != 0.f) {
			outputBlock.multiplyBy(1.f / originalMagnitude);
		}
		else {
			return false;
		}

		for (int blockStart = 0; blockStart < numSamples; blockStart += configuration::MOD_BLOCK_SIZE) {
			auto end = juce::jmin(blockStart + configuration::MOD_BLOCK_SIZE, (int)numSamples);

			auto dryWet = juce::jlimit(0.f, 1.f, dryWetBase + dryWetMod * dryWetModSrc[blockStart]);
			auto normalizedCutoff = juce::jlimit(0.f, 1.f, cutoffBase + cutoffMod * cutoffModSrc[blockStart]);
			auto filterPos = juce::jlimit(0.f, 1.f, filterPosBase + filterPosMod * filterPosModSrc[blockStart]);
			float k = 1.f + juce::jlimit(0.f, 1.f, kBase + kMod * kModSrc[blockStart]) * 4.f;
			auto feedback = juce::jlimit(0.f, .85f, kBase + kMod * kModSrc[blockStart]);

			auto cutoff = 20.f * std::powf(20000.f / 20.f, normalizedCutoff);
			auto g = juce::dsp::FastMathApproximations::tan<float>(juce::MathConstants<float>::pi * cutoff / static_cast<float>(data->sampleRate));
			auto G = g / (g + 1);

			for (int channel = 0; channel < outputBlock.getNumChannels(); channel++) {

				auto outputChannel = outputBlock.getChannelPointer(channel);
				for (int i = blockStart; i < end; i++) {
					auto input = outputChannel[i] + feedback * last[channel];
					auto v = G * (input - filterS1[channel]);
					auto y = v + filterS1[channel];
					filterS1[channel] = y + v;

					//lowpass = y, highpass = input - y
					// interpolate between low and highpass to get the signal that should be distorted
					auto variableSignal = y + filterPos * (input - 2 * y);
					auto fixedSignal = input - variableSignal;

					input = juce::dsp::FastMathApproximations::tanh(k * variableSignal) + fixedSignal;
					last[channel] = input;
					outputChannel[i] += dryWet * (input - outputChannel[i]);

				}
			}
		}
		minMax = outputBlock.findMinAndMax();
		auto newMagnitude = juce::jmax(std::abs(minMax.getStart()), std::abs(minMax.getEnd()));
		jassert(newMagnitude != 0.f);
		outputBlock.multiplyBy(originalMagnitude / newMagnitude);

		return isNoteOn;
	}
	void DelayLine::setMaxDelay(int maxDelay) {
		buffer.allocate(maxDelay, true);
		bufferSize = maxDelay;
		index = 0;
	}

	void DelayLine::reset() {
		buffer.clear(bufferSize);
	}

	float DelayLine::process(float sample, float delay, float feedback) {
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

	bool DelayLine::isEmpty() {
		for (int i = 0; i < bufferSize; i++) {
			if (buffer[i] != 0.f) {
				return false;
			}
		}
		return true;
	}

}