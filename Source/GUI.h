
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Configuration.h"

namespace customGui {

	using Fr = juce::Grid::Fr;
	using Px = juce::Grid::Px;
	using Track = juce::Grid::TrackInfo;
	using Property = juce::GridItem::Property;
	using StartAndEndProperty = juce::GridItem::StartAndEndProperty;
	using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
	using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
	using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

	class Util {
	public:
		inline static const juce::Grid createStretchGrid() {
			juce::Grid grid;
			grid.justifyItems = juce::Grid::JustifyItems::stretch;
			grid.alignItems = juce::Grid::AlignItems::stretch;
			grid.justifyContent = juce::Grid::JustifyContent::stretch;
			grid.alignContent = juce::Grid::AlignContent::stretch;
			return grid;
		}

		inline static const juce::FlexBox createHBox() {
			return juce::FlexBox{
		juce::FlexBox::Direction::row,
		juce::FlexBox::Wrap::noWrap,
		juce::FlexBox::AlignContent::stretch,
		juce::FlexBox::AlignItems::stretch,
		juce::FlexBox::JustifyContent::spaceAround };
		}

		inline static const juce::FlexBox createVBox() {
			return juce::FlexBox{
			juce::FlexBox::Direction::column,
			juce::FlexBox::Wrap::noWrap,
			juce::FlexBox::AlignContent::stretch,
			juce::FlexBox::AlignItems::stretch,
			juce::FlexBox::JustifyContent::spaceAround };
		}

		inline static const juce::Colour createRandomColour() {
			return juce::Colour::fromRGB((juce::uint8)juce::Random::getSystemRandom().nextInt(255),
				(juce::uint8)juce::Random::getSystemRandom().nextInt(255),
				(juce::uint8)juce::Random::getSystemRandom().nextInt(255)
			);
		}

		inline static void centerSquare(juce::Component& component) {
			auto newBounds = component.getBounds();
			auto centre = newBounds.getCentre();
			auto min = juce::jmin(newBounds.getWidth(), newBounds.getHeight());
			component.setSize(min, min);
			component.setCentrePosition(centre);
		}

	};

	class NamedKnob : public juce::Component {
	public:
		NamedKnob(const juce::String& labelText = juce::String("Name"), bool displayValue = false) :
			nameLabel(juce::String(), labelText),
			slider(juce::Slider::SliderStyle::RotaryVerticalDrag,
				displayValue ? juce::Slider::TextEntryBoxPosition::TextBoxBelow : juce::Slider::TextEntryBoxPosition::NoTextBox)
		{
			nameLabel.setJustificationType(juce::Justification::centred);
			addAndMakeVisible(nameLabel);
			mainVBox.items.add(juce::FlexItem(nameLabel).withFlex(0.2f));

			addAndMakeVisible(slider);
			mainVBox.items.add(juce::FlexItem(slider).withFlex(1.f));
		}
		virtual void setLabelText(const juce::String& t_text) {
			nameLabel.setText(t_text, juce::NotificationType::sendNotification);
		}
		virtual void resized() override {
			mainVBox.performLayout(getLocalBounds());
		}

		juce::FlexBox mainVBox = Util::createVBox();
		juce::Slider slider;

