#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include "LimiterClassic.h"
#include "LimiterModern.h"

class Limiter
{
public:
    Limiter() = default;

    void setAdvancedParameters(
        float holdMs,
        float holdReleaseMs,
        float attackTauDiv,
        float releaseSmoothBaseMs,
        float releaseSmoothRangeMs,
        float adaptFastStrength,
        float adaptSlowStrength,
        float scHpfHz,
        float ratioBase,
        float ratioSlope,
        float transientMix,
        float softSafetyStrength,
        float modernReleaseFastMs,
        float modernReleaseSlowMs,
        float modernLinkTransients,
        float modernLinkRelease)
    {
        modern.setAdvancedParameters(
            holdMs,
            holdReleaseMs,
            attackTauDiv,
            releaseSmoothBaseMs,
            releaseSmoothRangeMs,
            adaptFastStrength,
            adaptSlowStrength,
            scHpfHz,
            ratioBase,
            ratioSlope,
            transientMix,
            softSafetyStrength,
            modernReleaseFastMs,
            modernReleaseSlowMs,
            modernLinkTransients,
            modernLinkRelease);
    }

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        classic.prepare(sampleRate);
        modern.prepare(sampleRate);
    }

    void setParameters(float newLimiterDriveDb, float newClipperDriveDb, float newCeilingDb, float newCharacter, int newMode, bool isTruePeak, float newLookaheadMs)
    {
        limiterDrive = std::pow(10.0f, newLimiterDriveDb / 20.0f);
        clipperDrive = std::pow(10.0f, newClipperDriveDb / 20.0f);
        mode = newMode;

        float effectiveCeilingDb = newCeilingDb;
        if (isTruePeak)
            effectiveCeilingDb -= 0.2f;

        float ceilingLin = std::pow(10.0f, effectiveCeilingDb / 20.0f);

        if (mode == 0)
            classic.setParameters(ceilingLin, newCharacter, newLookaheadMs);
        else
            modern.setParameters(ceilingLin, newCharacter, newLookaheadMs);
    }

    void process(float& left, float& right)
    {
        if (limiterDrive != 1.0f)
        {
            left *= limiterDrive;
            right *= limiterDrive;
        }

        if (mode == 0)
            classic.process(left, right);
        else
            modern.process(left, right);
    }

    float getGainReduction() const
    {
        return mode == 0 ? classic.getGainReduction() : modern.getGainReduction();
    }

    float getGainReductionLeft() const
    {
        return mode == 0 ? classic.getGainReductionLeft() : modern.getGainReductionLeft();
    }

    float getGainReductionRight() const
    {
        return mode == 0 ? classic.getGainReductionRight() : modern.getGainReductionRight();
    }

private:
    double currentSampleRate = 44100.0;
    float limiterDrive = 1.0f;
    float clipperDrive = 1.0f;
    int mode = 0;
    LimiterClassic classic;
    LimiterModern modern;
};
