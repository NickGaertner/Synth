
#include "SpecificGUI.h"
#include "Configuration.h"
#include "FX.h"

customGui::MainComponent::MainComponent(SynthAudioProcessor& audioProcessor)
	: headerMenu(audioProcessor)
{
	mainGrid.columnGap = Px(Constants::seperatorSizePx);
	mainGrid.rowGap = Px(Constants::seperatorSizePx);

	mainGrid.templateRows = {
	Track("buffer0-start", Fr(0), "buffer0-end"),
	Track("header-row-start", Fr(1), "header-row-end"),
	Track("modules0-row-start", Fr(8), "modules0-row-end"),
	Track("modules1-row-start", Fr(4), "modules1.5-row-end"),
	Track("modules1.5-row-start", Fr(4), "modules1-row-end"),
	Track("buffer1-start", Fr(0), "buffer1-end"),
	};

	mainGrid.templateColumns = {
	Track("buffer0-start", Fr(0), "buffer0-end"),
	Track("col0-start", Fr(2), "col0-end"),
	Track("col1-start", Fr(1), "col1.5-end"),
	Track("col1.5-start", Fr(1), "col1-end"),
	Track("col2-start", Fr(1), "col2.5-end"),
	Track("col2.5-start", Fr(1), "col2-end"),
	Track("buffer1-start", Fr(0), "buffer1-end"),
	};

	mainGrid.items = {
		juce::GridItem(headerMenu).withArea(Property("header-row-start"), Property("col0-start"),
		Property("header-row-end"), Property("col2-end")),

		juce::GridItem(oscModuleHolder).withArea(Property("modules0-row-start"), Property("col0-start"),
		Property("modules1.5-row-end"), Property("col0-end")),
		juce::GridItem(filterModuleHolder).withArea(Property("modules0-row-start"), Property("col1-start"),
		Property("modules0-row-end"), Property("col1-end")),
		juce::GridItem(fxModuleHolder).withArea(Property("modules0-row-start"), Property("col2-start"),
		Property("modules0-row-end"), Property("col2-end")),

		juce::GridItem(envModuleHolder).withArea(Property("modules1.5-row-start"), Property("col0-start"),
		Property("modules1-row-end"), Property("col1.5-end")),
		juce::GridItem(lfoModuleHolder).withArea(Property("modules1.5-row-start"), Property("col1.5-start"),
		Property("modules1-row-end"), Property("col2-end")),
		juce::GridItem(panModuleHolder).withArea(Property("modules1-row-start"), Property("col2-start"),
		Property("modules1.5-row-end"), Property("col2.5-end")),
		juce::GridItem(masterModuleHolder).withArea(Property("modules1-row-start"), Property("col2.5-start"),
		Property("modules1.5-row-end"), Property("col2-end")),
		juce::GridItem(spectrumModuleHolder).withArea(Property("modules1-row-start"), Property("col1-start"),
		Property("modules1.5-row-end"), Property("col1-end")),
	};

	addAndMakeVisible(headerMenu);

	addAndMakeVisible(oscModuleHolder);
	addAndMakeVisible(filterModuleHolder);
	addAndMakeVisible(fxModuleHolder);
	addAndMakeVisible(envModuleHolder);
	addAndMakeVisible(lfoModuleHolder);
	addAndMakeVisible(panModuleHolder);
	addAndMakeVisible(masterModuleHolder);
	addAndMakeVisible(spectrumModuleHolder);

	initModules(audioProcessor);
}

customGui::MainComponent::~MainComponent()
{
}

void customGui::MainComponent::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);
}

void customGui::MainComponent::resized()
{
	mainGrid.performLayout(getLocalBounds());
}

void customGui::MainComponent::initModules(SynthAudioProcessor& audioProcessor)
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
	spectrumModuleHolder.addModule(new SpectrumAnalyzerModule(audioProcessor));
	panModuleHolder.addModule(new PanModule(audioProcessor));
	masterModuleHolder.addModule(new MasterModule(audioProcessor));
}

