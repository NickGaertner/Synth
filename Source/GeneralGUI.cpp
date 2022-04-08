
#include "GeneralGUI.h"

customGui::CustomLookAndFeel::CustomLookAndFeel()
{
	// PopupMenu
	setColour(juce::PopupMenu::backgroundColourId, Constants::background0Colour);
	setColour(juce::PopupMenu::textColourId, Constants::text0Colour);

	setColour(juce::PopupMenu::highlightedBackgroundColourId, Constants::background1Colour);
	setColour(juce::PopupMenu::highlightedTextColourId, Constants::text1Colour);

}

juce::Font customGui::CustomLookAndFeel::getComboBoxFont(juce::ComboBox& comboBox) {
	return { static_cast<float>(comboBox.getHeight()) };
}

juce::Font customGui::CustomLookAndFeel::getLabelFont(juce::Label& label) {
	return { static_cast<float>(label.getHeight()) };
}

void customGui::CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
	const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider)

{
	auto minDim = juce::jmin(width, height);
	auto bounds = juce::Rectangle<float>(x, y, width, height).withSizeKeepingCentre(minDim * 0.75f, minDim * 0.75f);
	auto radius = bounds.getWidth() / 2.f;
	auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

	const int numMeasureLines = 13;
	const float measureLineRadius = minDim / 2.f;
	for (int i = 0; i < numMeasureLines; i++) {
		auto lineAngle = rotaryStartAngle + (i / (numMeasureLines - 1.f)) * (rotaryEndAngle - rotaryStartAngle) - juce::MathConstants<float>::halfPi;
		juce::Path measureLinePath;
		juce::Point<float> endPoint{
			bounds.getCentreX() + measureLineRadius * std::cos(lineAngle),
			bounds.getCentreY() + measureLineRadius * std::sin(lineAngle)
		};
		juce::Line<float> line{ bounds.getCentre(), endPoint };
		measureLinePath.addLineSegment(line, 1.f);
		g.setColour(Constants::text1Colour);
		g.fillPath(measureLinePath);
	}
	Util::draw3DCircle(g, bounds, Constants::background0Colour);

	juce::Path indicatorPath;
	juce::Point<float> indicatorEndPoint{
		bounds.getCentreX() + (radius - Constants::seperatorSizePx) * std::cos(angle - juce::MathConstants<float>::halfPi),
		bounds.getCentreY() + (radius - Constants::seperatorSizePx) * std::sin(angle - juce::MathConstants<float>::halfPi)
	};
	juce::Line<float> indicatorLine{ bounds.getCentre(), indicatorEndPoint };
	indicatorPath.addLineSegment(indicatorLine, Constants::seperatorSizePx);
	indicatorPath = indicatorPath.createPathWithRoundedCorners(Constants::seperatorSizePx / 2.f);

	g.setColour(Constants::text0Colour);
	g.fillPath(indicatorPath);
}


void customGui::BypassedButton::paint(juce::Graphics& g)
{
	float roundedCornerSize = 0.f;
	auto* topLevelComp = getTopLevelComponent();
	if (topLevelComp) {
		roundedCornerSize = juce::jmin(topLevelComp->getWidth(), topLevelComp->getHeight()) * Constants::roundedCornerFactor;
	}
	else {
		jassertfalse;
	}

	roundedCornerSize *= 0.75f;
	//jassert(4.f * roundedCornerSize + 2.f * Constants::seperatorSizePx <= getWidth());
	jassert(2.f * roundedCornerSize + 1.f * Constants::seperatorSizePx <= getHeight());

	auto mainBounds = getLocalBounds().toFloat().withSizeKeepingCentre(getWidth() - 2.f * Constants::seperatorSizePx, roundedCornerSize * 2.f);
	g.setColour(!getToggleState() ? Constants::enabledColour : Constants::disabledColour);
	g.fillRoundedRectangle(mainBounds, roundedCornerSize);

	g.setColour(Constants::outlineColour);
	g.drawRoundedRectangle(mainBounds, roundedCornerSize, Constants::seperatorSizePx);

	auto circleBounds = mainBounds.withSizeKeepingCentre(roundedCornerSize * 2.f, roundedCornerSize * 2.f)
		.withX(mainBounds.getX() + (!getToggleState() ? mainBounds.getWidth() - roundedCornerSize * 2.f : 0.f));
	Util::draw3DCircle(g, circleBounds, Constants::background1Colour);

}

