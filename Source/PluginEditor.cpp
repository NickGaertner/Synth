/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
SynthAudioProcessorEditor::SynthAudioProcessorEditor(SynthAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p), synthComponent(audioProcessor)
{
	setLookAndFeel(&lookAndFeel);

	addAndMakeVisible(synthComponent);
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

	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SynthAudioProcessorEditor::resized()
{
	synthComponent.centreWithSize(getWidth(), getHeight());
}