	private:
		juce::Label nameLabel;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamedKnob)
	};

	class ModSrcChooser : public juce::ComboBox {
	public:
		ModSrcChooser(const juce::String& componentName = {}) : ComboBox(componentName) {

		}
		virtual void resized() override {
			Util::centerSquare(*this);
		}
	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModSrcChooser)
	};

	// TODO change to ImageButton
	class PowerButton : public  juce::ToggleButton {
		using ToggleButton::ToggleButton;
	public:

		virtual void resized() override {
			//Util::centerSquare(*this);
		}

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PowerButton)
	};

	//class BasicWaveformChooser : public juce::Component {

	//public:
	//	BasicWaveformChooser() {
	//		for (int i = 0; i < std::size(buttons); i++) {
	//			addAndMakeVisible(*buttons[i]);
	//			buttons[i]->setRadioGroupId(1);
	//			buttons[i]->onClick = [=]() {respondToClick(i); };
	//		}

	//		mainGrid.setGap(Px(0));
	//		mainGrid.templateRows = { Track(Fr(1)), Track(Fr(1)) };
	//		mainGrid.templateColumns = { Track(Fr(1)), Track(Fr(1)) };

	//		mainGrid.items.addArray({
	//			juce::GridItem(sineButton).withArea(Property(1), Property(1)),
	//			juce::GridItem(squareButton).withArea(Property(1), Property(2)),
	//			juce::GridItem(triangleButton).withArea(Property(2), Property(1)),
	//			juce::GridItem(sawButton).withArea(Property(2), Property(2)),
	//			});
	//	}

	//	virtual void resized() override {
	//		mainGrid.performLayout(getLocalBounds());
	//	}

	//	void respondToClick(int buttonNumber) {
	//		if (buttons[buttonNumber]->getToggleState()) {
	//			parameterAttachment->setValueAsCompleteGesture((float)buttonNumber);
	//		}
	//	}

	//	void parameterChanged(float value) {
	//		auto index = juce::roundToInt(value);
	//		jassert(0 <= index && index < std::size(buttons));
	//		buttons[index]->triggerClick();
	//	}

	//	void attachToParameter(juce::RangedAudioParameter& parameter) {
	//		parameterAttachment = std::make_unique<juce::ParameterAttachment>(parameter, [&](float x) {parameterChanged(x); });
	//		parameterAttachment->sendInitialUpdate();
	//	}

	//private:
	//	juce::Grid mainGrid = Util::createStretchGrid();
	//	// TODO change to ImageButtons
	//	juce::ToggleButton sineButton{ "Sine" };
	//	juce::ToggleButton squareButton{ "Square" };
	//	juce::ToggleButton triangleButton{ "Triangle" };
	//	juce::ToggleButton sawButton{ "Saw" };
	//	juce::ToggleButton* buttons[4]{ &sineButton, &squareButton, &triangleButton, &sawButton };
	//	std::unique_ptr<juce::ParameterAttachment> parameterAttachment;

	//	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BasicWaveformChooser)
	//};

	class SpectrumAnalyzer : public juce::Component, private juce::Timer {
	public:

		SpectrumAnalyzer(SynthAudioProcessor& t_audioProcessor);
		virtual void paint(juce::Graphics& g) override;
		virtual void resized() override;
		virtual void timerCallback() override;
		void readBlock(const juce::dsp::AudioBlock<float>& audioBlock);

		void setBypassed(bool t_bypassed) {
			bypassed = t_bypassed;
		}

		void setFFTOrder(int t_order);

		void setBlockNumber(int t_numBlocks);

		int getFFTOrder() {
			return fftOrder;
		}
		int getBlockNumber() {
			return numBlocks;
		}
	private:
		void drawFrame(juce::Graphics& g);

		void updateSpectrum();

		bool bypassed = false;
		int fftOrder;
		static inline const int minFFTOrder = 11;
		static inline const int maxFFTOrder = 15;
		int fftSize;
		static inline const int maxFFTSize = 1 << maxFFTOrder;
		static inline const int scopeSize = 512;

		juce::dsp::FFT forwardFFT{ 1 };
		std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

		float fifo[maxFFTSize]{ 0.f };
		int fifoBlockOffset;
		int numBlocks = 8;
		int fifoBlockSize;
		int fifoLocalIndex;

		float fftData[2 * maxFFTSize]{ 0.f };
		bool nextFFTBlockReady = false;
		float scopeData[scopeSize]{ 0.f };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
	};

	class SynthModule : public juce::Component {
	public:
		SynthModule() = delete;
		SynthModule(SynthAudioProcessor& audioProcessor, int id, int cols = 2);
		virtual ~SynthModule() override {};

		virtual void paint(juce::Graphics& g) override;
		virtual void resized() override;

	protected:
		juce::Grid mainGrid = Util::createStretchGrid();

		class PowerAndName : public Component {
		public:
			// TODO make the powerButton quadratic and add image
			PowerAndName() {
				hBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
				hBox.items.addArray({
					juce::FlexItem(powerButton).withFlex(1.f),
					juce::FlexItem(nameLabel).withFlex(3.f),
					});
				addAndMakeVisible(powerButton);
				addAndMakeVisible(nameLabel);
			}

			void resized() {
				hBox.performLayout(getLocalBounds());
			}
			juce::FlexBox hBox = Util::createHBox();
			PowerButton powerButton;
			juce::Label nameLabel;
		} powerAndName;

		juce::ComboBox dropDown;
		juce::OwnedArray<ButtonAttachment> buttonAttachments;
		juce::OwnedArray<ComboBoxAttachment> comboBoxAttachments;
		juce::OwnedArray<SliderAttachment> sliderAttachments;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthModule)
	};

	class ModuleHolder : public juce::Component {
	public:
		ModuleHolder(int cols = 1) {
			grid.autoFlow = juce::Grid::AutoFlow::row;
			grid.autoRows = Track(Fr(1));
			grid.autoColumns = Track(Fr(1));

			//ainGrid.templateRows = { Track(Fr(1)), Track(Fr(5)), Track(Fr(5)), Track(Fr(1)), };
			for (int i = 0; i < cols; i++) {
				grid.templateColumns.add(Track(Fr(1)));

			}
		}
		void addModule(SynthModule* module) {
			modules.add(module);
			grid.items.add(juce::GridItem(*module));
			addAndMakeVisible(*module);
		}

		virtual void resized() override {
			grid.performLayout(getLocalBounds());
		}
	private:
		juce::OwnedArray<SynthModule> modules;
		juce::Grid grid = Util::createStretchGrid();

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleHolder)
	};

	class OscModule : public SynthModule {
	public:
		OscModule() = delete;
		OscModule(SynthAudioProcessor& audioProcessor, int id);
		virtual ~OscModule() override {}

	protected:
		NamedKnob wtPosKnob{ "WtPos" };
		NamedKnob wtPosModKnob{ "Mod" };
		ModSrcChooser wtPosModSrcChooser;

		NamedKnob pitchKnob{ "Pitch" };
		NamedKnob pitchModKnob{ "Mod" };
		ModSrcChooser pitchModSrcChooser;

		NamedKnob gainKnob{ "Gain" };
		NamedKnob gainModKnob{ "Mod" };
		ModSrcChooser gainModSrcChooser;

		ModSrcChooser envChooser;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscModule)
	};

	class FilterModule : public SynthModule {
	public:
		FilterModule() = delete;
		FilterModule(SynthAudioProcessor& audioProcessor, int id);
		virtual ~FilterModule() override {}

	protected:
		NamedKnob cutoffKnob{ "Cutoff" };
		NamedKnob cutoffModKnob{ "Mod" };
		ModSrcChooser cutoffModSrcChooser;

		NamedKnob resonanceKnob{ "Resonance" };
		NamedKnob resonanceModKnob{ "Mod" };
		ModSrcChooser resonanceModSrcChooser;

		NamedKnob specialKnob{ "Special" };
		NamedKnob specialModKnob{ "Mod" };
		ModSrcChooser specialModSrcChooser;
	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterModule)
	};

	class FXModule : public SynthModule {
	public:
		FXModule() = delete;
		FXModule(SynthAudioProcessor& audioProcessor, int id);
		virtual ~FXModule() override {}

	protected:
		NamedKnob dryWetKnob{ "Dry-Wet" };
		NamedKnob dryWetModKnob{ "Mod" };
		ModSrcChooser dryWetModSrcChooser;

		NamedKnob parameter0Knob{ "Parameter 0" };
		NamedKnob parameter0ModKnob{ "Mod" };
		ModSrcChooser parameter0ModSrcChooser;

		NamedKnob parameter1Knob{ "Parameter 1" };
		NamedKnob parameter1ModKnob{ "Mod" };
		ModSrcChooser parameter1ModSrcChooser;

		NamedKnob parameter2Knob{ "Parameter 2" };
		NamedKnob parameter2ModKnob{ "Mod" };
		ModSrcChooser parameter2ModSrcChooser;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXModule)
	};

	class EnvModule : public SynthModule {
	public:
		EnvModule() = delete;
		EnvModule(SynthAudioProcessor& audioProcessor, int id);
		virtual ~EnvModule() override {}

	protected:
		NamedKnob attackKnob{ "Attack" };
		NamedKnob decayKnob{ "Decay" };
		NamedKnob sustainKnob{ "Sustain" };
		NamedKnob releaseKnob{ "Release" };

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvModule)
	};

	class LFOModule : public SynthModule {
	public:
		LFOModule() = delete;
		LFOModule(SynthAudioProcessor& audioProcessor, int id);
		virtual ~LFOModule() override {}

	protected:
		NamedKnob wtPosKnob{ "WtPos" };
		NamedKnob rateKnob{ "Rate" };

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOModule)
	};

	class PanModule : public SynthModule {
	public:
		PanModule() = delete;
		PanModule(SynthAudioProcessor& audioProcessor, int id = 0);
		virtual ~PanModule() override {}

	protected:
		NamedKnob panKnob{ "Pan" };
		NamedKnob panModKnob{ "Mod" };
		ModSrcChooser panModSrcChooser;
	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanModule)
	};

	// inspired by https://github.com/Thrifleganger/level-meter-demo
	class LevelMeter : public juce::Component, private juce::Timer {
	public:
		LevelMeter(float& valueRef) : value(valueRef) {
			startTimerHz(30);
		}

		virtual void paint(juce::Graphics& g) override {
			g.setColour(juce::Colours::black);
			g.fillRect(getLocalBounds());
			g.setGradientFill(gradient);
			jassert(0.f <= value && value <= 1.f);
			auto granularity = 100.f;
			auto  quantized = juce::roundToInt(value * granularity) / granularity;
			g.fillRect(getLocalBounds().removeFromBottom(getHeight() * quantized));
		}

		virtual void timerCallback() override {
			repaint();
		}
		void resized() {
			auto localBounds = getLocalBounds().toFloat();
			gradient = juce::ColourGradient{ juce::Colours::green, localBounds.getBottomLeft(),
											juce::Colours::red, localBounds.getTopLeft(),
											false };
			gradient.addColour(0.5, juce::Colours::greenyellow);
			gradient.addColour(0.9, juce::Colours::yellow);
		}
	private:
		float& value;
		juce::ColourGradient gradient;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
	};
	class MasterModule : public SynthModule {
	public:
		MasterModule() = delete;
		MasterModule(SynthAudioProcessor& audioProcessor, int id = 0);
		virtual ~MasterModule() override {}

	protected:
		class LevelDisplay : public Component {
		public:
			LevelDisplay() {
				vBox.items.addArray({
					juce::FlexItem(hBox).withFlex(10.f),//.withMargin(juce::FlexItem::Margin(3.f)),
					juce::FlexItem(meterLabel).withFlex(1.f),//.withMargin(juce::FlexItem::Margin(3.f)),
					});
				hBox.items.addArray({
					juce::FlexItem(meterLeft).withFlex(1.f).withMargin(juce::FlexItem::Margin(3.f)),
					juce::FlexItem(meterRight).withFlex(1.f).withMargin(juce::FlexItem::Margin(3.f)),
					});
				addAndMakeVisible(meterLeft);
				addAndMakeVisible(meterRight);
				addAndMakeVisible(meterLabel);
				meterLabel.setJustificationType(juce::Justification::centred);
				smoothedLeft.reset(20);
				smoothedRight.reset(20);
			}

			void resized() {
				vBox.performLayout(getLocalBounds());
			}

			void updateLevel(float t_left, float t_right) {
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
			juce::FlexBox vBox = Util::createVBox();
			juce::FlexBox hBox = Util::createHBox();
			juce::Label meterLabel{ "","Peak in dB" };
			float levelLeft{ 0.f }, levelRight{ 0.f };
			juce::SmoothedValue<float> smoothedLeft;
			juce::SmoothedValue<float> smoothedRight;
			LevelMeter meterLeft{ levelLeft };
			LevelMeter meterRight{ levelRight };
		} levelDisplay;

		NamedKnob masterKnob{ "Master" };

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterModule)
	};

	class SpectrumAnalyzerModule : public SynthModule {
	public:
		SpectrumAnalyzerModule() = delete;
		SpectrumAnalyzerModule(SynthAudioProcessor& audioProcessor, int id = 0);
		virtual ~SpectrumAnalyzerModule() override {};
	protected:
		SpectrumAnalyzer analyzer;
		NamedKnob fftOrderKnob{ "FFT Order" };
		NamedKnob refreshRateKnob{ "Refresh Rate" };
	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerModule);
	};
	class SynthComponent : public juce::Component {
	public:
		SynthComponent() = delete;
		SynthComponent(SynthAudioProcessor& audioProcessor);
		virtual ~SynthComponent() override;

		void paint(juce::Graphics&) override;
		void resized() override;
	private:
		void initModules(SynthAudioProcessor& audioProcessor);

		juce::Grid mainGrid = Util::createStretchGrid();

		juce::Label header;
		ModuleHolder oscModuleHolder;
		ModuleHolder filterModuleHolder;
		ModuleHolder fxModuleHolder;
		ModuleHolder envModuleHolder{ 2 };
		ModuleHolder lfoModuleHolder{ 2 };
		PanModule panModule;
		MasterModule masterModule;
		SpectrumAnalyzerModule spectrumModule;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthComponent)
	};


	class CustomLookAndFeel : public juce::LookAndFeel_V4 {
	public:
		CustomLookAndFeel() {};
	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLookAndFeel)
	};
}