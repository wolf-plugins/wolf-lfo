/*
 * Wolf LFO
 * Copyright (C) 2018 Patrick Desaulniers
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "DistrhoPlugin.hpp"
#include "extra/Mutex.hpp"
#include "extra/ScopedPointer.hpp"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cmath>

#include "WolfLFOParameters.hpp"
#include "Graph.hpp"
#include "Oversampler.hpp"
#include "ParamSmooth.hpp"
#include "Mathf.hpp"

#include "DspFilters/Dsp.h"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

enum LFORate
{
	SixteenBar = 0,
	TwelveBar,
	NineBar,
	EightBar,
	SixBar,
	FourBar,
	ThreeBar,
	TwoBar,
	OneAndHalfBar,
	OneBar,
	HalfBar,
	OneThirdBar,
	OneFourthBar,
	OneSixthBar,
	OneEighthBar,
	OneNinthBar,
	OneTwelfthBar,
	OneSixteenthBar,
	OneThirtyTwoBar,
	OneSixtyFourthBar,
	OneOneHundredTwentyEighthBar,
	OneTwoHundredFiftySixthBar,
	OneFiveHundredTwelveBar,
	LFORatesCount
};

static float getLFORateInBars(LFORate rate)
{
	DISTRHO_SAFE_ASSERT_RETURN(rate != LFORatesCount, 1);

	switch (rate)
	{
	case SixteenBar:
		return 16;
	case TwelveBar:
		return 12;
	case NineBar:
		return 9;
	case EightBar:
		return 8;
	case SixBar:
		return 6;
	case FourBar:
		return 4;
	case ThreeBar:
		return 3;
	case TwoBar:
		return 2;
	case OneAndHalfBar:
		return 1.5f;
	case OneBar:
		return 1;
	case HalfBar:
		return 1.0f / 2.0f;
	case OneThirdBar:
		return 1.0f / 3.0f;
	case OneFourthBar:
		return 1.0f / 4.0f;
	case OneSixthBar:
		return 1.0f / 6.0f;
	case OneEighthBar:
		return 1.0f / 8.0f;
	case OneNinthBar:
		return 1.0f / 9.0f;
	case OneTwelfthBar:
		return 1.0f / 12.0f;
	case OneSixteenthBar:
		return 1.0f / 16.0f;
	case OneThirtyTwoBar:
		return 1.0f / 32.0f;
	case OneSixtyFourthBar:
		return 1.0f / 64.0f;
	case OneOneHundredTwentyEighthBar:
		return 1.0f / 128.0f;
	case OneTwoHundredFiftySixthBar:
		return 1.0f / 256.0f;
	case OneFiveHundredTwelveBar:
		return 1.0f / 512.0f;
	case LFORatesCount:
		return 1;
	}

	return 1;
}

static float getFreeLFORate(float rate)
{
	return wolf::logScale(rate + 1, 1, LFORatesCount) - 1;
}

class WolfLFO : public Plugin
{
  public:
	WolfLFO() : Plugin(paramCount, 0, 1),
				graphOutput(),
				mustCopyLineEditor(false),
				playHeadPos(0.0f)
	{
		graphOutput.calculateCoeff(20.f, getSampleRate());
	}

  protected:
	const char *getLabel() const noexcept override
	{
		return "Wolf LFO";
	}

	const char *getDescription() const noexcept override
	{
		return "Multi-purpose LFO plugin with a spline-based graph.";
	}

	const char *getMaker() const noexcept override
	{
		return "Patrick Desaulniers";
	}

	const char *getHomePage() const noexcept override
	{
		return "https://github.com/pdesaulniers/wolf-lfo";
	}

	const char *getLicense() const noexcept override
	{
		return "GPL v3+";
	}

	uint32_t getVersion() const noexcept override
	{
		return d_version(0, 0, 1);
	}

	int64_t getUniqueId() const noexcept override
	{
		return d_cconst('W', 'L', 'F', 'O');
	}

	void initParameter(uint32_t index, Parameter &parameter) override
	{
		if (index >= paramCount)
			return;

		switch (index)
		{
		case paramPreGain:
			parameter.name = "Pre Gain";
			parameter.symbol = "pregain";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 2.0f;
			parameter.ranges.def = 1.0f;
			parameter.hints = kParameterIsAutomable | kParameterIsLogarithmic;
			break;
		case paramWet:
			parameter.name = "Wet";
			parameter.symbol = "wet";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 1.0f;
			parameter.ranges.def = 1.0f;
			parameter.hints = kParameterIsAutomable;
			break;
		case paramPostGain:
			parameter.name = "Post Gain";
			parameter.symbol = "postgain";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 1.0f;
			parameter.ranges.def = 1.0f;
			parameter.hints = kParameterIsAutomable | kParameterIsLogarithmic;
			break;
		case paramRemoveDC:
			parameter.name = "Remove DC Offset";
			parameter.symbol = "removedc";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 1.0f;
			parameter.ranges.def = 1.0f;
			parameter.hints = kParameterIsAutomable | kParameterIsBoolean | kParameterIsInteger;
			break;
		case paramOversample:
			//None, 2x, 4x, 8x, 16x
			parameter.name = "Oversample";
			parameter.symbol = "oversample";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 4.0f;
			parameter.ranges.def = 0.0f;
			parameter.hints = kParameterIsAutomable | kParameterIsInteger;
			break;
		case paramBipolarMode:
			parameter.name = "Bipolar Mode";
			parameter.symbol = "bipolarmode";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 1.0f;
			parameter.ranges.def = 0.0f;
			parameter.hints = kParameterIsAutomable | kParameterIsBoolean | kParameterIsInteger;
			break;
		case paramHorizontalWarpType:
			//None, Bend +, Bend -, Bend +/-, Skew +, Skew -, Skew +/-
			parameter.name = "H Warp Type";
			parameter.symbol = "warptype";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 6.0f;
			parameter.ranges.def = 0.0f;
			parameter.hints = kParameterIsAutomable | kParameterIsInteger;
			break;
		case paramHorizontalWarpAmount:
			parameter.name = "H Warp Amount";
			parameter.symbol = "warpamount";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 1.0f;
			parameter.ranges.def = 0.0f;
			parameter.hints = kParameterIsAutomable;
			break;
		case paramVerticalWarpType:
			//None, Bend +, Bend -, Bend +/-, Skew +, Skew -, Skew +/-
			parameter.name = "V Warp Type";
			parameter.symbol = "vwarptype";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 6.0f;
			parameter.ranges.def = 0.0f;
			parameter.hints = kParameterIsAutomable | kParameterIsInteger;
			break;
		case paramVerticalWarpAmount:
			parameter.name = "V Warp Amount";
			parameter.symbol = "vwarpamount";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 1.0f;
			parameter.ranges.def = 0.0f;
			parameter.hints = kParameterIsAutomable;
			break;
		case paramLFORate:
			parameter.name = "LFO Rate";
			parameter.symbol = "lforate";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = LFORatesCount - 1;
			parameter.ranges.def = OneFourthBar;
			parameter.hints = kParameterIsAutomable;
			break;
		case paramBPMSync:
			parameter.name = "BPM Sync";
			parameter.symbol = "bpmsync";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 1.0f;
			parameter.ranges.def = 1.0f;
			parameter.hints = kParameterIsAutomable | kParameterIsBoolean;
			break;
		case paramPhase:
			parameter.name = "Phase";
			parameter.symbol = "phase";
			parameter.ranges.min = 0.0f;
			parameter.ranges.max = 1.0f;
			parameter.ranges.def = 0.0f;
			parameter.hints = kParameterIsAutomable;
			break;
		case paramSmoothing:
			parameter.name = "Smoothing";
			parameter.symbol = "smoothing";
			parameter.ranges.min = 0.00001f;
			parameter.ranges.max = 42.0f;
			parameter.ranges.def = 20.0f;
			parameter.hints = kParameterIsAutomable | kParameterIsLogarithmic;
			break;
		case paramOut:
			parameter.name = "Out";
			parameter.symbol = "out";
			parameter.hints = kParameterIsOutput;
			parameter.ranges.def = 0.0f;
			break;
		}

		parameters[index] = ParamSmooth(parameter.ranges.def);
		parameters[index].calculateCoeff(20.f, getSampleRate());
	}

	float getParameterValue(uint32_t index) const override
	{
		return parameters[index].getRawValue();
	}

	void setParameterValue(uint32_t index, float value) override
	{
		parameters[index].setValue(value);
	}

	void initState(uint32_t index, String &stateKey, String &defaultStateValue) override
	{
		switch (index)
		{
		case 0:
			stateKey = "graph";
			break;
		}

		//generated with fprintf(stderr, "%A,%A,%A,%d;%A,%A,%A,%d;\n", 0.0f, 0.0f, 0.0f, wolf::CurveType::Exponential, 1.0f, 1.0f, 0.0f, wolf::CurveType::Exponential);
		defaultStateValue = String("0x0p+0,0x0p+0,0x0p+0,0;0x1p+0,0x1p+0,0x0p+0,0;");
	}

	void setState(const char *key, const char *value) override
	{
		const MutexLocker cml(mutex);

		if (std::strcmp(key, "graph") == 0)
		{
			tempLineEditor.rebuildFromString(value);
			mustCopyLineEditor = true;
		}
	}

	float getGraphValue(float input)
	{
		return lineEditor.getValueAt(input);
	}

	void synchronizePlayHead()
	{
		const bool bpmSync = std::round(parameters[paramBPMSync].getRawValue());

		if (!bpmSync)
			return;

		const TimePosition::BarBeatTick bbt = getTimePosition().bbt;

		if (!bbt.valid)
			return;

		const int32_t bar = bbt.bar - 1;
		const int32_t beat = bbt.beat - 1;
		const int32_t beatTick = bbt.tick;
		const double ticksPerBeat = bbt.ticksPerBeat;
		const float beatsPerBar = bbt.beatsPerBar;

		const int lfoRateIndex = std::round(parameters[paramLFORate].getRawValue());
		const float lfoRate = getLFORateInBars((LFORate)lfoRateIndex);

		const double beatsPerLFORotation = lfoRate * beatsPerBar;
		const double percentOfBeatDone = beatTick / ticksPerBeat;
		const float totalBeats = bar * beatsPerBar + beat + percentOfBeatDone;

		playHeadPos = std::fmod(totalBeats, beatsPerLFORotation) / beatsPerLFORotation;

		const float phase = parameters[paramPhase].getRawValue();
		playHeadPos += phase;

		if (playHeadPos > 1.0f)
		{
			playHeadPos -= 1.0f;
		}
	}

	void updatePlayHeadPos()
	{
		const TimePosition &timePos = getTimePosition();

		const bool bpmSync = std::round(parameters[paramBPMSync].getRawValue());

		if (bpmSync)
		{
			if (!timePos.playing)
				return;

			const int lfoRateIndex = std::round(parameters[paramLFORate].getRawValue());
			const float lfoRate = getLFORateInBars((LFORate)lfoRateIndex);

			playHeadPos += 1.0f / getSampleRate() / 60.f * (timePos.bbt.beatsPerMinute / timePos.bbt.beatsPerBar) / lfoRate;
		}
		else
		{
			const float lfoRate = getFreeLFORate(parameters[paramLFORate].getSmoothedValue());

			playHeadPos += 1.0f / getSampleRate() * lfoRate;
		}

		if (playHeadPos > 1.0f)
		{
			playHeadPos -= 1.0f;
		}
	}

	void run(const float **inputs, float **outputs, uint32_t frames) override
	{
		if (mutex.tryLock())
		{
			if (mustCopyLineEditor)
			{
				lineEditor = tempLineEditor;

				for (int i = 0; i < lineEditor.getVertexCount(); ++i)
				{
					lineEditor.getVertexAtIndex(i)->setGraphPtr(&lineEditor);
				}

				mustCopyLineEditor = false;
			}
		}

		wolf::WarpType horizontalWarpType = (wolf::WarpType)std::round(parameters[paramHorizontalWarpType].getRawValue());
		lineEditor.setHorizontalWarpType(horizontalWarpType);

		wolf::WarpType verticalWarpType = (wolf::WarpType)std::round(parameters[paramVerticalWarpType].getRawValue());
		lineEditor.setVerticalWarpType(verticalWarpType);

		synchronizePlayHead();

		const float smoothingFrequency = 44.1f - parameters[paramSmoothing].getRawValue();
		graphOutput.calculateCoeff(smoothingFrequency, getSampleRate());

		for (uint32_t i = 0; i < frames; ++i)
		{
			lineEditor.setHorizontalWarpAmount(parameters[paramHorizontalWarpAmount].getSmoothedValue());
			lineEditor.setVerticalWarpAmount(parameters[paramVerticalWarpAmount].getSmoothedValue());

			const float preGain = parameters[paramPreGain].getSmoothedValue();

			float inputL = preGain * inputs[0][i];
			float inputR = preGain * inputs[1][i];

			if (inputL < 0.0f && inputL > -0.00001f)
			{
				inputL = 0.0f;
			}
			if (inputR < 0.0f && inputR > -0.00001f)
			{
				inputR = 0.0f;
			}

			const float rawGraphOutput = getGraphValue(playHeadPos);

			const double euler = std::exp(1.0);
			const float scaledOutput = (std::exp(rawGraphOutput) - 1) / (euler - 1);

			graphOutput.setValue(scaledOutput);
			const float smoothedOutput = graphOutput.getSmoothedValue();

			const float outL = inputL * smoothedOutput;
			const float outR = inputR * smoothedOutput;

			const float wet = parameters[paramWet].getSmoothedValue();
			const float dry = 1.0f - wet;
			const float postGain = parameters[paramPostGain].getSmoothedValue();

			outputs[0][i] = (dry * inputL + wet * outL) * postGain;
			outputs[1][i] = (dry * inputR + wet * outR) * postGain;

			updatePlayHeadPos();
		}

		setParameterValue(paramOut, playHeadPos);

		mutex.unlock();
	}

  private:
	ParamSmooth parameters[paramCount];
	ParamSmooth graphOutput;

	wolf::Graph lineEditor;
	wolf::Graph tempLineEditor;
	bool mustCopyLineEditor;

	float playHeadPos;

	Mutex mutex;

	DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WolfLFO)
};

Plugin *createPlugin()
{
	return new WolfLFO();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
