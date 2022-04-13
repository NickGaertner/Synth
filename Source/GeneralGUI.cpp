
#include "GeneralGUI.h"

customGui::CustomLookAndFeel::CustomLookAndFeel()
{
	// PopupMenu
	setColour(juce::PopupMenu::backgroundColourId, Constants::background0Colour);
	setColour(juce::PopupMenu::textColourId, Constants::text0Colour);

	setColour(juce::PopupMenu::highlightedBackgroundColourId, Constants::background1Colour);
	setColour(juce::PopupMenu::highlightedTextColourId, Constants::text1Colour);

	// TextButton
	setColour(juce::TextButton::ColourIds::buttonColourId, Constants::background0Colour);
	setColour(juce::TextButton::ColourIds::buttonOnColourId, Constants::background0Colour);
	setColour(juce::TextButton::ColourIds::textColourOffId, Constants::text0Colour);
	setColour(juce::TextButton::ColourIds::textColourOnId, Constants::text0Colour);

	setColour(juce::ComboBox::ColourIds::outlineColourId, Constants::background1Colour);
}

juce::Font customGui::CustomLookAndFeel::getComboBoxFont(juce::ComboBox& comboBox) {
	return { static_cast<float>(comboBox.getHeight()) };
}

juce::Font customGui::CustomLookAndFeel::getLabelFont(juce::Label& label) {
	return { static_cast<float>(label.getHeight()) };
}

juce::Font customGui::CustomLookAndFeel::getTextButtonFont(juce::TextButton& textButton, int buttonHeight)
{
	juce::ignoreUnused(textButton);
	return {buttonHeight*0.8f};
}

void customGui::CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
	const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider)

{
	juce::ignoreUnused(slider);
	float minDim = static_cast<float>(juce::jmin(width, height));
	auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height))
		.withSizeKeepingCentre(minDim * 0.75f, minDim * 0.75f);
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

	auto mainBounds = getLocalBounds().toFloat().withSizeKeepingCentre(getWidth() - 2.f * Constants::seperatorSizePx, roundedCornerSize * 2.f);
	g.setColour(!getToggleState() ? Constants::enabledColour : Constants::disabledColour);
	g.fillRoundedRectangle(mainBounds, roundedCornerSize);

	g.setColour(Constants::outlineColour);
	g.drawRoundedRectangle(mainBounds, roundedCornerSize, Constants::seperatorSizePx);

	auto circleBounds = mainBounds.withSizeKeepingCentre(roundedCornerSize * 2.f, roundedCornerSize * 2.f)
		.withX(mainBounds.getX() + (!getToggleState() ? mainBounds.getWidth() - roundedCornerSize * 2.f : 0.f));
	Util::draw3DCircle(g, circleBounds, Constants::background1Colour);

}

customGui::Dropdown::Dropdown(bool t_useSeperatorLine)
	: useSeperatorLine{ t_useSeperatorLine }
{
	setColour(backgroundColourId, juce::Colours::transparentBlack);
	setColour(outlineColourId, juce::Colours::transparentBlack);
	setColour(textColourId, Constants::text0Colour);
	setColour(arrowColourId, Constants::text0Colour);
	setJustificationType(juce::Justification::centred);
}

