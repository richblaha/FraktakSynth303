#pragma once
#include <JuceHeader.h>
#include <cmath>

// ═══════════════════════════════════════════════════════════════════
//  LadderFilter  –  Moog-style 4-pole low-pass (Huovilainen model)
//  cutoff: 20–20000 Hz   resonance: 0–1 (self-oscillates at ~0.95)
// ═══════════════════════════════════════════════════════════════════

class LadderFilter
{
public:
    LadderFilter() { reset(); }

    void setSampleRate (double sr) { sampleRate = sr; }

    void setParameters (float cutoffHz, float res)
    {
        cutoff    = juce::jlimit (20.0f, 20000.0f, cutoffHz);
        resonance = juce::jlimit (0.0f,  1.0f,     res);
        updateCoefficients();
    }

    float process (float x)
    {
        // input mit resonanz- feedback
        float res = resonance * resonance; // nicht linear → musikalischer

        float input = x - res * 2.2f * s[3];

        //303: Input Drive + Compensation
        input *= (1.0f + res * 0.8f);

        //leichte Sättigung
        input = std::tanh(input);

        // four one-pole stages
        float a0 = input  * g;
        s[0] = a0 + 2.0f * (s[0] - a0) * 0.5f;  // simplified Huovilainen
        float a1 = s[0] * g;
        s[1] = a1 + 2.0f * (s[1] - a1) * 0.5f;
        float a2 = s[1] * g;
        s[2] = a2 + 2.0f * (s[2] - a2) * 0.5f;
        float a3 = s[2] * g;
        s[3] = a3 + 2.0f * (s[3] - a3) * 0.5f;

        // proper TPT (trapezoidal) one-pole lowpass
        updateStage (0, input);
        updateStage (1, s[0]);
        updateStage (2, s[1]);
        updateStage (3, s[2]);

        return s[3];
    }

    void reset()
    {
        for (auto& v : s) v = 0.0f;
        for (auto& v : z) v = 0.0f;
    }

private:
    double sampleRate = 44100.0;
    float  cutoff     = 1000.0f;
    float  resonance  = 0.0f;
    float  g          = 0.0f;
    float  s[4]       = {};
    float  z[4]       = {};

    void updateCoefficients()
    {
        float wd = juce::MathConstants<float>::twoPi * cutoff;
        float T  = 1.0f / static_cast<float>(sampleRate);
        float wa = (2.0f / T) * std::tan (wd * T * 0.5f);
        g  = wa * T * 0.5f;
        g /= (1.0f + g);
    }

    void updateStage (int i, float input)
    {
        float v = (input - z[i]) * g;
        float out = v + z[i];
        z[i] = out + v;
        s[i] = out;
    }
};
