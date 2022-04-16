/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
SynthAudioProcessorEditor::SynthAudioProcessorEditor(SynthAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p), mainComponent(audioProcessor)
{
	setLookAndFeel(&lookAndFeel);

	addAndMakeVisible(mainComponent);
	setResizeLimits(950, 650, 1920, 1080);
	//setSize(950, 650);
	setResizable(true, false);
}

SynthAudioProcessorEditor::~SynthAudioProcessorEditor()
{
	setLookAndFeel(nullptr);
}

//==============================================================================
void SynthAudioProcessorEditor::paint(juce::Graphics& g)
{
	g.fillAll(customGui::Constants::outlineColour);
}

void SynthAudioProcessorEditor::resized()
{
	mainComponent.centreWithSize(getWidth(), getHeight());
}
