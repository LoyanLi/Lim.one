#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <vector>

class LimiterClassic
{
public:
    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        for (int count = 0; count < 22049; ++count)
        {
            bL[count] = 0.0;
            bR[count] = 0.0;
        }
        refclipL = 0.99;
        refclipR = 0.99;
        gcount = 0;
        size_t requiredSamples = (size_t)(targetLookaheadMs * 0.001 * currentSampleRate) + 1;
        lookaheadBufferL.assign(requiredSamples, 0.0f);
        lookaheadBufferR.assign(requiredSamples, 0.0f);
        writePos = 0;
        currentGR = 0.0f;
        currentGR_L = 0.0f;
        currentGR_R = 0.0f;
    }

    void setParameters(float newCeilingLin, float newCharacter, float newLookaheadMs)
    {
        ceiling = newCeilingLin;
        character = newCharacter;
        float t = std::max(0.0f, std::min(1.0f, character));
        linkAmount = 1.0f - 1.0f * t;
        targetLookaheadMs = std::max(0.0f, newLookaheadMs);
        size_t requiredSamples = (size_t)(targetLookaheadMs * 0.001 * currentSampleRate) + 1;
        if (lookaheadBufferL.size() != requiredSamples)
        {
            lookaheadBufferL.assign(requiredSamples, 0.0f);
            lookaheadBufferR.assign(requiredSamples, 0.0f);
            writePos = 0;
        }
    }

    void process(float& left, float& right)
    {
        double inputSampleL = left;
        double inputSampleR = right;
        double rawInputL = inputSampleL;
        double rawInputR = inputSampleR;

        const double t = std::max(0.0, std::min(1.0, (double) character));
        const double downStep = 0.002 + 0.018 * t;
        const double upStep = 0.00002 - 0.000018 * t;
        const double minRefclip = 0.5;

        double overshootL = std::abs(inputSampleL) - refclipL;
        double overshootR = std::abs(inputSampleR) - refclipR;
        if (overshootL < 0.0) overshootL = 0.0;
        if (overshootR < 0.0) overshootR = 0.0;

        if (gcount < 0 || gcount > 11020)
            gcount = 11020;
        int count = gcount;
        bL[count + 11020] = bL[count] = overshootL;
        bR[count + 11020] = bR[count] = overshootR;
        gcount--;

        if (inputSampleL > refclipL && refclipL > 0.9) refclipL -= downStep;
        if (inputSampleL < -refclipL && refclipL > 0.9) refclipL -= downStep;
        if (refclipL < 1.0) refclipL += upStep;
        if (refclipL < minRefclip) refclipL = minRefclip;

        if (inputSampleR > refclipR && refclipR > 0.9) refclipR -= downStep;
        if (inputSampleR < -refclipR && refclipR > 0.9) refclipR -= downStep;
        if (refclipR < 1.0) refclipR += upStep;
        if (refclipR < minRefclip) refclipR = minRefclip;

        const double linkedRefclip = std::min(refclipL, refclipR);
        refclipL = refclipL + (linkedRefclip - refclipL) * linkAmount;
        refclipR = refclipR + (linkedRefclip - refclipR) * linkAmount;

        if (inputSampleL > refclipL) inputSampleL = refclipL;
        if (inputSampleL < -refclipL) inputSampleL = -refclipL;
        if (inputSampleR > refclipR) inputSampleR = refclipR;
        if (inputSampleR < -refclipR) inputSampleR = -refclipR;

        float grL = 0.0f;
        if (std::abs(rawInputL) > 0.0001 && std::abs(inputSampleL) < std::abs(rawInputL))
            grL = 1.0f - (float)(std::abs(inputSampleL) / std::abs(rawInputL));

        float grR = 0.0f;
        if (std::abs(rawInputR) > 0.0001 && std::abs(inputSampleR) < std::abs(rawInputR))
            grR = 1.0f - (float)(std::abs(inputSampleR) / std::abs(rawInputR));

        currentGR_L = grL;
        currentGR_R = grR;
        currentGR = std::max(grL, grR);

        float outL = (float)(inputSampleL * ceiling);
        float outR = (float)(inputSampleR * ceiling);

        size_t lookaheadSize = lookaheadBufferL.size();
        if (lookaheadSize > 0)
        {
            lookaheadBufferL[writePos] = outL;
            lookaheadBufferR[writePos] = outR;

            size_t readIndex = (writePos + 1) % lookaheadSize;
            left = lookaheadBufferL[readIndex];
            right = lookaheadBufferR[readIndex];
            writePos = (writePos + 1) % lookaheadSize;
        }
        else
        {
            left = outL;
            right = outR;
        }
    }

    float getGainReduction() const { return currentGR; }
    float getGainReductionLeft() const { return currentGR_L; }
    float getGainReductionRight() const { return currentGR_R; }

private:
    double currentSampleRate = 44100.0;
    float ceiling = 1.0f;
    float character = 0.0f;
    float currentGR = 0.0f;
    float currentGR_L = 0.0f;
    float currentGR_R = 0.0f;

    double bL[22050];
    double bR[22050];
    double refclipL = 0.99;
    double refclipR = 0.99;
    int gcount = 0;
    float linkAmount = 0.5f;

    std::vector<float> lookaheadBufferL;
    std::vector<float> lookaheadBufferR;
    size_t writePos = 0;
    float targetLookaheadMs = 2.0f;
};
