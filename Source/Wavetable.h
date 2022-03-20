
#pragma once

#include <JuceHeader.h>

namespace wavetable {

	class Wavetable {
	public:
		Wavetable(const juce::String& t_name) : name(t_name) {};
		virtual ~Wavetable(){}

		//needs to be called before getSample() is used and should be idempotent with the same sampleRate
		virtual void create(double sampleRate) = 0;

		float getSample(int exponent, float phase, float wtPos) const;

		const juce::AudioBuffer<float>& getTable(int exponent) const;

		const juce::String& getName();

	protected:
		juce::String name;
		double oldSampleRate = 0.0;
		static int getMaxHarmonics(double sampleRate);
		void createFromHarmonicWeights(double sampleRate, const juce::Array<double>& weights);
		
		// idempotent with same sampleRate
		static void updateSineValues(double sampleRate);
		inline static juce::AudioBuffer<double> sineValues;

		// number of samples for one cycle
		static const int wtResolution = 2048-1;

		// tables[i] holds the wavetable for frequencies f with 2^(i+4) < f <= 2^(i+5)
		// (for lower frequencies we have more and higher harmonics)
		// each channel represents one cycle for a fixed wtPosition with (wtResolution + 1) samples
		// last sample == first sample to simplify getSample()
		// each wavetable can have a different number of channels, but the last one will not be used; again to simplify get Sample()
		// I will go with 63+1 channels
		juce::AudioBuffer<float> tables[11];

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Wavetable)
	};


	class WavetableManager {
	public:
		enum {
			SAW_HARMONICS,
			SQUARE_HARMONICS,
			TRIANGLE_HARMONICS,
			PWM,
			TRAPEZ,

			NUMBER_OF_WT
		};
		WavetableManager() = delete;
		// idempotent
		static void initAll();
		static const Wavetable* getWavetable(int index, double sampleRate);
		static const juce::StringArray& getWavetableNames();
		static void cleanUp();
	private:
		inline static juce::OwnedArray<Wavetable> wavetables;
		inline static juce::StringArray wavetableNames;
	};

	class SawHarmonicsWavetable : public Wavetable {
	public:
		SawHarmonicsWavetable() : Wavetable("SawHarmonics") {};

		virtual void create(double sampleRate) override;

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SawHarmonicsWavetable)
	};

	class SquareHarmonicsWavetable : public Wavetable {
	public:
		SquareHarmonicsWavetable() : Wavetable("SquareHarmonics") {};

		virtual void create(double sampleRate) override;

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SquareHarmonicsWavetable)
	};

	class TriangleHarmonicsWavetable : public Wavetable {
	public:
		TriangleHarmonicsWavetable() : Wavetable("TriangleHarmonics") {};

		virtual void create(double sampleRate) override;

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriangleHarmonicsWavetable)
	};

	// for a period of P with wtPosition t: pwm(x) = saw(x) - saw(x+t*P/2) + (t-1)
	class PWMWavetable : public Wavetable {
	public:
		PWMWavetable() : Wavetable("PWM") {};

		virtual void create(double sampleRate) override;

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PWMWavetable)
	};

	// for a period of P with wtPosition t: trapez(x) = (triangle(x) + triangle(x+t*P/2))/(2*(t-1))
	class TrapezWavetable : public Wavetable {
	public:
		TrapezWavetable() : Wavetable("Trapez(Tri->Square)") {};

		virtual void create(double sampleRate) override;

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrapezWavetable)
	};
}