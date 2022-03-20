
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Configuration.h"
#include "Wavetable.h"

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
		NamedKnob(const juce::String& labelName = juce::String("Name"), bool displayValue = false) :
			nameLabel(juce::String(), labelName),
			slider(juce::Slider::SliderStyle::RotaryVerticalDrag,
				displayValue ? juce::Slider::TextEntryBoxPosition::TextBoxBelow : juce::Slider::TextEntryBoxPosition::NoTextBox)
		{
			nameLabel.setJustificationType(juce::Justification::centred);
			addAndMakeVisible(nameLabel);
			mainVBox.items.add(juce::FlexItem(nameLabel).withFlex(0.2f));

			addAndMakeVisible(slider);
			mainVBox.items.add(juce::FlexItem(slider).withFlex(1.f));
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

	class BasicWaveformChooser : public juce::Component {

	public:
		BasicWaveformChooser() {
			for (int i = 0; i < std::size(buttons); i++) {
				addAndMakeVisible(*buttons[i]);
				buttons[i]->setRadioGroupId(1);
				buttons[i]->onClick = [=]() {respondToClick(i); };
			}

			mainGrid.setGap(Px(0));
			mainGrid.templateRows = { Track(Fr(1)), Track(Fr(1)) };
			mainGrid.templateColumns = { Track(Fr(1)), Track(Fr(1)) };

			mainGrid.items.addArray({
				juce::GridItem(sineButton).withArea(Property(1), Property(1)),
				juce::GridItem(squareButton).withArea(Property(1), Property(2)),
				juce::GridItem(triangleButton).withArea(Property(2), Property(1)),
				juce::GridItem(sawButton).withArea(Property(2), Property(2)),
				});
		}

		virtual void resized() override {
			mainGrid.performLayout(getLocalBounds());
		}

		void respondToClick(int buttonNumber) {
			if (buttons[buttonNumber]->getToggleState()) {
				parameterAttachment->setValueAsCompleteGesture((float)buttonNumber);
			}
		}

		void parameterChanged(float value) {
			auto index = juce::roundToInt(value);
			jassert(0 <= index && index < std::size(buttons));
			buttons[index]->triggerClick();
		}

		void attachToParameter(juce::RangedAudioParameter& parameter) {
			parameterAttachment = std::make_unique<juce::ParameterAttachment>(parameter, [&](float x) {parameterChanged(x); });
			parameterAttachment->sendInitialUpdate();
		}

	private:
		juce::Grid mainGrid = Util::createStretchGrid();
		// TODO change to ImageButtons
		juce::ToggleButton sineButton{ "Sine" };
		juce::ToggleButton squareButton{ "Square" };
		juce::ToggleButton triangleButton{ "Triangle" };
		juce::ToggleButton sawButton{ "Saw" };
		juce::ToggleButton* buttons[4]{ &sineButton, &squareButton, &triangleButton, &sawButton };
		std::unique_ptr<juce::ParameterAttachment> parameterAttachment;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BasicWaveformChooser)
	};

	class SynthModule : public juce::Component {
	public:
		SynthModule() = delete;
		SynthModule(SynthAudioProcessor& audioProcessor, int id);
		virtual ~SynthModule() override {}

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
		using Component::Component;
	public:

		void addModule(SynthModule* module) {
			modules.add(module);
			vBox.items.add(juce::FlexItem(*module).withFlex(1.f));
			addAndMakeVisible(*module);
		}

		virtual void resized() override {
			vBox.performLayout(getLocalBounds());
		}
	private:
		juce::OwnedArray<SynthModule> modules;
		juce::FlexBox vBox = Util::createVBox();

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

	class EnvModule : public SynthModule {
	public:
		EnvModule() = delete;
		EnvModule(SynthAudioProcessor& audioProcessor, int id);
		virtual ~EnvModule() override {}

	protected:
		NamedKnob attackKnob{ "Attack"};
		NamedKnob decayKnob{ "Decay"};
		NamedKnob sustainKnob{ "Sustain"};
		NamedKnob releaseKnob{ "Release"};

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvModule)
	};

	class LFOModule : public SynthModule {
	public:
		LFOModule() = delete;
		LFOModule(SynthAudioProcessor& audioProcessor, int id);
		virtual ~LFOModule() override {}

	protected:
		BasicWaveformChooser wf0Chooser;
		BasicWaveformChooser wf1Chooser;
		NamedKnob wtPosKnob{ "WtPos" };
		NamedKnob rateKnob{ "Rate" };


	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOModule)
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
		ModuleHolder envModuleHolder;
		ModuleHolder lfoModuleHolder;
		juce::Label panModule;
		juce::Label masterModule;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthComponent)
	};


	class CustomLookAndFeel : public juce::LookAndFeel_V4 {
	public:
		CustomLookAndFeel() {};
	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLookAndFeel)
	};
}