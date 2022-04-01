
#include "FX.h"

void customDsp::FXChooser::createFX(FXType type)
{
	switch (type) {
	case FXType::CHORUS: fx = std::make_unique<Chorus>(data); break;
	case FXType::DELAY: fx = std::make_unique<Delay>(data); break;
	case FXType::FLANGER: fx = std::make_unique<Flanger>(data); break;
	case FXType::TUBE: fx = std::make_unique<DummyFX>(data); break;
	case FXType::REVERB: fx = std::make_unique<Reverb>(data); break;
	case FXType::PHASER: fx = std::make_unique<Phaser>(data); break;
	case FXType::NONE: fx = std::make_unique<DummyFX>(data); break;
	default: jassertfalse; break;
	}

	fx->prepareUpdate();
	if (isNoteOn) {
		fx->noteOn();
	}
}

void customDsp::FXChooser::prepare(const juce::dsp::ProcessSpec& spec)
{
	data->sampleRate = spec.sampleRate;
	data->numChannels = spec.numChannels;
	createFX(data->fxType);
}

void customDsp::FXChooser::reset()
{
	if (fx != nullptr) {
		fx->reset();
	}
}

bool customDsp::FXChooser::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
{
	if (data->bypassed || context.isBypassed) {
		return false;
	}
	if (data->fxType != fx->mode) {
		createFX(data->fxType);
	}
	return fx->process(context, workBuffers);
}

void customDsp::FXChooser::noteOn() {
	Processor::noteOn();
	if (fx) {
		fx->noteOn();
	}
}

void customDsp::FXChooser::noteOff() {
	Processor::noteOff();
	if (fx) {
		fx->noteOff();
	}
}

void customDsp::Reverb::prepareUpdate()
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
void customDsp::Reverb::reset() {
	combBuffer[LEFT].clear(leftCombBufferSize);
	combBuffer[RIGHT].clear(rightCombBufferSize);

	allPassBuffer[LEFT].clear(leftAllPassBufferSize);
	allPassBuffer[RIGHT].clear(rightAllPassBufferSize);
}

bool customDsp::Reverb::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
{
	if (data->bypassed || context.isBypassed) {
		return false;
	}
	//JUCE_BEGIN_IGNORE_WARNINGS_MSVC(6011)

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
			outputBlock.getSubBlock(blockStart, length).multiplyBy(dryGain);
			outputBlock.getSingleChannelBlock(n).getSubBlock(blockStart, length).addProductOf(currentOutWB, wetGain1);
			outputBlock.getSingleChannelBlock((n + 1) % 2).getSubBlock(blockStart, length).addProductOf(currentOutWB, wetGain2);
		}
	}
	//JUCE_END_IGNORE_WARNINGS_MSVC

	return (MIN_LEVEL < outputBlock.getSample(LEFT, 0)) || (MIN_LEVEL < outputBlock.getSample(LEFT, static_cast<int>(numSamples - 1)));
}

void customDsp::Delay::prepareUpdate()
{
	for (auto& d : delays) {
		d.setMaxDelay(maxDelaySec * data->sampleRate);
	}
	reset();

}

void customDsp::Delay::reset()
{
	for (auto& d : delays) {
		d.reset();
	}

}

bool customDsp::Delay::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
{
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

	for (int blockStart = 0; blockStart < numSamples; blockStart += configuration::MOD_BLOCK_SIZE) {
		auto end = juce::jmin(blockStart + configuration::MOD_BLOCK_SIZE, (int)numSamples);
		auto length = end - blockStart;

		auto dryWet = dryWetBase + dryWetMod * dryWetModSrc[blockStart];
		auto damp = dampBase + dampMod * dampModSrc[blockStart];

		float delayInSamples[2]{
			juce::jmap(juce::jlimit(0.f,1.f,timeLeftBase + timeLeftMod * timeLeftModSrc[blockStart]),
					2.f, maxDelaySec * static_cast<float>(data->sampleRate)),
			juce::jmap(juce::jlimit(0.f,1.f,timeRightBase + timeRightMod * timeRightModSrc[blockStart]),
					2.f, maxDelaySec * static_cast<float>(data->sampleRate)),
		};
		//delaySmoothed[LEFT].reset(length);
		//delaySmoothed[RIGHT].reset(length);
		//delaySmoothed[LEFT].setTargetValue(delayInSamples[LEFT]);
		//delaySmoothed[RIGHT].setTargetValue(delayInSamples[RIGHT]);

		for (int channel = LEFT; channel <= RIGHT; channel++) {
			auto& delay = delays[channel];
			auto outputChannel = outputBlock.getChannelPointer(channel);
			for (int i = blockStart; i < end; i++) {
				outputChannel[i] += dryWet * ((1.f - damp) * delay.process(outputChannel[i], delayInSamples[channel], 0.f) - outputChannel[i]);
			}
		}
	}

	return true; // TODO
}