customGui::OscModule::OscModule(SynthAudioProcessor& audioProcessor, int id) :
	SynthModule(audioProcessor, id, 4)
{
	juce::String prefix{ configuration::OSC_PREFIX + juce::String(id) };

	// LAYOUT
	nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);

	auto& grid = gridComponent.grid;
	grid.items.addArray({
		juce::GridItem(wtPosKnob).withArea(Property("knobRow0-start"), Property(1)),
		juce::GridItem(wtPosModKnob).withArea(Property("knobRow1-start"), Property(1)),
		juce::GridItem(wtPosModSrcChooser).withArea(Property("modSrcRow-start"), Property(1)),

		juce::GridItem(pitchKnob).withArea(Property("knobRow0-start"), Property(2)),
		juce::GridItem(pitchModKnob).withArea(Property("knobRow1-start"), Property(2)),
		juce::GridItem(pitchModSrcChooser).withArea(Property("modSrcRow-start"), Property(2)),

		juce::GridItem(gainKnob).withArea(Property("knobRow0-start"), Property(3)),
		juce::GridItem(gainModKnob).withArea(Property("knobRow1-start"), Property(3)),
		juce::GridItem(gainModSrcChooser).withArea(Property("modSrcRow-start"), Property(3)),

		juce::GridItem(envLabel).withArea(Property("knobRow1-start"), Property(4)),
		juce::GridItem(envChooser).withArea(Property("modSrcRow-start"), Property(4)),
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

	envLabel.setColour(envLabel.textColourId, Constants::text1Colour);
	envLabel.setJustificationType(juce::Justification::centredBottom);
	addAndMakeVisible(envLabel);
	addAndMakeVisible(envChooser);

	// VALUE TREE ATTACHMENTS
	auto& apvts = audioProcessor.getApvts();

	buttonAttachments.add(new ButtonAttachment(apvts, prefix + configuration::BYPASSED_SUFFIX, bypassedButton));
	dropDown.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(prefix + configuration::WT_SUFFIX))->choices, 1);
	comboBoxAttachments.add(new ComboBoxAttachment(apvts, prefix + configuration::WT_SUFFIX, dropDown));

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::WT_POS_SUFFIX, wtPosKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::WT_POS_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		wtPosModKnob.knob));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::WT_POS_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		wtPosModSrcChooser));
	wtPosModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::PITCH_SUFFIX, pitchKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::PITCH_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		pitchModKnob.knob));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::PITCH_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		pitchModSrcChooser));
	pitchModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::GAIN_SUFFIX, gainKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::GAIN_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		gainModKnob.knob));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::GAIN_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		gainModSrcChooser));
	gainModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::ENV_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		envChooser));
	envChooser.addItemList(configuration::getModChannelNames(), 1);
}

void customGui::OscModule::resized()
{
	SynthModule::resized();
	auto proportion = NamedKnob::knobFlex / (NamedKnob::labelFlex + NamedKnob::knobFlex);
	envLabel.setBounds(envLabel.getBounds().withTrimmedTop(envLabel.proportionOfHeight(proportion)));
}

