
#include "GUI.h"
#include "FX.h"

customGui::SynthComponent::SynthComponent(SynthAudioProcessor& audioProcessor)
	: spectrumModule(audioProcessor), panModule(audioProcessor), masterModule(audioProcessor)
{
	mainGrid.columnGap = Px(0);
	mainGrid.rowGap = Px(0);

	mainGrid.templateRows = { Track("header-row-start", Fr(1), "header-row-end"),
	Track("modules0-row-start", Fr(6), "modules0-row-end"),
	Track("modules1-row-start", Fr(3), "modules1.5-row-end"),
	Track("modules1.5-row-start", Fr(3), "modules1-row-end"),
	};

	mainGrid.templateColumns = { Track("col0-start", Fr(2), "col0-end"),
	Track("col1-start", Fr(2), "col1-end"),
	Track("col2-start", Fr(1), "col2.5-end"),
	Track("col2.5-start", Fr(1), "col2-end"),
	};


	mainGrid.items = {
		juce::GridItem(header).withArea(Property("header-row-start"), Property(1), Property("header-row-end"), Property(-1)),

		juce::GridItem(oscModuleHolder).withArea(Property("modules0-row-start"), Property("col0-start"),
		Property("modules1.5-row-end"), Property("col0-end")),
		juce::GridItem(filterModuleHolder).withArea(Property("modules0-row-start"), Property("col1-start"),
		Property("modules0-row-end"), Property("col1-end")),
		juce::GridItem(fxModuleHolder).withArea(Property("modules0-row-start"), Property("col2-start"),
		Property("modules0-row-end"), Property("col2-end")),

		juce::GridItem(envModuleHolder).withArea(Property("modules1-row-start"), Property("col1-start"),
		Property("modules1-row-end"), Property("col1-end")),
		juce::GridItem(lfoModuleHolder).withArea(Property("modules1.5-row-start"), Property("col0-start"),
		Property("modules1-row-end"), Property("col0-end")),
		juce::GridItem(panModule).withArea(Property("modules1-row-start"), Property("col2-start"),
		Property("modules1.5-row-end"), Property("col2.5-end")),
		juce::GridItem(masterModule).withArea(Property("modules1-row-start"), Property("col2.5-start"),
		Property("modules1.5-row-end"), Property("col2-end")),

		juce::GridItem(spectrumModule).withArea(Property("modules1.5-row-start"), Property("col2-start"),
		Property("modules1-row-end"), Property("col2-end")),
	};

	header.setColour(juce::Label::ColourIds::backgroundColourId, Util::createRandomColour());
	addAndMakeVisible(header);

	addAndMakeVisible(oscModuleHolder);
	addAndMakeVisible(filterModuleHolder);
	addAndMakeVisible(fxModuleHolder);
	addAndMakeVisible(envModuleHolder);
	addAndMakeVisible(lfoModuleHolder);
	addAndMakeVisible(panModule);
	addAndMakeVisible(masterModule);
	addAndMakeVisible(spectrumModule);

	initModules(audioProcessor);
}

customGui::SynthComponent::~SynthComponent()
{
}

void customGui::SynthComponent::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);
}

void customGui::SynthComponent::resized()
{
	mainGrid.performLayout(getLocalBounds());
}

void customGui::SynthComponent::initModules(SynthAudioProcessor& audioProcessor)
{
	for (auto i = 0; i < configuration::OSC_NUMBER; i++) {
		oscModuleHolder.addModule(new OscModule(audioProcessor, i));
	}
	for (auto i = 0; i < configuration::FILTER_NUMBER; i++) {
		filterModuleHolder.addModule(new FilterModule(audioProcessor, i));
	}
	for (auto i = 0; i < configuration::FX_NUMBER; i++) {
		fxModuleHolder.addModule(new FXModule(audioProcessor, i));
	}
	for (auto i = 0; i < configuration::ENV_NUMBER; i++) {
		envModuleHolder.addModule(new EnvModule(audioProcessor, i));
	}
	for (auto i = 0; i < configuration::LFO_NUMBER; i++) {
		lfoModuleHolder.addModule(new LFOModule(audioProcessor, i));
	}
}

