#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <vector>

class LimiterModern
{
public:
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
        advHoldMs = holdMs;
        advHoldReleaseMs = holdReleaseMs;
        advAttackTauDiv = attackTauDiv;
        advReleaseSmoothBaseMs = releaseSmoothBaseMs;
        advReleaseSmoothRangeMs = releaseSmoothRangeMs;
        advAdaptFastStrength = adaptFastStrength;
        advAdaptSlowStrength = adaptSlowStrength;
        advScHpfHz = scHpfHz;
        advRatioBase = ratioBase;
        advRatioSlope = ratioSlope;
        advTransientMix = transientMix;
        advSoftSafetyStrength = softSafetyStrength;
        advModernReleaseFastMs = modernReleaseFastMs;
        advModernReleaseSlowMs = modernReleaseSlowMs;
        advModernLinkTransients = modernLinkTransients;
        advModernLinkRelease = modernLinkRelease;
    }

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        // Allocate for max possible lookahead (5ms) at max oversampling (16x)
        // 5ms = 0.005s. With 192kHz * 16 = 3.072MHz.
        // 0.005 * 3072000 = 15360 samples.
        // Let's use a safe upper bound.
        // currentSampleRate here is already oversampled rate if oversampling is active.
        // We assume max 5ms lookahead.
        
        size_t maxSamples = (size_t)(0.005 * currentSampleRate) + 64; 
        // Ensure at least 1 sample to avoid div by zero or empty
        if (maxSamples < 1) maxSamples = 1;

        lookaheadBufferL.assign(maxSamples, 0.0f);
        lookaheadBufferR.assign(maxSamples, 0.0f);
        writePos = 0;

        peakDeque.assign(maxSamples + 1, PeakNode { 0u, 0.0f });
        peakDequeHead = 0;
        peakDequeTail = 0;
        peakSampleIndex = 0;

        fastEnvL = 0.0;
        fastEnvR = 0.0;
        slowEnvL = 0.0;
        slowEnvR = 0.0;
        smoothGainStage1L = 0.0;
        smoothGainStage1R = 0.0;
        smoothGainL = 0.0;
        smoothGainR = 0.0;
        peakHoldL = 0.0;
        peakHoldR = 0.0;
        peakHoldTimeL = 0;
        peakHoldTimeR = 0;
        scHpfL = 0.0;
        scHpfR = 0.0;
        lastScInputL = 0.0;
        lastScInputR = 0.0;

        patchGain = 1.0f;
        patchTarget = 1.0f;
        patchPhase = PatchPhase::Idle;
        patchPos = 0;
        patchAttackSamples = 0;
        patchReleaseSamples = 0;
        patchCooldownSamples = 0;
        peakTrackActive = false;
        peakTrackMax = 0.0f;
        peakTrackMaxL = 0.0f;
        peakTrackMaxR = 0.0f;
        peakTrackLast = 0.0f;

        currentGR = 0.0f;
        currentGR_L = 0.0f;
        currentGR_R = 0.0f;
        
        currentDelaySamples = 0;
    }

    void setParameters(float newCeilingLin, float newCharacter, float newLookaheadMs)
    {
        ceiling = newCeilingLin;
        character = newCharacter;

        releaseSlowMs = std::max(0.1f, advModernReleaseSlowMs);
        releaseFastMs = std::max(0.05f, advModernReleaseFastMs);
        if (releaseSlowMs < releaseFastMs) releaseSlowMs = releaseFastMs;
        linkTransients = std::max(0.0f, std::min(1.0f, advModernLinkTransients));
        linkRelease = std::max(0.0f, std::min(1.0f, advModernLinkRelease));

        targetLookaheadMs = std::max(0.0f, newLookaheadMs);
        
        // Update delay samples based on current sample rate and target ms
        // No allocation here!
        currentDelaySamples = (size_t)(targetLookaheadMs * 0.001 * currentSampleRate);
        
        // Safety clamp against buffer size (though prepare should have covered it)
        if (lookaheadBufferL.size() > 0 && currentDelaySamples >= lookaheadBufferL.size())
            currentDelaySamples = lookaheadBufferL.size() - 1;
    }

    void process(float& left, float& right)
    {
        size_t bufferSize = lookaheadBufferL.size();
        if (bufferSize == 0) return;

        lookaheadBufferL[writePos] = left;
        lookaheadBufferR[writePos] = right;

        // Ring buffer read
        // readIndex = writePos - currentDelaySamples
        // Handle wrap-around
        int readIndexInt = (int)writePos - (int)currentDelaySamples;
        if (readIndexInt < 0) readIndexInt += (int)bufferSize;
        size_t readIndex = (size_t)readIndexInt;

        float delayedL = lookaheadBufferL[readIndex];
        float delayedR = lookaheadBufferR[readIndex];

        float inputL = left;
        float inputR = right;

        float scHz = std::max(20.0f, std::min(400.0f, advScHpfHz));
        float hpfCoeff = std::exp(-2.0f * 3.14159f * scHz / (float)currentSampleRate);
        float hpfL = (float)(hpfCoeff * (scHpfL + inputL - lastScInputL));
        float hpfR = (float)(hpfCoeff * (scHpfR + inputR - lastScInputR));

        scHpfL = hpfL;
        scHpfR = hpfR;
        lastScInputL = inputL;
        lastScInputR = inputR;

        float detL = std::abs(inputL);
        float detR = std::abs(inputR);
        const float rawDetL = detL;
        const float rawDetR = detR;

        float lookaheadPeakDb = 0.0f;
        {
            const float detMaxNow = std::max(detL, detR);
            const uint64_t idx = peakSampleIndex++;
            const size_t cap = peakDeque.size();

            if (cap > 1)
            {
                // Remove from tail if value <= new value (standard sliding window max)
                while (peakDequeHead != peakDequeTail)
                {
                    const size_t last = (peakDequeTail == 0) ? (cap - 1) : (peakDequeTail - 1);
                    if (peakDeque[last].value >= detMaxNow)
                        break;
                    peakDequeTail = last;
                }

                peakDeque[peakDequeTail] = PeakNode { idx, detMaxNow };
                peakDequeTail = (peakDequeTail + 1) % cap;

                // Remove from head if index is out of window
                // Window size is currentDelaySamples.
                // We keep samples in [idx - currentDelaySamples, idx]
                // So min index allowed is idx - currentDelaySamples.
                
                const uint64_t minIdx = (idx >= (uint64_t)currentDelaySamples) ? (idx - (uint64_t)currentDelaySamples) : 0u;
                
                while (peakDequeHead != peakDequeTail && peakDeque[peakDequeHead].index < minIdx)
                    peakDequeHead = (peakDequeHead + 1) % cap;

                const float winMax = (peakDequeHead != peakDequeTail) ? peakDeque[peakDequeHead].value : detMaxNow;
                lookaheadPeakDb = (winMax > 1.0f) ? (20.0f * std::log10(winMax)) : 0.0f;
            }
        }

        float wideDetectL = rawDetL;
        float wideDetectR = rawDetR;

        auto smoothstep01 = [](float t) {
            t = std::max(0.0f, std::min(1.0f, t));
            return t * t * (3.0f - 2.0f * t);
        };

        float overshootL_DB = (detL > 1.0f) ? 20.0f * std::log10(detL) : 0.0f;
        float overshootR_DB = (detR > 1.0f) ? 20.0f * std::log10(detR) : 0.0f;

        float absWide = 0.5f * (wideDetectL + wideDetectR);
        float absHpf = 0.5f * (std::abs(hpfL) + std::abs(hpfR));
        float hfRatio = absWide > 0.000001f ? (absHpf / absWide) : 1.0f;
        hfRatio = std::max(0.0f, std::min(1.0f, hfRatio));
        float lowBias = 1.0f - hfRatio;

        float grDbProxy = std::max({ (float) fastEnvL, (float) fastEnvR, (float) slowEnvL, (float) slowEnvR, overshootL_DB, overshootR_DB });
        float grNorm = std::max(0.0f, std::min(1.0f, grDbProxy / 24.0f));

        const float charT = std::max(0.0f, std::min(1.0f, character));
        auto expLerp = [](float a, float b, float t) {
            if (a <= 0.0f || b <= 0.0f) return a + (b - a) * t;
            return a * std::pow(b / a, t);
        };

        float fastStrength = std::max(0.0f, std::min(8.0f, advAdaptFastStrength));
        float slowStrength = std::max(0.0f, std::min(16.0f, advAdaptSlowStrength));
        fastStrength *= expLerp(0.85f, 1.35f, charT);
        slowStrength *= expLerp(0.9f, 1.25f, charT);
        const float releaseFastMsBase = releaseFastMs * expLerp(1.25f, 0.65f, charT);
        const float releaseSlowMsBase = releaseSlowMs * expLerp(1.2f, 0.7f, charT);
        float effectiveFastMs = releaseFastMsBase * (1.0f + fastStrength * lowBias * grNorm);
        float effectiveSlowMs = releaseSlowMsBase * (1.0f + slowStrength * lowBias * grNorm);
        effectiveFastMs = std::max(0.05f, effectiveFastMs);
        effectiveSlowMs = std::max(effectiveFastMs, effectiveSlowMs);

        float fastCoeff = std::exp(-1.0f / (effectiveFastMs * 0.001f * (float)currentSampleRate));
        float slowCoeff = std::exp(-1.0f / (effectiveSlowMs * 0.001f * (float)currentSampleRate));

        if (overshootL_DB > fastEnvL) fastEnvL = overshootL_DB; else fastEnvL *= fastCoeff;
        if (overshootR_DB > fastEnvR) fastEnvR = overshootR_DB; else fastEnvR *= fastCoeff;

        if (overshootL_DB > slowEnvL) slowEnvL = overshootL_DB; else slowEnvL *= slowCoeff;
        if (overshootR_DB > slowEnvR) slowEnvR = overshootR_DB; else slowEnvR *= slowCoeff;

        auto getAdaptiveAtten = [&](float fast, float slow) {
            float ratio = (slow > 0.001f) ? fast / slow : 2.0f;
            float ratioBase = std::max(0.1f, advRatioBase);
            float ratioSlope = std::max(0.0f, std::min(1.5f, advRatioSlope));
            float ratioThresh = ratioBase * (1.0f - character * ratioSlope);
            if (ratioThresh < 0.05f) ratioThresh = 0.05f;

            if (ratio > ratioThresh) {
                float mix = std::max(0.0f, std::min(1.0f, advTransientMix * (1.0f - 0.65f * charT)));
                return std::max(fast, slow * mix);
            } else {
                return std::max(fast, slow);
            }
        };

        float maxFast = (float)std::max(fastEnvL, fastEnvR);
        const float lt = std::max(0.0f, std::min(1.0f, linkTransients * (1.0f - 0.6f * charT)));
        fastEnvL = fastEnvL + (maxFast - fastEnvL) * lt;
        fastEnvR = fastEnvR + (maxFast - fastEnvR) * lt;

        float maxSlow = (float)std::max(slowEnvL, slowEnvR);
        const float lr = std::max(0.0f, std::min(1.0f, linkRelease * (1.0f - 0.35f * charT)));
        slowEnvL = slowEnvL + (maxSlow - slowEnvL) * lr;
        slowEnvR = slowEnvR + (maxSlow - slowEnvR) * lr;

        float attenL = getAdaptiveAtten((float)fastEnvL, (float)slowEnvL);
        float attenR = getAdaptiveAtten((float)fastEnvR, (float)slowEnvR);

        const float requiredAttenDb = std::max(0.0f, lookaheadPeakDb);
        attenL = std::max(attenL, requiredAttenDb);
        attenR = std::max(attenR, requiredAttenDb);

        float holdMs = std::max(0.0f, std::min(10.0f, advHoldMs));
        int holdSamples = (int)((holdMs * 0.001f) * currentSampleRate);
        float holdReleaseMs = std::max(0.05f, std::min(10.0f, advHoldReleaseMs));
        float holdReleaseCoeff = 1.0f - std::exp(-1.0f / ((holdReleaseMs * 0.001f) * (float)currentSampleRate));

        auto applyHold = [&](double& peakHold, int& timer, float input) {
            if (input > (float) peakHold)
            {
                peakHold = input;
                timer = holdSamples;
            }
            else if (timer > 0)
            {
                timer--;
            }
            else
            {
                peakHold += (input - (float) peakHold) * holdReleaseCoeff;
            }
            return (float) peakHold;
        };

        float holdL = applyHold(peakHoldL, peakHoldTimeL, attenL);
        float holdR = applyHold(peakHoldR, peakHoldTimeR, attenR);

        float tauDiv = std::max(0.25f, std::min(8.0f, advAttackTauDiv * expLerp(0.85f, 1.35f, charT)));
        float attackTau = targetLookaheadMs * 0.001f / tauDiv;
        float smoothAttackCoeff = 1.0f - std::exp(-1.0f / (attackTau * (float)currentSampleRate));
        float baseMs = std::max(0.0f, std::min(20.0f, advReleaseSmoothBaseMs * expLerp(1.25f, 0.7f, charT)));
        float rangeMs = std::max(0.0f, std::min(50.0f, advReleaseSmoothRangeMs * expLerp(1.2f, 0.75f, charT)));
        float releaseSmoothMs = baseMs + rangeMs * lowBias * grNorm;
        float smoothReleaseCoeff = 1.0f - std::exp(-1.0f / ((releaseSmoothMs * 0.001f) * (float)currentSampleRate));

        if (holdL > smoothGainStage1L) smoothGainStage1L += smoothAttackCoeff * (holdL - smoothGainStage1L);
        else smoothGainStage1L += smoothReleaseCoeff * (holdL - smoothGainStage1L);

        if (holdR > smoothGainStage1R) smoothGainStage1R += smoothAttackCoeff * (holdR - smoothGainStage1R);
        else smoothGainStage1R += smoothReleaseCoeff * (holdR - smoothGainStage1R);

        if (smoothGainStage1L > smoothGainL) smoothGainL += smoothAttackCoeff * (smoothGainStage1L - smoothGainL);
        else smoothGainL += smoothReleaseCoeff * (smoothGainStage1L - smoothGainL);

        if (smoothGainStage1R > smoothGainR) smoothGainR += smoothAttackCoeff * (smoothGainStage1R - smoothGainR);
        else smoothGainR += smoothReleaseCoeff * (smoothGainStage1R - smoothGainR);

        if (patchCooldownSamples > 0)
            --patchCooldownSamples;

        const float transientDbL = std::max(0.0f, (float) fastEnvL - (float) slowEnvL);
        const float transientDbR = std::max(0.0f, (float) fastEnvR - (float) slowEnvR);
        const float transientDb = std::max(transientDbL, transientDbR);
        const bool isTransient = (transientDb > 0.35f);

        const float detMax = std::max(detL, detR);
        constexpr float patchMarginDb = 0.2f;
        const float patchMarginLin = std::pow(10.0f, patchMarginDb / 20.0f);

        if (detMax > patchMarginLin)
        {
            if (!peakTrackActive)
            {
                peakTrackActive = true;
                peakTrackMax = detMax;
                peakTrackMaxL = detL;
                peakTrackMaxR = detR;
                peakTrackLast = detMax;
            }
            else
            {
                if (detMax > peakTrackMax)
                {
                    peakTrackMax = detMax;
                    peakTrackMaxL = detL;
                    peakTrackMaxR = detR;
                }

                const bool falling = (detMax < peakTrackLast);
                peakTrackLast = detMax;

                if (falling && isTransient && patchCooldownSamples == 0 && patchPhase == PatchPhase::Idle)
                {
                    const float peakL = std::max(1.0e-9f, peakTrackMaxL);
                    const float peakR = std::max(1.0e-9f, peakTrackMaxR);

                    const float requiredDbL = (peakL > 1.0f) ? 20.0f * std::log10(peakL) : 0.0f;
                    const float requiredDbR = (peakR > 1.0f) ? 20.0f * std::log10(peakR) : 0.0f;

                    // Delay samples is now dynamic
                    const int delaySamples = (int)currentDelaySamples;
                    
                    const int attackSamples = std::max(1, delaySamples);
                    const int releaseSamples = std::max(1, (int) std::lround(0.004f * (float) currentSampleRate));

                    auto predictDbAt = [&](float s1, float s2, float targetDb) {
                        const float target = std::max(0.0f, targetDb);
                        float a1 = s1;
                        float a2 = s2;
                        for (int i = 0; i < attackSamples; ++i)
                        {
                            const float c1 = (target > a1) ? smoothAttackCoeff : smoothReleaseCoeff;
                            const float c2 = (a1 > a2) ? smoothAttackCoeff : smoothReleaseCoeff;
                            a1 += c1 * (target - a1);
                            a2 += c2 * (a1 - a2);
                        }
                        return a2;
                    };

                    const float predDbL = predictDbAt((float) smoothGainStage1L, (float) smoothGainL, requiredDbL);
                    const float predDbR = predictDbAt((float) smoothGainStage1R, (float) smoothGainR, requiredDbR);

                    const float predGainL = std::pow(10.0f, -predDbL / 20.0f);
                    const float predGainR = std::pow(10.0f, -predDbR / 20.0f);

                    const float reqGainL = (peakL > 1.0f) ? (1.0f / peakL) : 1.0f;
                    const float reqGainR = (peakR > 1.0f) ? (1.0f / peakR) : 1.0f;

                    const float addL = reqGainL / std::max(1.0e-9f, predGainL);
                    const float addR = reqGainR / std::max(1.0e-9f, predGainR);

                    const float target = std::max(0.0f, std::min(1.0f, std::min(addL, addR)));

                    if (target < 0.9995f)
                    {
                        patchGain = 1.0f;
                        patchTarget = target;
                        patchPhase = PatchPhase::Attack;
                        patchPos = 0;
                        patchAttackSamples = attackSamples;
                        patchReleaseSamples = releaseSamples;
                        patchCooldownSamples = std::max(1, (int) std::lround(0.010f * (float) currentSampleRate));
                    }

                    peakTrackActive = false;
                }
            }
        }
        else
        {
            peakTrackActive = false;
        }

        float patchNow = 1.0f;
        if (patchPhase == PatchPhase::Attack)
        {
            const float t = (patchAttackSamples > 0) ? ((float) patchPos / (float) patchAttackSamples) : 1.0f;
            float u = smoothstep01(t);
            u = u * u;
            u = u * u;
            patchNow = 1.0f + (patchTarget - 1.0f) * u;
            if (++patchPos >= patchAttackSamples)
            {
                patchPhase = PatchPhase::Release;
                patchPos = 0;
                patchNow = patchTarget;
            }
        }
        else if (patchPhase == PatchPhase::Release)
        {
            const float t = (patchReleaseSamples > 0) ? ((float) patchPos / (float) patchReleaseSamples) : 1.0f;
            float u = smoothstep01(t);
            float oneMinus = 1.0f - u;
            oneMinus = oneMinus * oneMinus;
            oneMinus = oneMinus * oneMinus;
            u = 1.0f - oneMinus;
            patchNow = patchTarget + (1.0f - patchTarget) * u;
            if (++patchPos >= patchReleaseSamples)
            {
                patchPhase = PatchPhase::Idle;
                patchPos = 0;
                patchTarget = 1.0f;
                patchNow = 1.0f;
            }
        }

        patchGain = patchNow;

        float linearGainL = std::pow(10.0f, -(float)smoothGainL / 20.0f);
        float linearGainR = std::pow(10.0f, -(float)smoothGainR / 20.0f);

        const float effectiveGainL = linearGainL * patchGain;
        const float effectiveGainR = linearGainR * patchGain;

        float outL = delayedL * effectiveGainL;
        float outR = delayedR * effectiveGainR;

        left = outL * ceiling;
        right = outR * ceiling;

        auto softSafetyLimit = [&](float x) {
            float a = std::abs(x);
            if (a <= ceiling) return x;
            float over = (a - ceiling) / ceiling;
            float strength = std::max(0.0f, std::min(20.0f, advSoftSafetyStrength));
            float alpha = 1.0f + strength * over;
            float y = a / (1.0f + alpha * over);
            return std::copysign(y, x);
        };

        left = softSafetyLimit(left);
        right = softSafetyLimit(right);

        if (left > ceiling) left = ceiling; else if (left < -ceiling) left = -ceiling;
        if (right > ceiling) right = ceiling; else if (right < -ceiling) right = -ceiling;

        currentGR_L = 1.0f - effectiveGainL;
        currentGR_R = 1.0f - effectiveGainR;
        float minGain = std::min(effectiveGainL, effectiveGainR);
        currentGR = 1.0f - minGain;

        writePos = (writePos + 1) % bufferSize;
    }

    float getGainReduction() const { return currentGR; }
    float getGainReductionLeft() const { return currentGR_L; }
    float getGainReductionRight() const { return currentGR_R; }

