
#pragma once

#include <JuceHeader.h>
#include "Configuration.h"

namespace wavetable {


	// Wavetables as data:
	// 4 Byte (int) for number of channels per table
	// 4 Byte (int) for wtResolution
	// then just the 11 wavetables one after another starting with low frequency tables 
	// and ending with the highest, going from first to last channel
	// empty channels and values at the end, which we use at runtime, won't be safed to disk and will be appended, when reading from disk

	class Wavetable : public juce::ReferenceCountedObject {
	public:
		using Ptr = juce::ReferenceCountedObjectPtr<Wavetable>;
		Wavetable() = delete;
		Wavetable(const juce::String& t_name, const juce::MemoryBlock& waveData);
		virtual ~Wavetable() override {}

		void toMemoryBlock(juce::MemoryBlock& blockToFill) const;

		const juce::AudioBuffer<float>& getTable(int exponent) const;
		const juce::AudioBuffer<float>& getTable(float frequency) const;

		const juce::String& getName();

	protected:
		Wavetable(const juce::String& t_name) : name(t_name) {
		}
		juce::String name;

		static int getMaxHarmonics(double sampleRate);
		void createFromHarmonicWeights(double sampleRate, const juce::Array<double>& weights);

		// idempotent with same sampleRate
		static void updateSineValues(double sampleRate);
		inline static juce::AudioBuffer<double> sineValues;

		// number of samples for one cycle
		static const int defaultWtResolution = 2048 - 1;

		// tables[i] holds the wavetable for frequencies f with 2^(i+4) < f <= 2^(i+5)
		// (for lower frequencies we have more and higher harmonics)
		// each channel represents one cycle for a fixed wtPosition with (wtResolution + 1) samples
		// last sample == first sample to simplify calculations
		// each wavetable can have a different number of channels, but the last one will not be used; again to simplify calculations
		// I will go with 63+1 channels
		juce::AudioBuffer<float> tables[11];

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Wavetable)
	};


	class WavetableCache : private juce::DeletedAtShutdown {
	public:
		WavetableCache();
		~WavetableCache() override { clearSingletonInstance(); }

		JUCE_DECLARE_SINGLETON(WavetableCache, true);

		Wavetable::Ptr getWavetable(int nameIndex) {
			return getWavetable(wavetableNames[nameIndex]);
		}

		Wavetable::Ptr getWavetable(const juce::String& wtName);

		const juce::StringArray& getWavetableNames() {
			return wavetableNames;
		}

		void replaceIdWithName(juce::XmlElement& xml);

		void replaceNameWithId(juce::XmlElement& xml);

	private:

		void deleteUnused() {
			for (auto wtPtr : wavetables) {
				if (wtPtr->getReferenceCount() == 1) {
					wavetables.removeObject(wtPtr);
				}
			}
		};

		std::unique_ptr<juce::File> wtFolder;
		juce::StringArray wavetableNames;

		juce::ReferenceCountedArray<Wavetable, juce::CriticalSection> wavetables;

		enum class GeneratableWt {
			SawHarmonics,
			TriangleHarmonics,
			SquareHarmonics,
			PWM,
			Trapez,
			WhiteNoise,
			TotalNumber,
		};
		const juce::StringArray GENERATABLE_WT_NAMES{
			"SawHarmonics",
			"TriangleHarmonics",
			"SquareHarmonics",
			"PWM",
			"Trapez",
			"WhiteNoise",
		};

		JUCE_DECLARE_NON_COPYABLE(WavetableCache)
	};

	// TODO maybe make these static function

	class SawHarmonicsWavetable : public Wavetable {
	public:
		SawHarmonicsWavetable(const juce::String& t_name, double sampleRate) : Wavetable(t_name) {
			create(sampleRate);
		}
		virtual ~SawHarmonicsWavetable() override {};

		virtual void create(double sampleRate);

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SawHarmonicsWavetable)
	};

	class SquareHarmonicsWavetable : public Wavetable {
	public:
		SquareHarmonicsWavetable(const juce::String& t_name, double sampleRate) : Wavetable(t_name) {
			create(sampleRate);
		}
		virtual ~SquareHarmonicsWavetable() override {};

		virtual void create(double sampleRate);

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SquareHarmonicsWavetable)
	};

	class TriangleHarmonicsWavetable : public Wavetable {
	public:
		TriangleHarmonicsWavetable(const juce::String& t_name, double sampleRate) : Wavetable(t_name) {
			create(sampleRate);
		}
		virtual ~TriangleHarmonicsWavetable() override {};

		virtual void create(double sampleRate);

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriangleHarmonicsWavetable)
	};

	// for a period of P with wtPosition t: pwm(x) = saw(x) - saw(x+t*P/2) + (t-1)
	class PWMWavetable : public Wavetable {
	public:
		PWMWavetable(const juce::String& t_name, double sampleRate) : Wavetable(t_name) {
			create(sampleRate);
		}
		virtual ~PWMWavetable() override {};

		virtual void create(double sampleRate);

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PWMWavetable)
	};

	// for a period of P with wtPosition t: trapez(x) = (triangle(x) + triangle(x+t*P/2))/(2*(t-1))
	class TrapezWavetable : public Wavetable {
	public:
		TrapezWavetable(const juce::String& t_name, double sampleRate) : Wavetable(t_name) {
			create(sampleRate);
		}
		virtual ~TrapezWavetable() override {};

		virtual void create(double sampleRate);

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrapezWavetable)
	};

	class WhiteNoiseWavetable : public Wavetable {
	public:
		WhiteNoiseWavetable(const juce::String& t_name, double sampleRate) : Wavetable(t_name) {
			create(sampleRate);
		}
		virtual ~WhiteNoiseWavetable() override {};

		virtual void create(double sampleRate);

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WhiteNoiseWavetable)
	};
}