customGui::Dropdown::Dropdown()
{
	setColour(backgroundColourId, juce::Colours::transparentBlack);
	setColour(outlineColourId, juce::Colours::transparentBlack);
	setColour(textColourId, Constants::text0Colour);
	setColour(arrowColourId, Constants::text0Colour);
	setJustificationType(juce::Justification::centred);
}

void customGui::Dropdown::paint(juce::Graphics& g)
{
	g.setColour(Constants::text0Colour);
	g.fillRect(0.f, 0.f, Constants::seperatorSizePx / 2.f, static_cast<float>(getHeight()));
	ComboBox::paint(g);
}

customGui::ModSrcChooser::ModSrcChooser()
{
	setColour(backgroundColourId, Constants::background0Colour);
	setColour(outlineColourId, juce::Colours::transparentBlack);
	setColour(textColourId, Constants::text0Colour);
	setColour(arrowColourId, juce::Colours::transparentBlack);
	setJustificationType(juce::Justification::centred);
}

void customGui::ModSrcChooser::paint(juce::Graphics& g)
{
	float roundedCornerSize = 0.f;
	auto* topLevelComp = getTopLevelComponent();
	if (topLevelComp) {
		roundedCornerSize = juce::jmin(topLevelComp->getWidth(), topLevelComp->getHeight()) * Constants::roundedCornerFactor;
	}
	else {
		jassertfalse;
	}
	auto minDim = juce::jmin(getWidth(), getHeight());
	auto bounds = getLocalBounds().withSizeKeepingCentre(minDim, minDim).toFloat();

	g.setColour(Constants::background0Colour);
	g.fillRoundedRectangle(bounds, roundedCornerSize);

	juce::Colour textColour;
	juce::String text;
	auto selected = getText();
	if (selected.startsWith(configuration::ENV_PREFIX)) {
		textColour = Constants::envColour;
		text = selected.getLastCharacters(1);
	}
	else if (selected.startsWith(configuration::LFO_PREFIX)) {
		textColour = Constants::lfoColour;
		text = selected.getLastCharacters(1);
	}
	else {
		textColour = Constants::text0Colour;
		text = "-";
	}
	g.setColour(textColour);
	g.drawText(text, bounds.withSizeKeepingCentre(bounds.getWidth() * 0.75f, bounds.getHeight() * 0.75f), juce::Justification::centred);
}

void customGui::ModSrcChooser::resized()
{
}

customGui::Knob::Knob() : Slider(SliderStyle::RotaryVerticalDrag, TextEntryBoxPosition::NoTextBox)
{
}


customGui::NamedKnob::NamedKnob(const juce::String& labelText, bool displayValue)
	: nameLabel(juce::String(), labelText)
{
	nameLabel.setJustificationType(juce::Justification::centred);
	nameLabel.setColour(nameLabel.textColourId, Constants::text1Colour);
	mainVBox.items.add(juce::FlexItem(nameLabel).withFlex(labelFlex));
	mainVBox.items.add(juce::FlexItem(knob).withFlex(knobFlex));

	addAndMakeVisible(nameLabel);
	addAndMakeVisible(knob);
}

void customGui::NamedKnob::setLabelText(const juce::String& t_text) {
	nameLabel.setText(t_text, juce::NotificationType::sendNotification);
}

