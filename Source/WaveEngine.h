#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <vector>

// ═══════════════════════════════════════════════════════════════════
//  WaveEngine v4
//  - 5 Wellenformen: Lorenz, FM Feedback, Granular, Saw, Square
//  - getMorphSample(): fließender Übergang zwischen allen 5
//    morph 0.0 = Lorenz ... 1.0 = FM ... 2.0 = Saw ... 3.0 = Square
//    (kontinuierlich interpoliert, kein Sprung)
// ═══════════════════════════════════════════════════════════════════

class WaveEngine
{
public:
    static constexpr int TABLE_SIZE = 2048;
    static constexpr int NUM_WAVES  = 5;

    enum class WaveType { Lorenz = 0, FMFeedback = 1, Granular = 2, Saw = 3, Square = 4 };

    WaveEngine()
    {
        buildLorenz();
        buildFMFeedback();
        buildSaw();
        buildSquare();
        // Granular wird per getSample mit Lorenz bedient (runtime granular)
    }

    // ── Wavetable lesen (einzelne Wellenform) ─────────────────────
    float getSample (WaveType type, double phase) const
    {
        const auto& table = tableFor (type);
        double pos = phase * TABLE_SIZE;
        int ia = static_cast<int>(pos) % TABLE_SIZE;
        int ib = (ia + 1) % TABLE_SIZE;
        float fr = static_cast<float>(pos - std::floor(pos));
        return table[ia] * (1.0f - fr) + table[ib] * fr;
    }

    // ── MORPH: fließend zwischen allen Wellenformen ───────────────
    // morphPos: 0.0 = Lorenz, 1.0 = FM, 2.0 = Granular*,
    //           3.0 = Saw,    4.0 = Square
    // (*Granular wird hier als Lorenz-Sample geliefert —
    //  die echte Granular-Logik läuft in SynthVoice)
    float getMorphSample (double phase, float morphPos) const
    {
        morphPos = juce::jlimit (0.0f, 4.0f, morphPos);
        int   wA  = static_cast<int>(morphPos);
        int   wB  = juce::jmin (wA + 1, NUM_WAVES - 1);
        float t   = morphPos - static_cast<float>(wA);

        float sA = getSampleFromIndex (wA, phase);
        float sB = getSampleFromIndex (wB, phase);

        // Crossfade mit Equal-Power Kurve → kein Lautstärkeeinbruch
        float tA = std::cos (t * juce::MathConstants<float>::halfPi);
        float tB = std::sin (t * juce::MathConstants<float>::halfPi);
        return sA * tA + sB * tB;
    }

    // ── PolyBLEP Sägezahn ─────────────────────────────────────────
    static float sawPolyBLEP (double phase, double phaseInc)
    {
        float saw = static_cast<float>(2.0 * phase - 1.0);
        saw -= polyBLEP (phase, phaseInc);
        return saw;
    }

    // ── Square mit PWM ────────────────────────────────────────────
    static float squarePWM (double phase, double phaseInc, float pulseWidth)
    {
        float pw = juce::jlimit (0.05f, 0.95f, pulseWidth);
        float saw1 = static_cast<float>(2.0 * phase - 1.0);
        saw1 -= polyBLEP (phase, phaseInc);
        double phase2 = phase + static_cast<double>(pw);
        if (phase2 >= 1.0) phase2 -= 1.0;
        float saw2 = static_cast<float>(2.0 * phase2 - 1.0);
        saw2 -= polyBLEP (phase2, phaseInc);
        return (saw1 - saw2 - (2.0f * pw - 1.0f)) * 0.5f;
    }

private:
    std::vector<float> lorenz;
    std::vector<float> fmFeedback;
    std::vector<float> sawTable;
    std::vector<float> squareTable;

