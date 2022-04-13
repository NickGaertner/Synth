
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace customGui {

	using Fr = juce::Grid::Fr;
	using Px = juce::Grid::Px;
	using Track = juce::Grid::TrackInfo;
	using Property = juce::GridItem::Property;
	using GridMargin = juce::GridItem::Margin;
	using FlexMargin = juce::FlexItem::Margin;
	using StartAndEndProperty = juce::GridItem::StartAndEndProperty;
	using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
	using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
	using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

	namespace Constants {
		const float roundedCornerFactor = 0.01f;
		const float seperatorSizePx = 3.f;
		const juce::Colour outlineColour = juce::Colours::black;
		const juce::Colour background0Colour = juce::Colours::darkgrey;
		const juce::Colour background1Colour = juce::Colours::grey;
		const juce::Colour text0Colour = background1Colour;
		const juce::Colour text1Colour = background0Colour;
		const juce::Colour enabledColour = juce::Colours::green;
		const juce::Colour disabledColour = juce::Colours::darkred;
		const juce::Colour knobMainColour = background0Colour;
		const juce::Colour envColour = juce::Colours::aqua;
		const juce::Colour lfoColour = juce::Colours::orange;

	}

	namespace Util {
		inline const juce::Grid createStretchGrid() {
			juce::Grid grid;
			grid.justifyItems = juce::Grid::JustifyItems::stretch;
			grid.alignItems = juce::Grid::AlignItems::stretch;
			grid.justifyContent = juce::Grid::JustifyContent::stretch;
			grid.alignContent = juce::Grid::AlignContent::stretch;
			return grid;
		}

		class GridComponent : public juce::Component {
		public:
			virtual void resized() override {
				grid.performLayout(getBounds());
			}
			juce::Grid grid = createStretchGrid();
		};

		inline const juce::FlexBox createHBox() {
			return juce::FlexBox{
		juce::FlexBox::Direction::row,
		juce::FlexBox::Wrap::noWrap,
		juce::FlexBox::AlignContent::stretch,
		juce::FlexBox::AlignItems::stretch,
		juce::FlexBox::JustifyContent::spaceAround };
		}

		class HBoxComponent : public juce::Component {
		public:
			virtual void resized() override {
				hBox.performLayout(getBounds());
			}
			juce::FlexBox hBox = createHBox();
		};

		inline const juce::FlexBox createVBox() {
			return juce::FlexBox{
		juce::FlexBox::Direction::column,
		juce::FlexBox::Wrap::noWrap,
		juce::FlexBox::AlignContent::stretch,
		juce::FlexBox::AlignItems::stretch,
		juce::FlexBox::JustifyContent::spaceAround };
		}

		class VBoxComponent : public juce::Component {
		public:
			virtual void resized() override {
				vBox.performLayout(getBounds());
			}
			juce::FlexBox vBox = createVBox();
		};

		inline const juce::Colour createRandomColour() {
			return juce::Colour::fromRGB((juce::uint8)juce::Random::getSystemRandom().nextInt(255),
				(juce::uint8)juce::Random::getSystemRandom().nextInt(255),
				(juce::uint8)juce::Random::getSystemRandom().nextInt(255)
			);
		}

		inline float getCornerSize(juce::Component* component) {
			if (auto* topLevelComp = component->getTopLevelComponent()) {
				return juce::jmin(topLevelComp->getWidth(), topLevelComp->getHeight()) * Constants::roundedCornerFactor;
			}
			else {
				jassertfalse;
			}
		}

		inline void centerSquare(juce::Component& component) {
			auto newBounds = component.getBounds();
			auto centre = newBounds.getCentre();
			auto min = juce::jmin(newBounds.getWidth(), newBounds.getHeight());
			component.setSize(min, min);
			component.setCentrePosition(centre);
		}

		inline void draw3DCircle(juce::Graphics& g, const juce::Rectangle<float>& bounds, const juce::Colour& colour) {
			auto gradientFill = juce::ColourGradient(
				colour.brighter(0.3f), bounds.getBottomRight(),
				colour, bounds.getTopLeft(),
				true);
			g.setGradientFill(gradientFill);
			g.fillEllipse(bounds);

			auto gradientOutline = juce::ColourGradient(
				colour.brighter(0.3f), bounds.getTopLeft(),
				colour, bounds.getBottomRight(),
				true);
			g.setGradientFill(gradientOutline);
			g.drawEllipse(bounds, Constants::seperatorSizePx);
		}

		inline void draw3DRoundedRectangle(juce::Graphics& g, const juce::Rectangle<float>& bounds, const juce::Colour& colour, float cornerSize) {
			auto gradientFill = juce::ColourGradient(
				colour.brighter(0.1f), bounds.getBottomRight(),
				colour, bounds.getTopLeft(),
				true);
			g.setGradientFill(gradientFill);
			g.fillRoundedRectangle(bounds, cornerSize);

			auto gradientOutline = juce::ColourGradient(
				colour.brighter(0.1f), bounds.getTopLeft(),
				colour, bounds.getBottomRight(),
				true);
			g.setGradientFill(gradientOutline);
			g.drawRoundedRectangle(bounds, cornerSize, Constants::seperatorSizePx);
		}
	};

	class CustomLookAndFeel : public juce::LookAndFeel_V4 {
	public:
		CustomLookAndFeel();
		virtual ~CustomLookAndFeel() override {}
		virtual juce::Font getComboBoxFont(juce::ComboBox& comboBox) override;

		virtual juce::Font getLabelFont(juce::Label& label) override;

		virtual juce::Font getTextButtonFont(juce::TextButton& textButton, int buttonHeight) override;

		virtual void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
			const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override;
	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLookAndFeel)
	};

	class BypassedButton : public  juce::ToggleButton {
		using ToggleButton::ToggleButton;
	public:
		virtual ~BypassedButton() override {};

		virtual void paint(juce::Graphics& g) override;

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BypassedButton)
	};

	class Dropdown : public juce::ComboBox {
	public:
		Dropdown(bool t_useSeperatorLine = true);
		virtual ~Dropdown()override {}
		virtual void paint(juce::Graphics& g) override;

	private:
		bool useSeperatorLine = true;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dropdown)
	};

	class ModSrcChooser : public juce::ComboBox {
	public:
		ModSrcChooser();
		virtual ~ModSrcChooser()override {}
		virtual void paint(juce::Graphics& g) override;
		virtual void resized() override;
	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModSrcChooser)
	};

	class Knob : public juce::Slider {
	public:
		Knob(bool rotary = true);
		virtual ~Knob()override {}
		virtual void resized() override;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)
	};

	class NamedKnob : public juce::Component {
	public:
		NamedKnob(const juce::String& labelText = juce::String("Name"), bool rotary = true);
		virtual ~NamedKnob()override {}
		virtual void setLabelText(const juce::String& t_text);

		virtual void resized() override;

		juce::FlexBox mainVBox = Util::createVBox();
		Knob knob;
		juce::Label nameLabel;

		static inline float labelFlex = 1.f;
		static inline float knobFlex = 2.5f;
	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamedKnob)
	};

	class MenuButton : public juce::Button {
	public:
		MenuButton(const juce::String& buttonName);
		virtual ~MenuButton()override {}
		virtual void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
	protected:

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MenuButton)
	};

	class HeaderMenu : public juce::Component {
	public:
		HeaderMenu() = delete;
		HeaderMenu(SynthAudioProcessor& t_audioProcessor);
		virtual ~HeaderMenu() override;

		virtual void paint(juce::Graphics& g) override;
		virtual void resized() override;

	protected:
		juce::FlexBox mainHBox = Util::createHBox();
		juce::FlexBox miscHBox = Util::createHBox();
		Util::GridComponent buttonGrid;

		MenuButton panicButton{ "DON'T PANIC!" };
		juce::Label label;
		MenuButton newButton{ "New" };
		MenuButton openButton{ "Open" };
		MenuButton saveButton{ "Save" };
		MenuButton saveAsButton{ "SaveAs" };

		std::unique_ptr<juce::File> presetDir;
		std::unique_ptr<juce::FileChooser> presetFileChooser;
		SynthAudioProcessor& audioProcessor;
	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderMenu)
	};

	class SpectrumAnalyzer : public juce::Component, private juce::Timer {
	public:

		SpectrumAnalyzer(SynthAudioProcessor& t_audioProcessor);
		virtual ~SpectrumAnalyzer() override {
			audioProcessor.observationCallback = nullptr;
		}
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

		SynthAudioProcessor& audioProcessor;
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

	// inspired by https://github.com/Thrifleganger/level-meter-demo
	class LevelMeter : public juce::Component, private juce::Timer {
	public:
		LevelMeter(float& valueRef);
		virtual ~LevelMeter() override {}
		virtual void paint(juce::Graphics& g) override;

		virtual void timerCallback() override;

		void resized();

	private:
		float& value;
		juce::ColourGradient gradient;
		const float granularity = 100.f;
		const float numGrillSegments = 21.f;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
	};

	class LevelDisplay : public juce::Component {
	public:
		LevelDisplay();
		virtual ~LevelDisplay() override {}
		void resized();

		void updateLevel(float t_left, float t_right);

	private:
		juce::FlexBox vBox = Util::createVBox();
		juce::FlexBox hBox = Util::createHBox();
		juce::Label meterLabel{ "","Peak in dB" };
		float levelLeft{ 0.f }, levelRight{ 0.f };
		juce::SmoothedValue<float> smoothedLeft{ 0 };
		juce::SmoothedValue<float> smoothedRight{ 0 };
		LevelMeter meterLeft{ levelLeft };
		LevelMeter meterRight{ levelRight };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelDisplay)
	};

	class SynthModule : public juce::Component {
	public:
		SynthModule() = delete;
		SynthModule(SynthAudioProcessor& audioProcessor, int id, int cols = 2);
		virtual ~SynthModule() override {};

		virtual void paint(juce::Graphics& g) override;
		virtual void resized() override;

		inline static const float gridMarginFactor = 3.f;
	protected:

		// Unfortunately this needs to be called in child destructors if they use attachments
		void deleteAllAttachments();

		juce::FlexBox vBox = Util::createVBox();
		juce::FlexBox headerHBox = Util::createHBox();
		Util::GridComponent gridComponent;

		BypassedButton bypassedButton;
		juce::Label nameLabel{ "","Test X" };
		Dropdown dropDown;

		juce::OwnedArray<ButtonAttachment> buttonAttachments;
		juce::OwnedArray<ComboBoxAttachment> comboBoxAttachments;
		juce::OwnedArray<SliderAttachment> sliderAttachments;

		const Fr knobRowFr{ 8 };
		const Fr modSrcRowFr{ 2 };

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthModule)
	};

	class ModuleHolder : public juce::Component {
	public:
		ModuleHolder(int cols = 1);
		virtual ~ModuleHolder() override {}
		void addModule(SynthModule* module);

		virtual void resized() override;
		virtual void paint(juce::Graphics& g) override;

	private:
		juce::OwnedArray<SynthModule> modules;
		juce::Grid grid = Util::createStretchGrid();

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleHolder)
	};
}