customGui::FilterModule::FilterModule(SynthAudioProcessor& audioProcessor, int id) :
	SynthModule(audioProcessor, id, 3)
{
	juce::String prefix{ configuration::FILTER_PREFIX + juce::String(id) };

	// LAYOUT
	nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);

	auto& grid = gridComponent.grid;
	grid.items.addArray({
		juce::GridItem(cutoffKnob).withArea(Property("knobRow0-start"), Property(1)),
		juce::GridItem(cutoffModKnob).withArea(Property("knobRow1-start"), Property(1)),
		juce::GridItem(cutoffModSrcChooser).withArea(Property("modSrcRow-start"), Property(1)),

		juce::GridItem(resonanceKnob).withArea(Property("knobRow0-start"), Property(2)),
		juce::GridItem(resonanceModKnob).withArea(Property("knobRow1-start"), Property(2)),
		juce::GridItem(resonanceModSrcChooser).withArea(Property("modSrcRow-start"), Property(2)),

		juce::GridItem(specialKnob).withArea(Property("knobRow0-start"), Property(3)),
		juce::GridItem(specialModKnob).withArea(Property("knobRow1-start"), Property(3)),
		juce::GridItem(specialModSrcChooser).withArea(Property("modSrcRow-start"), Property(3)),
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

	buttonAttachments.add(new ButtonAttachment(apvts, prefix + configuration::BYPASSED_SUFFIX, bypassedButton));
	dropDown.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(prefix + configuration::FILTER_TYPE_SUFFIX))->choices, 1);
	comboBoxAttachments.add(new ComboBoxAttachment(apvts, prefix + configuration::FILTER_TYPE_SUFFIX, dropDown));

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::CUTOFF_SUFFIX, cutoffKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::CUTOFF_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		cutoffModKnob.knob));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::CUTOFF_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		cutoffModSrcChooser));
	cutoffModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::RESONANCE_SUFFIX, resonanceKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::RESONANCE_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		resonanceModKnob.knob));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::RESONANCE_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		resonanceModSrcChooser));
	resonanceModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::SPECIAL_SUFFIX, specialKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::SPECIAL_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		specialModKnob.knob));
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
	nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);

	auto& grid = gridComponent.grid;
	grid.items.addArray({
		juce::GridItem(dryWetKnob).withArea(Property("knobRow0-start"), Property(1)),
		juce::GridItem(dryWetModKnob).withArea(Property("knobRow1-start"), Property(1)),
		juce::GridItem(dryWetModSrcChooser).withArea(Property("modSrcRow-start"), Property(1)),

		juce::GridItem(parameter0Knob).withArea(Property("knobRow0-start"), Property(2)),
		juce::GridItem(parameter0ModKnob).withArea(Property("knobRow1-start"), Property(2)),
		juce::GridItem(parameter0ModSrcChooser).withArea(Property("modSrcRow-start"), Property(2)),

		juce::GridItem(parameter1Knob).withArea(Property("knobRow0-start"), Property(3)),
		juce::GridItem(parameter1ModKnob).withArea(Property("knobRow1-start"), Property(3)),
		juce::GridItem(parameter1ModSrcChooser).withArea(Property("modSrcRow-start"), Property(3)),

		juce::GridItem(parameter2Knob).withArea(Property("knobRow0-start"), Property(4)),
		juce::GridItem(parameter2ModKnob).withArea(Property("knobRow1-start"), Property(4)),
		juce::GridItem(parameter2ModSrcChooser).withArea(Property("modSrcRow-start"), Property(4)),
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

	buttonAttachments.add(new ButtonAttachment(apvts, prefix + configuration::BYPASSED_SUFFIX, bypassedButton));
	dropDown.onChange = [&]() {
		int fxType = dropDown.getSelectedId() - 1;
		parameter0Knob.setLabelText(customDsp::PARAMETER_0_NAMES[fxType]);
		parameter1Knob.setLabelText(customDsp::PARAMETER_1_NAMES[fxType]);
		parameter2Knob.setLabelText(customDsp::PARAMETER_2_NAMES[fxType]);
	};
	dropDown.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(prefix + configuration::FX_TYPE_SUFFIX))->choices, 1);
	comboBoxAttachments.add(new ComboBoxAttachment(apvts, prefix + configuration::FX_TYPE_SUFFIX, dropDown));

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::DRY_WET_SUFFIX, dryWetKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::DRY_WET_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		dryWetModKnob.knob));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::DRY_WET_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		dryWetModSrcChooser));
	dryWetModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::PARAMETER_0_SUFFIX, parameter0Knob.knob));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::PARAMETER_0_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		parameter0ModKnob.knob));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::PARAMETER_0_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		parameter0ModSrcChooser));
	parameter0ModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::PARAMETER_1_SUFFIX, parameter1Knob.knob));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::PARAMETER_1_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		parameter1ModKnob.knob));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::PARAMETER_1_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		parameter1ModSrcChooser));
	parameter1ModSrcChooser.addItemList(configuration::getModChannelNames(), 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::PARAMETER_2_SUFFIX, parameter2Knob.knob));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::PARAMETER_2_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		parameter2ModKnob.knob));
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
	removeChildComponent(&bypassedButton);
	removeChildComponent(&dropDown);

	nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);
	nameLabel.setColour(nameLabel.textColourId, Constants::envColour);
	nameLabel.setJustificationType(juce::Justification::centred);
	headerHBox.items = { juce::FlexItem(nameLabel).withFlex(1.f) };

	auto& grid = gridComponent.grid;
	grid.items.addArray({
		juce::GridItem(attackKnob).withArea(Property("knobRow0-start"), Property(1)),
		juce::GridItem(decayKnob).withArea(Property("knobRow0-start"), Property(2)),
		juce::GridItem(sustainKnob).withArea(Property("knobRow1-start"), Property(1)),
		juce::GridItem(releaseKnob).withArea(Property("knobRow1-start"), Property(2)),
		});

	addAndMakeVisible(attackKnob);
	addAndMakeVisible(decayKnob);
	addAndMakeVisible(sustainKnob);
	addAndMakeVisible(releaseKnob);

	// VALUE TREE ATTACHMENTS
	auto& apvts = audioProcessor.getApvts();

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::ATTACK_SUFFIX, attackKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::DECAY_SUFFIX, decayKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::SUSTAIN_SUFFIX, sustainKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::RELEASE_SUFFIX, releaseKnob.knob));

}