void customDsp::Flanger::prepareUpdate()
{
	for (auto& d : delays) {
		d.setMaxDelay(1 + maxDelayMSec * data->sampleRate / 1000.f);
	}
	lfoData.sampleRate = data->sampleRate;
	lfoData.wt = wavetable::WavetableCache::getInstance()->getWavetable(0);
	reset();
}

void customDsp::Flanger::reset()
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

bool customDsp::Flanger::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
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

		auto dryWet = dryWetBase + dryWetMod * dryWetModSrc[blockStart];
		auto normalizedRate = juce::jlimit(0.f, 1.f, rateBase + rateMod * rateModSrc[blockStart]);
		auto depth = juce::jlimit(0.f, 1.f, depthBase + depthMod * depthModSrc[blockStart]);
		auto normalizedFeedback = juce::jlimit(0.f, 1.f, feedbackBase + feedbackMod * feedbackModSrc[blockStart]);

		lfoData.rate = std::powf(3, 2 * normalizedRate) - 1;
		auto feedback = juce::jmap(normalizedFeedback, -0.95f, 0.95f);
		float maxDelay = juce::jmax(depth * maxDelayMSec * static_cast<float>(data->sampleRate) / 1000.f, minDelaySamples); // in samples

		auto lfoValues = workBuffers.getSingleChannelBlock(0).getSubBlock(0, length);
		auto lfoContext = juce::dsp::ProcessContextNonReplacing<float>(emptyBlock, lfoValues);

		for (int channel = LEFT; channel <= RIGHT; channel++) {
			lfoValues.clear();
			lfos[channel].process(lfoContext, emptyBlock);

			delaySmoothed[channel].reset(length);
			feedbackSmoothed[channel].reset(length);

			delaySmoothed[channel].setTargetValue(minDelaySamples + (maxDelay - minDelaySamples)
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

	return true; // TODO
}

void customDsp::Chorus::prepareUpdate()
{
	for (auto& d : delays) {
		d.setMaxDelay(1 + maxDelayMSec * data->sampleRate / 1000.f);
	}
	lfoData.sampleRate = data->sampleRate;
	lfoData.wt = wavetable::WavetableCache::getInstance()->getWavetable(0);
	reset();
}

void customDsp::Chorus::reset()
{
	for (auto& d : delays) {
		d.reset();
	}
	for (auto& l : lfos) {
		l.reset();
	}
	lfos[RIGHT].advancePhase(juce::MathConstants<float>::halfPi);
	for (auto& s : delaySmoothed) {
		s.setCurrentAndTargetValue(((maxDelayMSec + minDelayMSec) / 2.f) * data->sampleRate / 1000.f);
	}
}

bool customDsp::Chorus::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers)
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

	const auto minDelaySamples = minDelayMSec * static_cast<float>(data->sampleRate) / 1000.f;
	const auto maxDelaySamples = maxDelayMSec * static_cast<float>(data->sampleRate) / 1000.f;
	const auto delayRange = maxDelaySamples - minDelaySamples;

	for (int blockStart = 0; blockStart < numSamples; blockStart += configuration::MOD_BLOCK_SIZE) {
		auto end = juce::jmin(blockStart + configuration::MOD_BLOCK_SIZE, (int)numSamples);
		auto length = end - blockStart;

		auto dryWet = dryWetBase + dryWetMod * dryWetModSrc[blockStart];
		auto normalizedRate = juce::jlimit(0.f, 1.f, rateBase + rateMod * rateModSrc[blockStart]);
		auto normalizedDepth = juce::jlimit(0.f, 1.f, depthBase + depthMod * depthModSrc[blockStart]);
		auto normalizedCentreDelay = juce::jlimit(0.f, 1.f, centreDelayBase + centreDelayMod * centreDelayModSrc[blockStart]);

		lfoData.rate = std::powf(3, 2 * normalizedRate) - 1;
		auto centreDelay = minDelaySamples + normalizedCentreDelay * delayRange;

		auto lfoValues = workBuffers.getSingleChannelBlock(0).getSubBlock(0, length);
		auto lfoContext = juce::dsp::ProcessContextNonReplacing<float>(emptyBlock, lfoValues);

		for (int channel = LEFT; channel <= RIGHT; channel++) {
			lfoValues.clear();
			lfos[channel].process(lfoContext, emptyBlock);

			delaySmoothed[channel].reset(length);

			delaySmoothed[channel].setTargetValue(juce::jlimit(minDelaySamples, maxDelaySamples,
				centreDelay + normalizedDepth * (delayRange / 2.f) * lfoValues.getSample(0, length - 1)));

			auto outputChannel = outputBlock.getChannelPointer(channel);
			auto& delay = delays[channel];
			for (int i = blockStart; i < end; i++) {
				outputChannel[i] +=
					dryWet * (delay.process(outputChannel[i], delaySmoothed[channel].getNextValue(), 0.f) - outputChannel[i]);
			}
		}
	}

	return true; // TODO
}