void customGui::Dropdown::paint(juce::Graphics& g)
{
	if (useSeperatorLine) {
		g.setColour(Constants::text0Colour);
		g.fillRect(0.f, 0.f, Constants::seperatorSizePx / 2.f, static_cast<float>(getHeight()));
	}
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
	float roundedCornerSize = Util::getCornerSize(this);
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

customGui::Knob::Knob(bool rotary) : Slider(
	rotary ? SliderStyle::RotaryVerticalDrag : SliderStyle::IncDecButtons,
	rotary ? TextEntryBoxPosition::NoTextBox : TextEntryBoxPosition::TextBoxAbove)
{
	if (rotary) {
	}
	else {
		setIncDecButtonsMode(IncDecButtonMode::incDecButtonsDraggable_Vertical);
		setNumDecimalPlacesToDisplay(2);
		setTextBoxIsEditable(true);
		setColour(textBoxTextColourId, Constants::text0Colour);
		setColour(textBoxBackgroundColourId, Constants::background0Colour);
		setColour(textBoxHighlightColourId, Constants::background1Colour);
		setColour(textBoxOutlineColourId, Constants::background0Colour);
	}
}

void customGui::Knob::resized()
{
	Slider::resized();
	setTextBoxStyle(getTextBoxPosition(), false, getWidth(), getHeight() / 2);
}


customGui::NamedKnob::NamedKnob(const juce::String& labelText, bool rotary)
	: nameLabel(juce::String(), labelText), knob(rotary)
{
	nameLabel.setJustificationType(juce::Justification::centred);
	nameLabel.setColour(nameLabel.textColourId, Constants::text1Colour);
	mainVBox.items.add(juce::FlexItem(nameLabel).withFlex(LABEL_FLEX));
	mainVBox.items.add(juce::FlexItem(knob).withFlex(KNOB_FLEX));

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

customGui::MenuButton::MenuButton(const juce::String& buttonName)
	: Button{ buttonName }
{
}

void customGui::MenuButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	juce::ignoreUnused(shouldDrawButtonAsDown, shouldDrawButtonAsHighlighted);
	float roundedCornerSize = Util::getCornerSize(this);

	auto bounds = getLocalBounds().toFloat();
	juce::Colour backgroundColour, textColour;
	switch (getState()) {
	case buttonNormal:
		backgroundColour = Constants::background0Colour;
		textColour = Constants::text0Colour;
		break;
	case buttonOver:
		backgroundColour = Constants::background1Colour;
		textColour = Constants::text1Colour;
		break;
	case buttonDown:
		backgroundColour = Constants::background1Colour.darker(0.2f);
		textColour = Constants::text1Colour.darker(0.2f);
		break;
	default:
		jassertfalse;
		break;
	}
	bounds = bounds.withSizeKeepingCentre(bounds.getWidth() - Constants::seperatorSizePx,
		bounds.getHeight() - Constants::seperatorSizePx);

	Util::draw3DRoundedRectangle(g, bounds, backgroundColour, roundedCornerSize);

	g.setColour(textColour);
	g.setFont(juce::Font(bounds.getHeight() * 0.55f, juce::Font::FontStyleFlags::bold));
	g.drawFittedText(getButtonText(), getLocalBounds(), juce::Justification::centred, 1);
}

customGui::HeaderMenu::HeaderMenu(SynthAudioProcessor& t_audioProcessor)
	: audioProcessor(t_audioProcessor)
{
	auto horizontalMargin = SynthModule::GRID_MARGIN_FACTOR * Constants::seperatorSizePx / 2.f;
	mainHBox.items.addArray({
		juce::FlexItem(buttonGrid).withFlex(1.f)
		.withMargin(FlexMargin(0.f,horizontalMargin,0.f,horizontalMargin)),
		juce::FlexItem(label).withFlex(1.f)
		.withMargin(FlexMargin(0.f,horizontalMargin,0.f,horizontalMargin)),
		juce::FlexItem(miscHBox).withFlex(1.f)
		.withMargin(FlexMargin(0.f,horizontalMargin,0.f,horizontalMargin)),
		});

	miscHBox.items.addArray({
		juce::FlexItem(panicButton).withFlex(1.f)
		});
	addAndMakeVisible(panicButton);

	label.setJustificationType(juce::Justification::centred);
	label.setColour(label.textColourId, Constants::text1Colour);
	label.setText(audioProcessor.presetName, juce::NotificationType::sendNotification);
	addAndMakeVisible(label);

	auto& grid = buttonGrid.grid;
	grid.autoRows = Track(Fr(1));
	grid.autoColumns = Track(Fr(1));
	grid.rowGap = Px(Constants::seperatorSizePx);
	grid.columnGap = Px(3 * Constants::seperatorSizePx);
	grid.items.addArray({
		juce::GridItem(newButton).withArea(Property(1),Property(1)),
		juce::GridItem(openButton).withArea(Property(),Property(2)),
		juce::GridItem(saveButton).withArea(Property(1),Property(3)),
		juce::GridItem(saveAsButton).withArea(Property(),Property(4)),
		});

	addAndMakeVisible(newButton);
	addAndMakeVisible(openButton);
	addAndMakeVisible(saveButton);
	addAndMakeVisible(saveAsButton);

	panicButton.onClick = [this]() {
		audioProcessor.panicReset();
	};

	juce::String userAppDirPath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory)
		.getFullPathName();
	presetDir = std::make_unique<juce::File>(userAppDirPath
		+ juce::File::getSeparatorChar() + configuration::MAIN_FOLDER_NAME
		+ juce::File::getSeparatorChar() + configuration::PRESET_FOLDER_NAME);
	if (!presetDir->isDirectory()) {
		if (presetDir->createDirectory().failed()) {
			jassertfalse;
		}
	}

	newButton.onClick = [this]() {
		audioProcessor.getApvts().replaceState(juce::ValueTree(configuration::VALUE_TREE_IDENTIFIER));
		label.setText("Untitled", juce::NotificationType::sendNotification);
		audioProcessor.presetName = "Untitled";
	};

	openButton.onClick = [this]() {
		presetFileChooser = std::make_unique<juce::FileChooser>("Open a preset",
			*presetDir,
			"*.xml");
		auto flags = juce::FileBrowserComponent::openMode
			| juce::FileBrowserComponent::canSelectFiles;

		presetFileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
			{
				if (fc.getResult() == juce::File{})
					return;

				juce::MemoryBlock data;

				if (fc.getResult().loadFileAsData(data)) {
					audioProcessor.setStateInformation(data.getData(), (int)data.getSize());
					label.setText(fc.getResult().getFileNameWithoutExtension(), juce::NotificationType::sendNotification);
					audioProcessor.presetName = fc.getResult().getFileNameWithoutExtension();
				}
				else {
					juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
						TRANS("Error whilst loading"),
						TRANS("Couldn't read from the specified file!"));
				}
			});
	};
	saveButton.onClick = [this]() {
		if (label.getText() == "Untitled") {
			saveAsButton.onClick();
			return;
		}
		auto file = presetDir->getChildFile(label.getText() + ".xml");
		if (!file.existsAsFile()) {
			saveAsButton.onClick();
			return;
		}
		juce::MemoryBlock data;
		audioProcessor.getStateInformation(data);
		if (!file.replaceWithData(data.getData(), data.getSize())) {
			juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
				TRANS("Error whilst saving"),
				TRANS("Couldn't write to the specified file!"));
		}

	};
	saveAsButton.onClick = [this]() {
		presetFileChooser = std::make_unique<juce::FileChooser>("Save current preset",
			*presetDir,
			"*.xml");
		auto flags = juce::FileBrowserComponent::saveMode
			| juce::FileBrowserComponent::canSelectFiles
			| juce::FileBrowserComponent::warnAboutOverwriting;

		presetFileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
			{
				if (fc.getResult() == juce::File{})
					return;

				juce::MemoryBlock data;
				audioProcessor.getStateInformation(data);
				if (!fc.getResult().replaceWithData(data.getData(), data.getSize())) {
					juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
						TRANS("Error whilst saving"),
						TRANS("Couldn't write to the specified file!"));
				}
				else {
					label.setText(fc.getResult().getFileNameWithoutExtension(), juce::NotificationType::sendNotification);
					audioProcessor.presetName = fc.getResult().getFileNameWithoutExtension();
				}
			});
	};
}