customGui::SynthModule::SynthModule(SynthAudioProcessor& audioProcessor, int id, int cols)
{
	juce::ignoreUnused(audioProcessor, id);
	mainGrid.autoFlow = juce::Grid::AutoFlow::row;
	mainGrid.autoRows = Track(Fr(5));
	mainGrid.autoColumns = Track(Fr(1));

	mainGrid.templateRows = { Track(Fr(1)), Track(Fr(5)), Track(Fr(5)), Track(Fr(1)), };

	//mainGrid.templateColumns = { Track(Fr(1)), Track(Fr(1)), };

	mainGrid.items = {
		juce::GridItem(powerAndName).withArea(Property(1), Property(1)),
		juce::GridItem(dropDown).withArea(Property(1), Property(2), Property(1), Property(cols + 1)),

	};

	addAndMakeVisible(powerAndName);
	addAndMakeVisible(dropDown);
}

void customGui::SynthModule::paint(juce::Graphics& g) {
	g.fillAll(Util::createRandomColour());

}

void customGui::SynthModule::resized()
{
	mainGrid.performLayout(getLocalBounds());
}

customGui::OscModule::OscModule(SynthAudioProcessor& audioProcessor, int id) :
	SynthModule(audioProcessor, id, 4)
{
	juce::String prefix{ configuration::OSC_PREFIX + juce::String(id) };

	// LAYOUT
	powerAndName.nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);

	mainGrid.items.addArray({
		juce::GridItem(wtPosKnob).withArea(Property(2), Property(1)),
		juce::GridItem(wtPosModKnob).withArea(Property(3), Property(1)),
		juce::GridItem(wtPosModSrcChooser).withArea(Property(4), Property(1)),

		juce::GridItem(pitchKnob).withArea(Property(2), Property(2)),
		juce::GridItem(pitchModKnob).withArea(Property(3), Property(2)),
		juce::GridItem(pitchModSrcChooser).withArea(Property(4), Property(2)),

		juce::GridItem(gainKnob).withArea(Property(2), Property(3)),
		juce::GridItem(gainModKnob).withArea(Property(3), Property(3)),
		juce::GridItem(gainModSrcChooser).withArea(Property(4), Property(3)),

		juce::GridItem(envChooser).withArea(Property(4), Property(4)),
		});

	addAndMakeVisible(wtPosKnob);
	addAndMakeVisible(wtPosModKnob);
	addAndMakeVisible(wtPosModSrcChooser);

	addAndMakeVisible(pitchKnob);
	addAndMakeVisible(pitchModKnob);
	addAndMakeVisible(pitchModSrcChooser);

	addAndMakeVisible(gainKnob);
	addAndMakeVisible(gainModKnob);
	addAndMakeVisible(gainModSrcChooser);

	addAndMakeVisible(envChooser);

	// VALUE TREE ATTACHMENTS
	auto& apvts = audioProcessor.getApvts();

	buttonAttachments.add(new ButtonAttachment(apvts, prefix + configuration::BYPASSED_SUFFIX, powerAndName.powerButton));
	dropDown.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(prefix + configuration::WT_SUFFIX))->choices, 1);
	comboBoxAttachments.add(new ComboBoxAttachment(apvts, prefix + configuration::WT_SUFFIX, dropDown));

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::WT_POS_SUFFIX, wtPosKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::WT_POS_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		wtPosModKnob.slider));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::WT_POS_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		wtPosModSrcChooser));
	wtPosModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::PITCH_SUFFIX, pitchKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::PITCH_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		pitchModKnob.slider));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::PITCH_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		pitchModSrcChooser));
	pitchModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::GAIN_SUFFIX, gainKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::GAIN_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		gainModKnob.slider));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::GAIN_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		gainModSrcChooser));
	gainModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::ENV_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		envChooser));
	envChooser.addItemList(configuration::getModChannelNames(), 1);
}

