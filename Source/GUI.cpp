
#include "GUI.h"

customGui::SynthComponent::SynthComponent(SynthAudioProcessor& audioProcessor)
{
	mainGrid.columnGap = Px(0);
	mainGrid.rowGap = Px(0);

	mainGrid.templateRows = { Track("header-row-start", Fr(1), "header-row-end"),
	Track("modules0-row-start", Fr(5), "modules0-row-end"),
	Track("modules1-row-start", Fr(5), "modules1-row-end"), };

	mainGrid.templateColumns = { Track("col0-start", Fr(1), "col0-end"),
	Track("col1-start", Fr(1), "col1-end"),
	Track("col2-start", Fr(1), "col2-end"), };


	mainGrid.items = {
		juce::GridItem(header).withArea(Property("header-row-start"), Property(1), Property("header-row-end"), Property(-1)),

		juce::GridItem(oscModuleHolder).withArea(Property("modules0-row-start"), Property("col0-start"),
		Property("modules0-row-end"), Property("col0-end")),
		juce::GridItem(filterModuleHolder).withArea(Property("modules0-row-start"), Property("col1-start"),
		Property("modules0-row-end"), Property("col1-end")),
		juce::GridItem(fxModuleHolder).withArea(Property("modules0-row-start"), Property("col2-start"),
		Property("modules0-row-end"), Property("col2-end")),

		juce::GridItem(envModuleHolder).withArea(Property("modules1-row-start"), Property("col0-start"),
		Property("modules1-row-end"), Property("col0-end")),
		juce::GridItem(lfoModuleHolder).withArea(Property("modules1-row-start"), Property("col1-start"),
		Property("modules1-row-end"), Property("col1-end")),
		//juce::GridItem(panModule).withArea(Property("header-row-start"), Property(1), Property("header-row-end"), Property(-1)),
		juce::GridItem(masterModule).withArea(Property("modules1-row-start"), Property("col2-start"),
		Property("modules1-row-end"), Property("col2-end")),

	};

	header.setColour(juce::Label::ColourIds::backgroundColourId, Util::createRandomColour());
	addAndMakeVisible(header);

	addAndMakeVisible(oscModuleHolder);
	addAndMakeVisible(filterModuleHolder);
	addAndMakeVisible(fxModuleHolder);
	addAndMakeVisible(envModuleHolder);
	addAndMakeVisible(lfoModuleHolder);

	panModule.setColour(juce::Label::ColourIds::backgroundColourId, Util::createRandomColour());
	addAndMakeVisible(panModule);

	masterModule.setColour(juce::Label::ColourIds::backgroundColourId, Util::createRandomColour());
	addAndMakeVisible(masterModule);

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
		fxModuleHolder.addModule(new SynthModule(audioProcessor, i));
	}
	for (auto i = 0; i < configuration::ENV_NUMBER; i++) {
		envModuleHolder.addModule(new EnvModule(audioProcessor, i));
	}
	for (auto i = 0; i < configuration::LFO_NUMBER; i++) {
		lfoModuleHolder.addModule(new LFOModule(audioProcessor, i));
	}
}

customGui::SynthModule::SynthModule(SynthAudioProcessor& audioProcessor, int id)
{
	juce::ignoreUnused(audioProcessor, id);
	mainGrid.autoFlow = juce::Grid::AutoFlow::row;
	mainGrid.autoRows = Track(Fr(5));
	mainGrid.autoColumns = Track(Fr(1));

	mainGrid.templateRows = { Track(Fr(1)), Track(Fr(5)), Track(Fr(5)), Track(Fr(1)), };

	mainGrid.templateColumns = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)), };

	mainGrid.items = {
		juce::GridItem(powerAndName).withArea(Property(1), Property(1)),
		juce::GridItem(dropDown).withArea(Property(1), Property(2), Property(1), Property(-1)),

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
	SynthModule(audioProcessor, id)
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
	comboBoxAttachments.add(new ComboBoxAttachment(apvts, prefix + configuration::WT_SUFFIX, dropDown)); 
	dropDown.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(prefix + configuration::WT_SUFFIX))->choices, 1);

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
	SynthModule(audioProcessor, id)
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
	comboBoxAttachments.add(new ComboBoxAttachment(apvts, prefix + configuration::FILTER_TYPE_SUFFIX, dropDown));
	dropDown.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(prefix + configuration::FILTER_TYPE_SUFFIX))->choices, 1);

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

	mainGrid.items.addArray({
		juce::GridItem(wtPosKnob).withArea(Property(2), Property(2)),
		juce::GridItem(rateKnob).withArea(Property(3), Property(2)),
		});

	addAndMakeVisible(wtPosKnob);
	addAndMakeVisible(rateKnob);
	
	// VALUE TREE ATTACHMENTS
	auto& apvts = audioProcessor.getApvts();

	comboBoxAttachments.add(new ComboBoxAttachment(apvts, prefix + configuration::WT_SUFFIX, dropDown));
	dropDown.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(prefix + configuration::WT_SUFFIX))->choices, 1);

	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::WT_POS_SUFFIX, wtPosKnob.slider));
	sliderAttachments.add(new SliderAttachment(apvts, prefix + configuration::RATE_SUFFIX, rateKnob.slider));
}