customGui::HeaderMenu::~HeaderMenu()
{
}

void customGui::HeaderMenu::resized()
{
	float roundedCornerSize = Util::getCornerSize(this);
	mainHBox.performLayout(getLocalBounds().toFloat().withSizeKeepingCentre(
		getWidth() - 2.f * roundedCornerSize,
		getHeight() - 2.f * roundedCornerSize));
}

void customGui::HeaderMenu::paint(juce::Graphics& g)
{
	float roundedCornerSize = Util::getCornerSize(this);
	auto bounds = getLocalBounds().toFloat();
	g.setColour(Constants::background1Colour);
	g.fillRoundedRectangle(bounds, roundedCornerSize);
}

customGui::SpectrumAnalyzer::SpectrumAnalyzer(SynthAudioProcessor& t_audioProcessor)
	:audioProcessor(t_audioProcessor)
{
	setFFTOrder(14);
	setBlockNumber(8);
	jassert(!audioProcessor.observationCallback);
	audioProcessor.observationCallback = [&](const juce::dsp::AudioBlock<float>& b) {readBlock(b); };
	startTimerHz(30);
}

void customGui::SpectrumAnalyzer::paint(juce::Graphics& g)
{
	float roundedCornerSize = Util::getCornerSize(this);

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
	jassert(MIN_FFT_ORDER <= t_order && t_order <= MAX_FFT_ORDER);
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
	for (int i = 1; i < SCOPE_SIZE; i++) {
		g.drawLine({
			(float)juce::jmap(i - 1, 0, SCOPE_SIZE - 1, 0, width),
			juce::jmap(scopeData[i - 1], 0.0f, 1.0f, (float)height, 0.0f),
			(float)juce::jmap(i, 0, SCOPE_SIZE - 1, 0, width),
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

	for (int i = 0; i < SCOPE_SIZE; i++) {
		auto t = (float)i / (float)SCOPE_SIZE;
		auto exponent = (offset + t * (1.f - offset)) * halfSizeLog;
		int fftDataIndex = static_cast<int>(std::exp2f(exponent));
		auto level = fftData[fftDataIndex] / (max + 1);
		level = juce::jmap(juce::Decibels::gainToDecibels(level), minDB, maxDB, 0.f, 1.f);

		auto t2 = (float)(i + 0.5f) / (float)SCOPE_SIZE;
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
	float roundedCornerSize = Util::getCornerSize(this);
	auto bounds = getLocalBounds().toFloat();

	g.setColour(Constants::background0Colour);
	g.fillRect(bounds);

	jassert(0.f <= value && value <= 1.f);
	auto  quantized = juce::roundToInt(value * GRANULARITY) / GRANULARITY;
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

	auto grillLineDistance = bounds.getHeight() / (NUM_GRILL_SEGMENTS);
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
	gradient.addColour((NUM_GRILL_SEGMENTS - 2.f) / NUM_GRILL_SEGMENTS, Constants::lfoColour);
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
	juce::ignoreUnused(audioProcessor, id, cols);

	// Main Box
	vBox.items.addArray({
		juce::FlexItem(headerHBox).withFlex(2.f),
		juce::FlexItem(gridComponent).withFlex(15.f).withMargin(FlexMargin(GRID_MARGIN_FACTOR * Constants::seperatorSizePx)),
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
	grid.autoRows = Track(KNOB_ROW_FR);
	grid.autoColumns = Track(Fr(1));
	//grid.setGap(Px(2.f));

	grid.templateRows = {
		Track("knobRow0-start",KNOB_ROW_FR,"knobRow0-end"),
		Track("knobRow1-start",KNOB_ROW_FR,"knobRow1-end"),
		Track("modSrcRow-start",MOD_ROW_FR,"modSrcRow-end"),
	};
	addAndMakeVisible(gridComponent);
}

void customGui::SynthModule::paint(juce::Graphics& g) {

	float roundedCornerSize = Util::getCornerSize(this);

	auto gridBounds = gridComponent.getBoundsInParent().toFloat();
	gridBounds = gridBounds.withSizeKeepingCentre(
		gridBounds.getWidth() + GRID_MARGIN_FACTOR * Constants::seperatorSizePx,
		gridBounds.getHeight() + GRID_MARGIN_FACTOR * Constants::seperatorSizePx
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

void customGui::SynthModule::deleteAllAttachments()
{
	buttonAttachments.clear();
	comboBoxAttachments.clear();
	sliderAttachments.clear();
}

customGui::ModuleHolder::ModuleHolder(int cols) {
	grid.autoFlow = juce::Grid::AutoFlow::row;
	grid.autoRows = Track(Fr(1));
	grid.autoColumns = Track(Fr(1));

	for (int i = 0; i < cols; i++) {
		grid.templateColumns.add(Track(Fr(1)));
	}
}

void customGui::ModuleHolder::addModule(SynthModule* module) {
	modules.add(module);
	grid.items.add(juce::GridItem(*module));
	addAndMakeVisible(*module);
}

void customGui::ModuleHolder::resized() {
	float roundedCornerSize = Util::getCornerSize(this);
	auto gridBounds = getLocalBounds().withSizeKeepingCentre(static_cast<int>(getWidth() - 2 * roundedCornerSize), getHeight());
	grid.performLayout(gridBounds);
}

void customGui::ModuleHolder::paint(juce::Graphics& g)
{
	float roundedCornerSize = Util::getCornerSize(this);
	
	auto localBounds = getLocalBounds().toFloat();
	g.setColour(Constants::background0Colour);
	g.fillRoundedRectangle(localBounds, roundedCornerSize);
}