customGui::LFOModule::LFOModule(SynthAudioProcessor& audioProcessor, int id)
	: SynthModule(audioProcessor, id)
{
	juce::String prefix{ configuration::LFO_PREFIX + juce::String(id) };

	// LAYOUT
	removeChildComponent(&bypassedButton);

	nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);
	nameLabel.setColour(nameLabel.textColourId, Constants::lfoColour);
	headerHBox.items = { juce::FlexItem(nameLabel).withFlex(3.f), juce::FlexItem(dropDown).withFlex(7.f) };


	// adjust original layout
	auto& grid = gridComponent.grid;
	//grid.templateRows = {};

	grid.items.addArray({
		juce::GridItem(wtPosKnob).withArea(Property("knobRow0-start"), Property(1)),
		juce::GridItem(rateKnob).withArea(Property("knobRow0-start"), Property(2)),
		});

	addAndMakeVisible(wtPosKnob);
	addAndMakeVisible(rateKnob);

	// VALUE TREE ATTACHMENTS
	auto& apvts = audioProcessor.getApvts();

	dropDown.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(prefix + configuration::WT_SUFFIX))->choices, 1);
	comboBoxAttachments.add(new ComboBoxAttachment(apvts, prefix + configuration::WT_SUFFIX, dropDown));

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::WT_POS_SUFFIX, wtPosKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::RATE_SUFFIX, rateKnob.knob));
}

customGui::PanModule::PanModule(SynthAudioProcessor& audioProcessor, int id)
	: SynthModule(audioProcessor, id)
{
	juce::String prefix{ configuration::PAN_PREFIX };

	// LAYOUT
	removeChildComponent(&bypassedButton);
	removeChildComponent(&dropDown);

	nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);
	nameLabel.setJustificationType(juce::Justification::centred);
	headerHBox.items = { juce::FlexItem(nameLabel).withFlex(1.f) };

	auto& grid = gridComponent.grid;
	grid.items.addArray({
		juce::GridItem(panKnob).withArea(Property("knobRow0-start"), Property(1)),
		juce::GridItem(panModKnob).withArea(Property("knobRow1-start"), Property(1)),
		juce::GridItem(panModSrcChooser).withArea(Property("modSrcRow-start"), Property(1)),
		});

	addAndMakeVisible(panKnob);
	addAndMakeVisible(panModKnob);
	addAndMakeVisible(panModSrcChooser);

	// VALUE TREE ATTACHMENTS
	auto& apvts = audioProcessor.getApvts();

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::PAN_SUFFIX, panKnob.knob));
	sliderAttachments.add(new SliderAttachment(apvts,
		prefix + configuration::PAN_SUFFIX + configuration::MOD_FACTOR_SUFFIX,
		panModKnob.knob));
	comboBoxAttachments.add(new ComboBoxAttachment(apvts,
		prefix + configuration::PAN_SUFFIX + configuration::MOD_CHANNEL_SUFFIX,
		panModSrcChooser));
	panModSrcChooser.addItemList(configuration::getModChannelNames(), 1);
}

