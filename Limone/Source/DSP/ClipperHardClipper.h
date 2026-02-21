#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>

class ClipperHardClipper
{
public:
    ClipperHardClipper() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        lastInL = 0.0f;
        lastInR = 0.0f;
    }

    void setParameters(float newDrive, float newKnee)
    {
        drive = newDrive;
        knee = juce::jlimit(0.0f, 1.0f, newKnee);
        inputGain = std::pow(10.0, drive / 20.0);
    }

    void process(float& left, float& right)
    {
        if (std::abs(drive) < 1.0e-7f && knee < 1.0e-7f)
        {
            lastInL = left;
            lastInR = right;
            currentGR_L = 0.0f;
            currentGR_R = 0.0f;
            currentGR = 0.0f;
            return;
        }

        if (inputGain != 1.0)
        {
            left *= (float)inputGain;
            right *= (float)inputGain;
        }

        const float inL = left;
        const float outL = juce::jlimit(-1.0f, 1.0f, inL);
        const float inR = right;
        const float outR = juce::jlimit(-1.0f, 1.0f, inR);

        float grL = 0.0f;
        const float absInL = std::abs(inL);
        const float absOutL = std::abs(outL);
        if (absInL > 0.0f && absOutL < absInL) grL = 1.0f - (absOutL / absInL);

        float grR = 0.0f;
        const float absInR = std::abs(inR);
        const float absOutR = std::abs(outR);
        if (absInR > 0.0f && absOutR < absInR) grR = 1.0f - (absOutR / absInR);

        currentGR_L = grL;
        currentGR_R = grR;
        currentGR = std::max(grL, grR);

        left = outL;
        right = outR;
    }

    float getGainReduction() const { return currentGR; }
    float getGainReductionLeft() const { return currentGR_L; }
    float getGainReductionRight() const { return currentGR_R; }

private:
    double currentSampleRate = 44100.0;
    float drive = 0.0f;
    float knee = 0.0f;
    float currentGR = 0.0f;
    float currentGR_L = 0.0f;
    float currentGR_R = 0.0f;
    double inputGain = 1.0;
    float lastInL = 0.0f;
    float lastInR = 0.0f;
};
