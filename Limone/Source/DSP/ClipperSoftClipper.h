#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>

class ClipperSoftClipper
{
public:
    ClipperSoftClipper() = default;

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
        updateADAAState();
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
        const float outL = processADAA(inL, lastInL);

        float grL = 0.0f;
        const float absInL = std::abs(inL);
        const float absOutLMeter = applyClipCurve(absInL);
        if (absInL > 0.0f && absOutLMeter < absInL) grL = 1.0f - (absOutLMeter / absInL);
        left = outL;

        const float inR = right;
        const float outR = processADAA(inR, lastInR);

        float grR = 0.0f;
        const float absInR = std::abs(inR);
        const float absOutRMeter = applyClipCurve(absInR);
        if (absInR > 0.0f && absOutRMeter < absInR) grR = 1.0f - (absOutRMeter / absInR);
        right = outR;

        currentGR_L = grL;
        currentGR_R = grR;
        currentGR = std::max(grL, grR);
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

    struct ADAAState
    {
        float k = 0.0f;
        float start = 1.0f;
        float n2 = 0.0f;
        float n3 = 0.0f;
        float n4 = 0.0f;
        float n5 = 0.0f;
        float integralKneeRegion = 0.0f;
        bool enabled = false;
    } adaa;

    static float clampf(float v, float lo, float hi)
    {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }

    void updateADAAState()
    {
        adaa.enabled = (knee > 0.0f);
        adaa.k = knee;
        adaa.start = 1.0f - knee;

        if (!adaa.enabled)
        {
            adaa.n2 = adaa.n3 = adaa.n4 = adaa.n5 = 0.0f;
            adaa.integralKneeRegion = 0.0f;
            return;
        }

        const float k = adaa.k;
        const float start = adaa.start;

        const float yAt1 = 1.0f - 0.5f * k;
        const float g = (yAt1 > 0.0f) ? (1.0f / yAt1) : 1.0f;
        const float a = g - 1.0f;

        const float invK = 1.0f / k;
        const float invK2 = invK * invK;
        const float invK3 = invK2 * invK;

        const float b2 = a * 3.0f * invK2;
        const float b3 = -2.0f * a * invK3;

        const float c0 = start;
        const float c1 = 1.0f;
        const float c2 = -0.5f * invK;

        adaa.n2 = c2 + c0 * b2;
        adaa.n3 = c1 * b2 + c0 * b3;
        adaa.n4 = c2 * b2 + c1 * b3;
        adaa.n5 = c2 * b3;

        const float dx = k;
        const float integral =
            (c0 * dx)
            + (c1 * dx * dx) * 0.5f
            + (adaa.n2 * dx * dx * dx) / 3.0f
            + (adaa.n3 * dx * dx * dx * dx) / 4.0f
            + (adaa.n4 * dx * dx * dx * dx * dx) / 5.0f
            + (adaa.n5 * dx * dx * dx * dx * dx * dx) / 6.0f;

        adaa.integralKneeRegion = integral;
    }

    float applyClipCurve(float x) const
    {
        if (x <= 0.0f) return 0.0f;

        if (knee <= 0.0f)
            return (x > 1.0f) ? 1.0f : x;

        const float k = knee;
        const float start = 1.0f - k;

        if (x <= start) return x;
        if (x >= 1.0f) return 1.0f;

        const float dx = x - start;
        const float y0 = x - (dx * dx) / (2.0f * k);

        const float t = dx / k;
        const float s = t * t * (3.0f - 2.0f * t);

        const float yAt1 = 1.0f - 0.5f * k;
        const float g = (yAt1 > 0.0f) ? (1.0f / yAt1) : 1.0f;
        const float m = 1.0f + (g - 1.0f) * s;

        const float y = y0 * m;
        return (y > 1.0f) ? 1.0f : ((y < 0.0f) ? 0.0f : y);
    }

    float nonlinearity(float x) const
    {
        const float u = std::abs(x);
        const float y = applyClipCurve(u);
        return std::copysign(y, x);
    }

    float integralPositive(float u) const
    {
        if (u <= 0.0f) return 0.0f;

        if (knee <= 0.0f)
        {
            if (u <= 1.0f) return 0.5f * u * u;
            return 0.5f + (u - 1.0f);
        }

        const float start = adaa.start;

        if (u <= start)
            return 0.5f * u * u;

        const float base = 0.5f * start * start;

        if (u >= 1.0f)
            return base + adaa.integralKneeRegion + (u - 1.0f);

        const float dx = u - start;
        const float c0 = start;
        const float c1 = 1.0f;
        const float dx2 = dx * dx;
        const float dx3 = dx2 * dx;
        const float dx4 = dx2 * dx2;
        const float dx5 = dx4 * dx;
        const float dx6 = dx3 * dx3;

        const float integral =
            (c0 * dx)
            + (c1 * dx2) * 0.5f
            + (adaa.n2 * dx3) / 3.0f
            + (adaa.n3 * dx4) / 4.0f
            + (adaa.n4 * dx5) / 5.0f
            + (adaa.n5 * dx6) / 6.0f;

        return base + integral;
    }

    float antiderivative(float x) const
    {
        return integralPositive(std::abs(x));
    }

    float processADAA(float x, float& xPrev) const
    {
        if (!std::isfinite(x))
        {
            xPrev = 0.0f;
            return 0.0f;
        }

        if (!std::isfinite(xPrev))
            xPrev = x;

        if (knee <= 0.0f)
        {
            xPrev = x;
            return nonlinearity(x);
        }

        const float absX = std::abs(x);
        const float absPrev = std::abs(xPrev);
        const float linearLimit = (knee > 0.0f) ? adaa.start : 1.0f;
        if (absX <= linearLimit && absPrev <= linearLimit)
        {
            xPrev = x;
            return x;
        }

        const float dx = x - xPrev;
        float y = 0.0f;

        if (std::abs(dx) < 1.0e-9f)
        {
            y = nonlinearity(x);
        }
        else
        {
            const float Fx = antiderivative(x);
            const float Fp = antiderivative(xPrev);
            y = (Fx - Fp) / dx;
        }

        xPrev = x;
        return clampf(y, -1.0f, 1.0f);
    }
};