void customGui::NamedKnob::resized() {
	auto localBounds = getLocalBounds().toFloat();
	mainVBox.performLayout(localBounds);
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
	float roundedCornerSize = 0.f;
	auto* topLevelComp = getTopLevelComponent();
	if (topLevelComp) {
		roundedCornerSize = juce::jmin(topLevelComp->getWidth(), topLevelComp->getHeight()) * Constants::roundedCornerFactor;
	}
	else {
		jassertfalse;
	}

	g.setColour(Constants::background0Colour);
	g.fillRoundedRectangle(getLocalBounds().toFloat(), roundedCornerSize);

	g.setColour(Constants::text0Colour);
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

customGui::LevelMeter::LevelMeter(float& valueRef) : value(valueRef) {
	startTimerHz(30);
}

void customGui::LevelMeter::paint(juce::Graphics& g) {
	float roundedCornerSize = 0.f;
	auto* topLevelComp = getTopLevelComponent();
	if (topLevelComp) {
		roundedCornerSize = juce::jmin(topLevelComp->getWidth(), topLevelComp->getHeight()) * Constants::roundedCornerFactor;
	}
	else {
		jassertfalse;
	}
	auto bounds = getLocalBounds().toFloat();

	g.setColour(Constants::background0Colour);
	//g.fillRoundedRectangle(bounds, roundedCornerSize);
	g.fillRect(bounds);

	jassert(0.f <= value && value <= 1.f);
	auto  quantized = juce::roundToInt(value * granularity) / granularity;
	bounds = bounds.removeFromBottom(getHeight() * quantized);

	g.setGradientFill(gradient);
	g.fillRect(bounds);

	juce::Path path;
	bounds = getLocalBounds().toFloat();
	auto x = bounds.getX();
	auto y = bounds.getY();
	auto x2 = x + bounds.getWidth();
	auto y2 = y + bounds.getHeight();
	auto cs45 = roundedCornerSize * 0.45f;
	roundedCornerSize;

	path.startNewSubPath(x - 1, y - 1);
	path.lineTo(x, y + roundedCornerSize);
	path.cubicTo(x, y + cs45, x + cs45, y, x + roundedCornerSize, y);
	path.closeSubPath();

	path.startNewSubPath(x - 1, y2 + 1);
	path.lineTo(x + roundedCornerSize, y2);
	path.cubicTo(x + cs45, y2, x, y2 - cs45, x, y2 - roundedCornerSize);
	path.closeSubPath();

	path.startNewSubPath(x2 + 1, y - 1);
	path.lineTo(x2 - roundedCornerSize, y);
	path.cubicTo(x2 - cs45, y, x2, y + cs45, x2, y + roundedCornerSize);
	path.closeSubPath();

	path.startNewSubPath(x2 + 1, y2 + 1);
	path.lineTo(x2, y2 - roundedCornerSize);
	path.cubicTo(x2, y2 - cs45, x2 - cs45, y2, x2 - roundedCornerSize, y2);
	path.closeSubPath();

	g.setColour(Constants::background1Colour);
	g.fillPath(path);

	auto grillLineDistance = bounds.getHeight() / (numGrillSegments);
	auto grillLineThickness = grillLineDistance / 3.f;
	auto grillLinePos = bounds.getY() + grillLineDistance;
	g.setColour(Constants::background1Colour);
	while (grillLinePos < bounds.getY() + bounds.getHeight()) {
		g.fillRect(bounds.getX(), grillLinePos, bounds.getWidth(), grillLineThickness);
		grillLinePos += grillLineDistance;
	}
}

void customGui::LevelMeter::timerCallback() {
	repaint();
}

void customGui::LevelMeter::resized() {
	auto localBounds = getLocalBounds().toFloat();
	gradient = juce::ColourGradient{ Constants::enabledColour, localBounds.getBottomLeft(),
									Constants::disabledColour, localBounds.getTopLeft(),
									false };
	gradient.addColour((numGrillSegments - 2.f) / numGrillSegments, Constants::lfoColour);
	gradient.addColour(0.75f, Constants::lfoColour.interpolatedWith(Constants::enabledColour, 0.5f));

}

customGui::LevelDisplay::LevelDisplay() {
	vBox.items.addArray({
		juce::FlexItem(hBox).withFlex(8.f),
		juce::FlexItem(meterLabel).withFlex(1.f),
		});
	hBox.items.addArray({
		juce::FlexItem(meterLeft).withFlex(1.f).withMargin(juce::FlexItem::Margin(3.f)),
		juce::FlexItem(meterRight).withFlex(1.f).withMargin(juce::FlexItem::Margin(3.f)),
		});
	addAndMakeVisible(meterLeft);
	addAndMakeVisible(meterRight);
	addAndMakeVisible(meterLabel);
	meterLabel.setJustificationType(juce::Justification::centred);
	meterLabel.setColour(meterLabel.textColourId, Constants::text1Colour);
	smoothedLeft.reset(20);
	smoothedRight.reset(20);
}

void customGui::LevelDisplay::resized() {
	vBox.performLayout(getLocalBounds());
}

void customGui::LevelDisplay::updateLevel(float t_left, float t_right) {
	t_left = juce::jmap(juce::Decibels::gainToDecibels(t_left, -96.f), -96.f, 0.f, 0.f, 1.f);
	t_right = juce::jmap(juce::Decibels::gainToDecibels(t_right, -96.f), -96.f, 0.f, 0.f, 1.f);
	if (t_left > smoothedLeft.getCurrentValue()) {
		smoothedLeft.setCurrentAndTargetValue(t_left);
	}
	else {
		smoothedLeft.setTargetValue(t_left);
	}
	if (t_right > smoothedRight.getCurrentValue()) {
		smoothedRight.setCurrentAndTargetValue(t_right);
	}
	else {
		smoothedRight.setTargetValue(t_right);
	}
	levelLeft = smoothedLeft.getNextValue();
	levelRight = smoothedRight.getNextValue();
}

customGui::SynthModule::SynthModule(SynthAudioProcessor& audioProcessor, int id, int cols)
{
	juce::ignoreUnused(audioProcessor, id);

	// Main Box
	vBox.items.addArray({
		juce::FlexItem(headerHBox).withFlex(2.f),//.withMargin(FlexMargin(Constants::seperatorSizePx)),
		juce::FlexItem(gridComponent).withFlex(15.f).withMargin(FlexMargin(gridMarginFactor * Constants::seperatorSizePx)),
		});

	// Header
	headerHBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
	headerHBox.items.addArray({
		juce::FlexItem(bypassedButton).withFlex(1.f),
		juce::FlexItem(nameLabel).withFlex(2.f),
		juce::FlexItem(dropDown).withFlex(7.f),
		});
	nameLabel.setColour(nameLabel.textColourId, Constants::text0Colour);
	addAndMakeVisible(bypassedButton);
	addAndMakeVisible(nameLabel);
	addAndMakeVisible(dropDown);

	// Body
	auto& grid = gridComponent.grid;
	grid.autoFlow = juce::Grid::AutoFlow::row;
	grid.autoRows = Track(knobRowFr);
	grid.autoColumns = Track(Fr(1));
	//grid.setGap(Px(2.f));

	grid.templateRows = {
		Track("knobRow0-start",knobRowFr,"knobRow0-end"),
		Track("knobRow1-start",knobRowFr,"knobRow1-end"),
		Track("modSrcRow-start",modSrcRowFr,"modSrcRow-end"),
	};
	addAndMakeVisible(gridComponent);
}

void customGui::SynthModule::paint(juce::Graphics& g) {

	float roundedCornerSize = 0.f;
	auto* topLevelComp = getTopLevelComponent();
	if (topLevelComp) {
		roundedCornerSize = juce::jmin(topLevelComp->getWidth(), topLevelComp->getHeight()) * Constants::roundedCornerFactor;
	}
	else {
		jassertfalse;
	}

	auto gridBounds = gridComponent.getBoundsInParent().toFloat();
	gridBounds = gridBounds.withSizeKeepingCentre(
		gridBounds.getWidth() + gridMarginFactor * Constants::seperatorSizePx,
		gridBounds.getHeight() + gridMarginFactor * Constants::seperatorSizePx
	);

	g.setColour(Constants::background1Colour);
	g.fillRoundedRectangle(gridBounds, roundedCornerSize);

	g.setColour(Constants::outlineColour);
	g.drawRoundedRectangle(gridBounds, roundedCornerSize, Constants::seperatorSizePx);

}

void customGui::SynthModule::resized()
{
	vBox.performLayout(getLocalBounds());

}

customGui::ModuleHolder::ModuleHolder(int cols) {
	//grid.columnGap = Px(3); // TODO are these two needed?
	//grid.rowGap = Px(3);
	grid.autoFlow = juce::Grid::AutoFlow::row;
	grid.autoRows = Track(Fr(1));
	grid.autoColumns = Track(Fr(1));

	for (int i = 0; i < cols; i++) {
		grid.templateColumns.add(Track(Fr(1)));
	}
}

void customGui::ModuleHolder::addModule(SynthModule* module) {
	modules.add(module);
	grid.items.add(juce::GridItem(*module)); // TODO margin?
	addAndMakeVisible(*module);
}

void customGui::ModuleHolder::resized() {
	float roundedCornerSize = 0.f;
	auto* topLevelComp = getTopLevelComponent();
	if (topLevelComp) {
		roundedCornerSize = juce::jmin(topLevelComp->getWidth(), topLevelComp->getHeight()) * Constants::roundedCornerFactor;
	}
	else {
		jassertfalse;
	}
	auto gridBounds = getLocalBounds().withSizeKeepingCentre(getWidth() - 2 * roundedCornerSize, getHeight());// -2 * roundedCornerSize);
	grid.performLayout(gridBounds);
}

void customGui::ModuleHolder::paint(juce::Graphics& g)
{
	g.setColour(Constants::background0Colour);
	auto localBounds = getLocalBounds().toFloat();

	float roundedCornerSize = 0.f;
	auto* topLevelComp = getTopLevelComponent();
	if (topLevelComp) {
		roundedCornerSize = juce::jmin(topLevelComp->getWidth(), topLevelComp->getHeight()) * Constants::roundedCornerFactor;
	}
	else {
		jassertfalse;
	}
	g.fillRoundedRectangle(localBounds, roundedCornerSize);

}