private:
    float linkTransients = 0.5f;
    float linkRelease = 0.95f;

    float advHoldMs = 4.0f;
    float advHoldReleaseMs = 1.5f;
    float advAttackTauDiv = 2.5f;
    float advReleaseSmoothBaseMs = 1.5f;
    float advReleaseSmoothRangeMs = 6.0f;
    float advAdaptFastStrength = 1.5f;
    float advAdaptSlowStrength = 3.0f;
    float advScHpfHz = 120.0f;
    float advRatioBase = 1.5f;
    float advRatioSlope = 0.9f;
    float advTransientMix = 0.3f;
    float advSoftSafetyStrength = 6.0f;
    float advModernReleaseFastMs = 2.0f;
    float advModernReleaseSlowMs = 30.0f;
    float advModernLinkTransients = 0.50f;
    float advModernLinkRelease = 0.95f;

    double currentSampleRate = 44100.0;
    float ceiling = 1.0f;
    float character = 0.0f;
    float currentGR = 0.0f;
    float currentGR_L = 0.0f;
    float currentGR_R = 0.0f;

    std::vector<float> lookaheadBufferL;
    std::vector<float> lookaheadBufferR;
    size_t writePos = 0;
    size_t currentDelaySamples = 0;

    double fastEnvL = 0.0, fastEnvR = 0.0;
    double slowEnvL = 0.0, slowEnvR = 0.0;
    double smoothGainStage1L = 0.0, smoothGainStage1R = 0.0;
    double smoothGainL = 0.0, smoothGainR = 0.0;

    float releaseFastMs = 10.0f;
    float releaseSlowMs = 500.0f;
    float targetLookaheadMs = 2.0f;

    double peakHoldL = 0.0;
    double peakHoldR = 0.0;
    int peakHoldTimeL = 0;
    int peakHoldTimeR = 0;

    double scHpfL = 0.0, scHpfR = 0.0;
    double lastScInputL = 0.0, lastScInputR = 0.0;

    struct PeakNode
    {
        uint64_t index = 0;
        float value = 0.0f;
    };

    std::vector<PeakNode> peakDeque;
    size_t peakDequeHead = 0;
    size_t peakDequeTail = 0;
    uint64_t peakSampleIndex = 0;

    enum class PatchPhase { Idle, Attack, Release };
    float patchGain = 1.0f;
    float patchTarget = 1.0f;
    PatchPhase patchPhase = PatchPhase::Idle;
    int patchPos = 0;
    int patchAttackSamples = 0;
    int patchReleaseSamples = 0;
    int patchCooldownSamples = 0;

    bool peakTrackActive = false;
    float peakTrackMax = 0.0f;
    float peakTrackMaxL = 0.0f;
    float peakTrackMaxR = 0.0f;
    float peakTrackLast = 0.0f;
};