customGui::FilterModule::FilterModule(SynthAudioProcessor& audioProcessor, int id) :
	SynthModule(audioProcessor, id, 3)
{
	juce::String prefix{ configuration::FILTER_PREFIX + juce::String(id) };

	// LAYOUT
	powerAndName.nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);

	mainGrid.items.addArray({
		juce::GridItem(cutoffKnob).withArea(Property(2), Property(1)),
		juce::GridItem(cutoffModKnob).withArea(Property(3), Property(1)),
		juce::GridItem(cutoffModSrcChooser).withArea(Property(4), Property(1)),

		juce::GridItem(resonanceKnob).withArea(Property(2), Property(2)),
		juce::GridItem(resonanceModKnob).withArea(Property(3), Property(2)),
		juce::GridItem(resonanceModSrcChooser).withArea(Property(4), Property(2)),

		juce::GridItem(specialKnob).withArea(Property(2), Property(3)),
		juce::GridItem(specialModKnob).withArea(Property(3), Property(3)),
		juce::GridItem(specialModSrcChooser).withArea(Property(4), Property(3)),
		});

	addAndMakeVisible(cutoffKnob);
	addAndMakeVisible(cutoffModKnob);
	addAndMakeVisible(cutoffModSrcChooser);

	addAndMakeVisible(resonanceKnob);
	addAndMakeVisible(resonanceModKnob);
	addAndMakeVisible(resonanceModSrcChooser);

	addAndMakeVisible(specialKnob);
	addAndMakeVisible(specialModKnob);
	addAndMakeVisible(specialModSrcChooser);

	// VALUE TREE ATTACHMENTS
	auto& apvts = audioProcessor.getApvts();

	buttonAttachments.add(new ButtonAttachment(apvts, prefix + configuration::BYPASSED_SUFFIX, powerAndName.powerButton));
	dropDown.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(prefix + configuration::FILTER_TYPE_SUFFIX))->choices, 1);
	comboBoxAttachments.add(new ComboBoxAttachment(apvts, prefix + configuration::FILTER_TYPE_SUFFIX, dropDown));

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::CUTOFF_SUFFIX, cutoffKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::CUTOFF_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		cutoffModKnob.slider));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::CUTOFF_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		cutoffModSrcChooser));
	cutoffModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::RESONANCE_SUFFIX, resonanceKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::RESONANCE_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		resonanceModKnob.slider));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::RESONANCE_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		resonanceModSrcChooser));
	resonanceModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::SPECIAL_SUFFIX, specialKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::SPECIAL_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		specialModKnob.slider));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::SPECIAL_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		specialModSrcChooser));
	specialModSrcChooser.addItemList(configuration::getModChannelNames(), 1);
}

customGui::FXModule::FXModule(SynthAudioProcessor& audioProcessor, int id) :
	SynthModule(audioProcessor, id, 4)
{
	juce::String prefix{ configuration::FX_PREFIX + juce::String(id) };

	// LAYOUT
	powerAndName.nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);

	mainGrid.items.addArray({
		juce::GridItem(dryWetKnob).withArea(Property(2), Property(1)),
		juce::GridItem(dryWetModKnob).withArea(Property(3), Property(1)),
		juce::GridItem(dryWetModSrcChooser).withArea(Property(4), Property(1)),

		juce::GridItem(parameter0Knob).withArea(Property(2), Property(2)),
		juce::GridItem(parameter0ModKnob).withArea(Property(3), Property(2)),
		juce::GridItem(parameter0ModSrcChooser).withArea(Property(4), Property(2)),

		juce::GridItem(parameter1Knob).withArea(Property(2), Property(3)),
		juce::GridItem(parameter1ModKnob).withArea(Property(3), Property(3)),
		juce::GridItem(parameter1ModSrcChooser).withArea(Property(4), Property(3)),

		juce::GridItem(parameter2Knob).withArea(Property(2), Property(4)),
		juce::GridItem(parameter2ModKnob).withArea(Property(3), Property(4)),
		juce::GridItem(parameter2ModSrcChooser).withArea(Property(4), Property(4)),
		});

	addAndMakeVisible(dryWetKnob);
	addAndMakeVisible(dryWetModKnob);
	addAndMakeVisible(dryWetModSrcChooser);

	addAndMakeVisible(parameter0Knob);
	addAndMakeVisible(parameter0ModKnob);
	addAndMakeVisible(parameter0ModSrcChooser);

	addAndMakeVisible(parameter1Knob);
	addAndMakeVisible(parameter1ModKnob);
	addAndMakeVisible(parameter1ModSrcChooser);

	addAndMakeVisible(parameter2Knob);
	addAndMakeVisible(parameter2ModKnob);
	addAndMakeVisible(parameter2ModSrcChooser);

	// VALUE TREE ATTACHMENTS
	auto& apvts = audioProcessor.getApvts();

	buttonAttachments.add(new ButtonAttachment(apvts, prefix + configuration::BYPASSED_SUFFIX, powerAndName.powerButton));
	dropDown.onChange = [&]() {
		int fxType = dropDown.getSelectedId() - 1;
		parameter0Knob.setLabelText(customDsp::PARAMETER_0_NAMES[fxType]);
		parameter1Knob.setLabelText(customDsp::PARAMETER_1_NAMES[fxType]);
		parameter2Knob.setLabelText(customDsp::PARAMETER_2_NAMES[fxType]);
	};
	dropDown.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(prefix + configuration::FX_TYPE_SUFFIX))->choices, 1);
	comboBoxAttachments.add(new ComboBoxAttachment(apvts, prefix + configuration::FX_TYPE_SUFFIX, dropDown));

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::DRY_WET_SUFFIX, dryWetKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::DRY_WET_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		dryWetModKnob.slider));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::DRY_WET_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		dryWetModSrcChooser));
	dryWetModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::PARAMETER_0_SUFFIX, parameter0Knob.slider));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::PARAMETER_0_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		parameter0ModKnob.slider));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::PARAMETER_0_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		parameter0ModSrcChooser));
	parameter0ModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::PARAMETER_1_SUFFIX, parameter1Knob.slider));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::PARAMETER_1_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		parameter1ModKnob.slider));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::PARAMETER_1_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		parameter1ModSrcChooser));
	parameter1ModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::PARAMETER_2_SUFFIX, parameter2Knob.slider));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::PARAMETER_2_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		parameter2ModKnob.slider));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::PARAMETER_2_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		parameter2ModSrcChooser));
	parameter2ModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

}

