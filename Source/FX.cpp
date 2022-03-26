/*
  ==============================================================================

	FX.cpp
	Created: 20 Mar 2022 5:09:03pm
	Author:  nick

  ==============================================================================
*/

#include "FX.h"

void customDsp::FXChooser::createFX(FXType type)
{
	switch (type) {
	case FXType::CHORUS: fx = std::make_unique<DummyFX>(data); break;
	case FXType::DELAY: fx = std::make_unique<DummyFX>(data); break;
	case FXType::FLANGER: fx = std::make_unique<DummyFX>(data); break;
	case FXType::TUBE: fx = std::make_unique<DummyFX>(data); break;
	case FXType::REVERB: fx = std::make_unique<Reverb>(data); break;
	case FXType::PHASER: fx = std::make_unique<DummyFX>(data); break;
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
		combData[LEFT][i].update(leftCombBufferSize, lSize);
		leftCombBufferSize += lSize;

		auto rSize = (intSampleRate * (combTunings[i] + stereoSpread)) / 44100;
		combData[RIGHT][i].update(rightCombBufferSize,rSize);
		rightCombBufferSize += rSize;
	}
	combBuffer[LEFT].allocate(leftCombBufferSize, true);
	combBuffer[RIGHT].allocate(rightCombBufferSize, true);

	leftAllPassBufferSize = 0;
	rightAllPassBufferSize = 0;
	for (int i = 0; i < numAllPasses; ++i)
	{
		auto lSize = (intSampleRate * allPassTunings[i]) / 44100;
		allPassData[LEFT][i].update(leftAllPassBufferSize, lSize);
		leftAllPassBufferSize += lSize;

		auto rSize = (intSampleRate * (allPassTunings[i] + stereoSpread)) / 44100;
		allPassData[RIGHT][i].update(rightAllPassBufferSize, rSize);
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
		auto end = juce::jmin(blockStart + configuration::MOD_BLOCK_SIZE,(int) numSamples);
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
			outputBlock.getSubBlock(blockStart,length).multiplyBy(dryGain);
			outputBlock.getSingleChannelBlock(n).getSubBlock(blockStart, length).addProductOf(currentOutWB, wetGain1);
			outputBlock.getSingleChannelBlock((n + 1) % 2).getSubBlock(blockStart, length).addProductOf(currentOutWB, wetGain2);
		}
	}
	//JUCE_END_IGNORE_WARNINGS_MSVC

	return (MIN_LEVEL < outputBlock.getSample(LEFT, 0)) || (MIN_LEVEL < outputBlock.getSample(LEFT, numSamples-1));
}