customGui::SpectrumAnalyzerModule::SpectrumAnalyzerModule(SynthAudioProcessor& audioProcessor, int id)
	: SynthModule(audioProcessor, id), analyzer(audioProcessor)
{
	removeChildComponent(&dropDown);

	nameLabel.setText("Spectrum Analyzer", juce::NotificationType::dontSendNotification);
	headerHBox.items = { juce::FlexItem(bypassedButton).withFlex(1.f), juce::FlexItem(nameLabel).withFlex(9.f) };

	auto& grid = gridComponent.grid;
	grid.items.addArray({
		juce::GridItem(fftOrderKnob).withArea(Property("knobRow0-start"), Property(1)),
		juce::GridItem(refreshRateKnob).withArea(Property("knobRow1-start"), Property(1)),
		juce::GridItem(analyzer).withArea(Property("knobRow0-start"), Property(2),Property("modSrcRow-end"), Property(5)),
		});

	addAndMakeVisible(fftOrderKnob);
	addAndMakeVisible(refreshRateKnob);
	addAndMakeVisible(analyzer);

	// Connect knobs to analyzer
	bypassedButton.onClick = [&]() {
		analyzer.setBypassed(bypassedButton.getToggleState());
	};

	fftOrderKnob.knob.setRange(0, 4, 1);
	fftOrderKnob.knob.onValueChange = [&]() {
		auto order = 11 + (int)fftOrderKnob.knob.getValue();
		analyzer.setFFTOrder(order);
	};
	fftOrderKnob.knob.setValue(analyzer.getFFTOrder() - 11);

	refreshRateKnob.knob.setRange(0, 3, 1);
	refreshRateKnob.knob.onValueChange = [&]() {
		auto blockNumber = (int)std::exp2(refreshRateKnob.knob.getValue());
		analyzer.setBlockNumber(blockNumber);
	};
	refreshRateKnob.knob.setValue(std::log2(analyzer.getBlockNumber()));
}

customGui::MasterModule::MasterModule(SynthAudioProcessor& t_audioProcessor, int id)
	: SynthModule(t_audioProcessor, id),
	audioProcessor(t_audioProcessor)
{
	juce::String prefix{ configuration::MASTER_PREFIX };

	// LAYOUT
	removeChildComponent(&bypassedButton);
	removeChildComponent(&dropDown);

	nameLabel.setText(juce::String(prefix), juce::NotificationType::dontSendNotification);
	nameLabel.setJustificationType(juce::Justification::centred);
	headerHBox.items = { juce::FlexItem(nameLabel).withFlex(1.f) };

	auto& grid = gridComponent.grid;
	grid.items.addArray({
		juce::GridItem(masterKnob).withArea(Property("knobRow0-start"), Property(1)),
		juce::GridItem(levelDisplay).withArea(Property("knobRow0-start"), Property(2), Property("modSrcRow-end"), Property(3)),
		});

	addAndMakeVisible(masterKnob);
	addAndMakeVisible(levelDisplay);

	// VALUE TREE ATTACHMENTS
	jassert(!audioProcessor.masterLevelCallback);
	audioProcessor.masterLevelCallback = [&](float levelLeft, float levelRight) {
		levelDisplay.updateLevel(levelLeft, levelRight);
	};

	auto& apvts = audioProcessor.getApvts();

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::GAIN_SUFFIX, masterKnob.knob));
}