    // Wellenform per Index abrufen (für Morph)
    float getSampleFromIndex (int idx, double phase) const
    {
        double pos = phase * TABLE_SIZE;
        int ia = static_cast<int>(pos) % TABLE_SIZE;
        int ib = (ia + 1) % TABLE_SIZE;
        float fr = static_cast<float>(pos - std::floor(pos));

        const std::vector<float>* tbl = &lorenz;
        switch (idx) {
            case 1: tbl = &fmFeedback; break;
            case 2: tbl = &lorenz;     break;  // Granular = Lorenz hier
            case 3: tbl = &sawTable;   break;
            case 4: tbl = &squareTable;break;
            default: break;
        }
        return (*tbl)[ia] * (1.0f - fr) + (*tbl)[ib] * fr;
    }

    const std::vector<float>& tableFor (WaveType t) const
    {
        switch (t) {
            case WaveType::FMFeedback: return fmFeedback;
            case WaveType::Saw:        return sawTable;
            case WaveType::Square:     return squareTable;
            case WaveType::Granular:   return lorenz;
            default:                   return lorenz;
        }
    }

    static float polyBLEP (double t, double dt)
    {
        if (dt <= 0.0) return 0.0f;
        if (t < dt) {
            double x = t / dt;
            return static_cast<float>(x + x - x * x - 1.0);
        } else if (t > 1.0 - dt) {
            double x = (t - 1.0) / dt;
            return static_cast<float>(x * x + x + x + 1.0);
        }
        return 0.0f;
    }

    void buildLorenz()
    {
        lorenz.resize (TABLE_SIZE);
        const float sigma = 10.0f, rho = 28.0f, beta = 2.6667f;
        float x = 0.1f, y = 0.0f, z = 20.0f;
        const float dt = 0.002f;
        for (int i = 0; i < 8000; ++i) {
            float dx = sigma*(y-x), dy = x*(rho-z)-y, dz = x*y-beta*z;
            x += dx*dt; y += dy*dt; z += dz*dt;
        }
        for (int i = 0; i < TABLE_SIZE; ++i) {
            float dx = sigma*(y-x), dy = x*(rho-z)-y, dz = x*y-beta*z;
            x += dx*dt; y += dy*dt; z += dz*dt;
            lorenz[i] = x;
        }
        normalise (lorenz);
    }

    void buildFMFeedback()
    {
        fmFeedback.resize (TABLE_SIZE);
        float prev = 0.0f;
        for (int i = 0; i < TABLE_SIZE; ++i) {
            float t  = static_cast<float>(i) / TABLE_SIZE;
            float ph = t * juce::MathConstants<float>::twoPi + t * 6.0f * prev;
            float s  = std::sin (ph);
            prev = s;
            fmFeedback[i] = s;
        }
        normalise (fmFeedback);
    }

    void buildSaw()
    {
        sawTable.resize (TABLE_SIZE);
        // Bandbegrenzter Sägezahn über Additive Synthese (16 Harmonische)
        for (int i = 0; i < TABLE_SIZE; ++i) {
            float t = static_cast<float>(i) / TABLE_SIZE;
            float s = 0.0f;
            for (int h = 1; h <= 16; ++h)
                s += std::sin (h * t * juce::MathConstants<float>::twoPi) / static_cast<float>(h);
            sawTable[i] = s * (2.0f / juce::MathConstants<float>::pi);
        }
        normalise (sawTable);
    }

    void buildSquare()
    {
        squareTable.resize (TABLE_SIZE);
        // Bandbegrenztes Square (nur ungerade Harmonische)
        for (int i = 0; i < TABLE_SIZE; ++i) {
            float t = static_cast<float>(i) / TABLE_SIZE;
            float s = 0.0f;
            for (int h = 1; h <= 16; h += 2)
                s += std::sin (h * t * juce::MathConstants<float>::twoPi) / static_cast<float>(h);
            squareTable[i] = s * (4.0f / juce::MathConstants<float>::pi);
        }
        normalise (squareTable);
    }

    static void normalise (std::vector<float>& t)
    {
        float peak = 0.0f;
        for (float s : t) peak = std::max (peak, std::abs (s));
        if (peak > 1e-6f)
            for (float& s : t) s /= peak;
    }
};