customGui::EnvModule::EnvModule(SynthAudioProcessor& audioProcessor, int id) :
	SynthModule(audioProcessor, id)
{
	juce::String prefix{ configuration::ENV_PREFIX + juce::String(id) };

	// LAYOUT
	powerAndName.powerButton.setVisible(false);
	powerAndName.nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);
	dropDown.setVisible(false);

	mainGrid.items.addArray({
		juce::GridItem(attackKnob).withArea(Property(2), Property(1)),
		juce::GridItem(decayKnob).withArea(Property(2), Property(2)),
		juce::GridItem(sustainKnob).withArea(Property(3), Property(1)),
		juce::GridItem(releaseKnob).withArea(Property(3), Property(2)),
		});

	addAndMakeVisible(attackKnob);
	addAndMakeVisible(decayKnob);
	addAndMakeVisible(sustainKnob);
	addAndMakeVisible(releaseKnob);

	// VALUE TREE ATTACHMENTS
	auto& apvts = audioProcessor.getApvts();

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::ATTACK_SUFFIX, attackKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::DECAY_SUFFIX, decayKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::SUSTAIN_SUFFIX, sustainKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::RELEASE_SUFFIX, releaseKnob.slider));

}

customGui::LFOModule::LFOModule(SynthAudioProcessor& audioProcessor, int id)
	: SynthModule(audioProcessor, id)
{
	juce::String prefix{ configuration::LFO_PREFIX + juce::String(id) };

	// LAYOUT
	powerAndName.powerButton.setVisible(false);
	powerAndName.nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);

	// adjust original layout
	mainGrid.templateRows = { Track(Fr(1)), Track(Fr(5)) };

	mainGrid.items.addArray({
		juce::GridItem(wtPosKnob).withArea(Property(2), Property(1)),
		juce::GridItem(rateKnob).withArea(Property(2), Property(2)),
		});

	addAndMakeVisible(wtPosKnob);
	addAndMakeVisible(rateKnob);

	// VALUE TREE ATTACHMENTS
	auto& apvts = audioProcessor.getApvts();

	dropDown.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(prefix + configuration::WT_SUFFIX))->choices, 1);
	comboBoxAttachments.add(new ComboBoxAttachment(apvts, prefix + configuration::WT_SUFFIX, dropDown));

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::WT_POS_SUFFIX, wtPosKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::RATE_SUFFIX, rateKnob.slider));
}

customGui::PanModule::PanModule(SynthAudioProcessor& audioProcessor, int id)
	: SynthModule(audioProcessor, id)
{
	juce::String prefix{ configuration::PAN_PREFIX };

	// LAYOUT
	powerAndName.powerButton.setVisible(false);
	powerAndName.nameLabel.setText(prefix, juce::NotificationType::dontSendNotification);
	dropDown.setVisible(false);

	mainGrid.items.addArray({
		juce::GridItem(panKnob).withArea(Property(2), Property(1)),
		juce::GridItem(panModKnob).withArea(Property(3), Property(1)),
		juce::GridItem(panModSrcChooser).withArea(Property(4), Property(1)),
		});

	addAndMakeVisible(panKnob);
	addAndMakeVisible(panModKnob);
	addAndMakeVisible(panModSrcChooser);

	// VALUE TREE ATTACHMENTS
	auto& apvts = audioProcessor.getApvts();

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::PAN_SUFFIX, panKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::PAN_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		panModKnob.slider));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::PAN_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		panModSrcChooser));
	panModSrcChooser.addItemList(configuration::getModChannelNames(), 1);
}