void customDsp::Phaser::prepareUpdate() {
	lfoData.sampleRate = data->sampleRate;
	lfoData.wt = wavetable::WavetableCache::getInstance()->getWavetable(0);
	reset();
}
void customDsp::Phaser::reset() {
	for (auto& l : lfos) {
		l.reset();
	}
	lfos[RIGHT].advancePhase(juce::MathConstants<float>::halfPi);
	for (int i = 0; i < stages; i++) {
		allpassS1[LEFT][i] = 0.f;
		allpassS1[RIGHT][i] = 0.f;
	}
	last[LEFT] = 0.f;
	last[RIGHT] = 0.f;
}

bool customDsp::Phaser::process(juce::dsp::ProcessContextNonReplacing<float>& context, juce::dsp::AudioBlock<float>& workBuffers) {

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

		auto dryWet = dryWetBase + dryWetMod * dryWetModSrc[blockStart];
		auto normalizedRate = juce::jlimit(0.f, 1.f, rateBase + rateMod * rateModSrc[blockStart]);
		auto normalizedDepth = juce::jlimit(0.f, 1.f, depthBase + depthMod * depthModSrc[blockStart]);
		auto feedback = juce::jlimit(0.f, 0.9f, feedbackBase + feedbackMod * feedbackModSrc[blockStart]);

		lfoData.rate = std::powf(3, 2 * normalizedRate) - 1;

		auto lfoValues = workBuffers.getSingleChannelBlock(0).getSubBlock(0, length);
		auto lfoContext = juce::dsp::ProcessContextNonReplacing<float>(emptyBlock, lfoValues);

		// coefficients for allpass calculation
		float G[stages];

		for (int channel = LEFT; channel <= RIGHT; channel++) {
			lfoValues.clear();
			lfos[channel].process(lfoContext, emptyBlock);
			auto normalizedLfo = normalizedDepth * (lfoValues.getSample(0, length - 1) + 1.f) / 2.f;
			for (int i = 0; i < stages; i++) {
				auto cutoff = minCutoffs[i] * std::powf(maxCutoffs[i] / minCutoffs[i], normalizedLfo);
				auto g = juce::dsp::FastMathApproximations::tan<float>(juce::MathConstants<float>::pi * cutoff / data->sampleRate);
				G[i] = g / (g + 1);
			}
			auto outputChannel = outputBlock.getChannelPointer(channel);
			for (int i = blockStart; i < end; i++) {
				auto input = outputChannel[i] + feedback * last[channel];
				// allpass calculations in series
				// for reference see FirstOrderTptFilter from JUCE or directly Vadim Zavalishin's 
				// literature on TPT filter design
				for (int s = 0; s < stages; s++) {
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

	return true; // TODO
}