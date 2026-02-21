#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>
#include <optional>

namespace LimiterCurves
{
    struct Curve
    {
        float start;
        float end;
        bool isExponential;
        float minVal;
        float maxVal;
    };

    static inline float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
    
    static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
    
    static inline float expLerp(float a, float b, float t) {
        if (a <= 0.0f || b <= 0.0f) return a + (b - a) * t;
        return a * std::pow(b / a, t);
    }

    static inline float calculate(float character, const Curve& c)
    {
        float t = clampf(character, 0.0f, 1.0f);
        float val;
        if (c.isExponential)
            val = expLerp(c.start, c.end, t);
        else
            val = lerp(c.start, c.end, t);
        
        return clampf(val, c.minVal, c.maxVal);
    }

    inline std::optional<Curve> getCurve(const juce::String& paramID)
    {
        if (paramID == "modern_hold_ms")                  return Curve{ 8.0f, 0.8f, false, 0.0f, 10.0f };
        if (paramID == "modern_hold_release_ms")          return Curve{ 4.0f, 0.35f, true, 0.05f, 10.0f };
        if (paramID == "modern_attack_tau_div")           return Curve{ 3.2f, 1.6f, false, 0.25f, 8.0f };
        if (paramID == "modern_release_smooth_base_ms")   return Curve{ 4.0f, 0.6f, false, 0.0f, 20.0f };
        if (paramID == "modern_release_smooth_range_ms")  return Curve{ 18.0f, 4.0f, false, 0.0f, 50.0f };
        if (paramID == "modern_adapt_fast_strength")      return Curve{ 2.2f, 1.2f, false, 0.0f, 8.0f };
        if (paramID == "modern_adapt_slow_strength")      return Curve{ 4.5f, 2.0f, false, 0.0f, 16.0f };
        if (paramID == "modern_sc_hpf_hz")                return Curve{ 80.0f, 180.0f, false, 20.0f, 400.0f };
        if (paramID == "modern_transient_mix")            return Curve{ 0.15f, 0.75f, false, 0.0f, 1.0f };
        if (paramID == "modern_release_fast_ms")          return Curve{ 6.0f, 0.7f, true, 0.05f, 20.0f };
        if (paramID == "modern_release_slow_ms")          return Curve{ 90.0f, 18.0f, true, 0.1f, 200.0f };
        if (paramID == "modern_link_transients")          return Curve{ 0.95f, 0.35f, false, 0.0f, 1.0f };
        if (paramID == "modern_link_release")             return Curve{ 0.99f, 0.75f, false, 0.0f, 1.0f };
        
        return std::nullopt;
    }
}