customGui::MasterModule::MasterModule(SynthAudioProcessor& audioProcessor, int id)
	: SynthModule(audioProcessor, id)
{
	juce::String prefix{ configuration::MASTER_PREFIX };

	// LAYOUT
	powerAndName.powerButton.setVisible(false);
	powerAndName.nameLabel.setText(prefix, juce::NotificationType::dontSendNotification);
	dropDown.setVisible(false);

	mainGrid.items.addArray({
		juce::GridItem(masterKnob).withArea(Property(2), Property(1)),
		juce::GridItem(levelDisplay).withArea(Property(2), Property(2), Property(4), Property(3)),
		});

	addAndMakeVisible(masterKnob);
	addAndMakeVisible(levelDisplay);

	// VALUE TREE ATTACHMENTS
	audioProcessor.masterLevelCallback = [&](float levelLeft, float levelRight) {
		levelDisplay.updateLevel(levelLeft, levelRight);
	};

	auto& apvts = audioProcessor.getApvts();

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::GAIN_SUFFIX, masterKnob.slider));
}

customGui::SpectrumAnalyzerModule::SpectrumAnalyzerModule(SynthAudioProcessor& audioProcessor, int id)
	: SynthModule(audioProcessor, id), analyzer(audioProcessor)
{
	powerAndName.nameLabel.setText("Spectrum Analyzer", juce::NotificationType::dontSendNotification);
	dropDown.setVisible(false);
	mainGrid.items.addArray({
		juce::GridItem(fftOrderKnob).withArea(Property(2), Property(1)),
		juce::GridItem(refreshRateKnob).withArea(Property(3), Property(1)),
		juce::GridItem(analyzer).withArea(Property(2), Property(2),Property(4), Property(5)),
		});

	addAndMakeVisible(fftOrderKnob);
	addAndMakeVisible(refreshRateKnob);
	addAndMakeVisible(analyzer);

	// Connect knobs to analyzer
	powerAndName.powerButton.onClick = [&]() {
		analyzer.setBypassed(powerAndName.powerButton.getToggleState());
	};

	fftOrderKnob.slider.setRange(0, 4, 1);
	fftOrderKnob.slider.onValueChange = [&]() {
		auto order = 11 + (int)fftOrderKnob.slider.getValue();
		analyzer.setFFTOrder(order);
	};
	fftOrderKnob.slider.setValue(analyzer.getFFTOrder() - 11);

	refreshRateKnob.slider.setRange(0, 3, 1);
	refreshRateKnob.slider.onValueChange = [&]() {
		auto blockNumber = (int)std::exp2(refreshRateKnob.slider.getValue());
		analyzer.setBlockNumber(blockNumber);
	};
	refreshRateKnob.slider.setValue(std::log2(analyzer.getBlockNumber()));
}

customGui::SpectrumAnalyzer::SpectrumAnalyzer(SynthAudioProcessor& audioProcessor)
{
	setFFTOrder(14);
	setBlockNumber(8);
	audioProcessor.observationCallback = [this](const juce::dsp::AudioBlock<float>& b) {readBlock(b); };
	startTimerHz(30);
}

void customGui::SpectrumAnalyzer::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);
	g.setOpacity(1.f);
	g.setColour(juce::Colours::white);
	drawFrame(g);
}

void customGui::SpectrumAnalyzer::resized()
{
}

void customGui::SpectrumAnalyzer::timerCallback()
{
	if (bypassed) {
		return;
	}
	if (nextFFTBlockReady) {
		nextFFTBlockReady = false;
		updateSpectrum();
		repaint();
	}
}

