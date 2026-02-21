#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include "ClipperSoftClipper.h"
#include "ClipperHardClipper.h"

class Clipper
{
public:
    enum class Algorithm
    {
        Soft = 0,
        Hard = 1
    };

    Clipper() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        soft.prepare(sampleRate);
        hard.prepare(sampleRate);
    }

    void setAlgorithm(int newAlgorithm)
    {
        if (newAlgorithm < 0) newAlgorithm = 0;
        if (newAlgorithm > 1) newAlgorithm = 1;
        algorithm = static_cast<Algorithm>(newAlgorithm);
    }

    void setParameters(float newDrive, float newKnee)
    {
        soft.setParameters(newDrive, newKnee);
        hard.setParameters(newDrive, newKnee);
    }

    void process(float& left, float& right)
    {
        if (algorithm == Algorithm::Hard)
            hard.process(left, right);
        else
            soft.process(left, right);
    }

    float getGainReduction() const
    {
        return algorithm == Algorithm::Hard ? hard.getGainReduction() : soft.getGainReduction();
    }

    float getGainReductionLeft() const
    {
        return algorithm == Algorithm::Hard ? hard.getGainReductionLeft() : soft.getGainReductionLeft();
    }

    float getGainReductionRight() const
    {
        return algorithm == Algorithm::Hard ? hard.getGainReductionRight() : soft.getGainReductionRight();
    }

private:
    double currentSampleRate = 44100.0;
    Algorithm algorithm = Algorithm::Soft;
    ClipperSoftClipper soft;
    ClipperHardClipper hard;
};
