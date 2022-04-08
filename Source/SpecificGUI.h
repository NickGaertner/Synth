
#pragma once
#include <JuceHeader.h>
#include "GeneralGUI.h"
#include "PluginProcessor.h"

namespace customGui {

	class MainComponent : public juce::Component {
	public:
		MainComponent() = delete;
		MainComponent(SynthAudioProcessor& audioProcessor);
		virtual ~MainComponent() override;

		void paint(juce::Graphics&) override;
		void resized() override;
	private:
		void initModules(SynthAudioProcessor& audioProcessor);

		juce::Grid mainGrid = Util::createStretchGrid();

		juce::Label header;
		ModuleHolder oscModuleHolder;
		ModuleHolder filterModuleHolder;
		ModuleHolder fxModuleHolder;
		ModuleHolder envModuleHolder{ configuration::ENV_NUMBER };
		ModuleHolder lfoModuleHolder{ configuration::LFO_NUMBER };
		ModuleHolder panModuleHolder;
		ModuleHolder masterModuleHolder;
		ModuleHolder spectrumModuleHolder;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
	};

	class OscModule : public SynthModule {
	public:
		OscModule() = delete;
		OscModule(SynthAudioProcessor& audioProcessor, int id);
		virtual ~OscModule() override {}
		
		virtual void resized() override;

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

		juce::Label envLabel{ "","Env" };
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

	// TODO master here

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

	class MasterModule : public SynthModule {
	public:
		MasterModule() = delete;
		MasterModule(SynthAudioProcessor& audioProcessor, int id = 0);
		virtual ~MasterModule() override {}

	protected:
		LevelDisplay levelDisplay;

		NamedKnob masterKnob{ "Master" };

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterModule)
	};

}