void customGui::SpectrumAnalyzer::readBlock(const juce::dsp::AudioBlock<float>& audioBlock)
{
	if (bypassed) {
		return;
	}
	auto* channelPtr = audioBlock.getChannelPointer(0);
	auto channelPos = 0;
	auto numSamples = audioBlock.getNumSamples();
	while (channelPos < numSamples) {
		auto length = juce::jmin((size_t)fifoBlockSize - fifoLocalIndex, numSamples - channelPos);
		for (int i = 0; i < length; i++) {
			fifo[fifoBlockOffset * fifoBlockSize + fifoLocalIndex++] = channelPtr[channelPos++];
		}
		if (fifoLocalIndex == fifoBlockSize) {
			fifoLocalIndex = 0;
			fifoBlockOffset = (fifoBlockOffset + 1) % numBlocks;
			if (!nextFFTBlockReady) {
				nextFFTBlockReady = true;
				juce::zeromem(fftData, sizeof(float) * fftSize * 2);
				auto wrapPos = fifoBlockOffset * fifoBlockSize;
				auto bytesPerBlock = sizeof(float) * fftSize / numBlocks;
				memcpy(fftData, fifo + wrapPos, (numBlocks - fifoBlockOffset) * bytesPerBlock);
				memcpy(fftData + (fftSize - wrapPos), fifo, fifoBlockOffset * bytesPerBlock);
				/*for (int i = 0; i < fftSize; i++) {
					fftData[i] = fifo[(i + fifoBlockOffset * fifoBlockSize) % fftSize];
				}*/
			}
		}
	}
}

void customGui::SpectrumAnalyzer::setFFTOrder(int t_order)
{
	jassert(minFFTOrder <= t_order && t_order <= maxFFTOrder);
	fftOrder = t_order;
	fftSize = 1 << fftOrder;
	forwardFFT = juce::dsp::FFT{ fftOrder };
	window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::hann);
	juce::zeromem(fftData, sizeof(float) * fftSize * 2);
	fifoBlockOffset = 0;
	fifoBlockSize = fftSize / numBlocks;
	fifoLocalIndex = 0;
	nextFFTBlockReady = false;
}

void customGui::SpectrumAnalyzer::setBlockNumber(int t_numBlocks)
{
	jassert(1 <= t_numBlocks && t_numBlocks <= 16);
	numBlocks = t_numBlocks;
	fifoBlockOffset = 0;
	fifoBlockSize = fftSize / numBlocks;
	fifoLocalIndex = 0;
	nextFFTBlockReady = false;
}

void customGui::SpectrumAnalyzer::drawFrame(juce::Graphics& g)
{
	if (bypassed) {
		return;
	}
	auto width = getLocalBounds().getWidth();
	auto height = getLocalBounds().getHeight();
	for (int i = 1; i < scopeSize; i++) {
		g.drawLine({
			(float)juce::jmap(i - 1, 0, scopeSize - 1, 0, width),
			juce::jmap(scopeData[i - 1], 0.0f, 1.0f, (float)height, 0.0f),
			(float)juce::jmap(i, 0, scopeSize - 1, 0, width),
			juce::jmap(scopeData[i], 0.0f, 1.0f, (float)height, 0.0f)
			});
	}
}

void customGui::SpectrumAnalyzer::updateSpectrum()
{
	window->multiplyWithWindowingTable(fftData, fftSize);
	forwardFFT.performFrequencyOnlyForwardTransform(fftData);

	const auto minDB = -80.f;
	const auto maxDB = 0.f;
	const auto halfSizeLog = std::log2f(static_cast<float>(fftSize / 2));
	const auto localMinFreq = 25.f * (fftSize / 48000.f); // theoretically this should be dependant on sampleRate
	const auto offset = std::log2f(localMinFreq) / halfSizeLog;

	auto max = fftData[0];
	for (int i = 0; i < fftSize / 2; i++) {
		max = juce::jmax(max, fftData[i]);
	}

	for (int i = 0; i < scopeSize; i++) {
		auto t = (float)i / (float)scopeSize;
		auto exponent = (offset + t * (1.f - offset)) * halfSizeLog;
		int fftDataIndex = static_cast<int>(std::exp2f(exponent));
		auto level = fftData[fftDataIndex] / (max + 1);
		level = juce::jmap(juce::Decibels::gainToDecibels(level), minDB, maxDB, 0.f, 1.f);

		auto t2 = (float)(i + 0.5f) / (float)scopeSize;
		auto exponent2 = (offset + t2 * (1.f - offset)) * halfSizeLog;
		int fftDataIndex2 = static_cast<int>(std::exp2f(exponent2));
		auto level2 = fftData[fftDataIndex2] / (max + 1);
		level2 = juce::jmap(juce::Decibels::gainToDecibels(level2), minDB, maxDB, 0.f, 1.f);

		scopeData[i] = (level + level2) / 2.f;
	}

}

