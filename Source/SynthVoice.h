#pragma once
#include <JuceHeader.h>
#include "WaveEngine.h"
#include "LadderFilter.h"
#include <cmath>
#include <array>

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice (const WaveEngine& we, juce::AudioProcessorValueTreeState& apvts)
        : waveEngine (we), tree (apvts) {}

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<juce::SynthesiserSound*>(s) != nullptr;
    }

    void setBPM (float newBpm) { bpm = newBpm; }

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int) override
    {
        targetFreq     = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        frequency      = targetFreq;
        if (currentFreq <= 0.0) currentFreq = targetFreq;

        phase = lfoPhase = stutterPhase = flangerPhase = 0.0;
        shimmerPhase = morphAutoPhase = phaseDestPhase = 0.0;

        // Granular 1: langsam, texturell
        gran1Phase = 0.0f;
        gran1Pos   = juce::Random::getSystemRandom().nextFloat();
        gran1Env   = 0.0f;

        // Granular 2: schnell, etwas glitchy aber nicht zu viel
        gran2Phase = 0.0f;
        gran2Pos   = juce::Random::getSystemRandom().nextFloat();
        gran2Env   = 0.0f;

        grainPhase = 0.0f;
        grainPos   = juce::Random::getSystemRandom().nextFloat();

        smoothedGate = pdFeedback = 0.0f;
        this->velocity = velocity;

        timeFoldBuf.fill (0.0f);
        timeFoldPos  = 0;
        reverbBuf[0] = reverbBuf[1] = 0.0f;
        std::fill (std::begin(delayLine), std::end(delayLine), 0.0f);

        ampEnv.noteOn();
        filterEnv.noteOn();
        filter.reset();
    }

    void stopNote (float, bool allowTailOff) override
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
        if (!allowTailOff) clearCurrentNote();
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override
    {
        if (!ampEnv.isActive()) return;

        // ── Parameter ─────────────────────────────────────────────
        int   waveIdx    = static_cast<int> (*tree.getRawParameterValue ("wave"));
        float cutoff     = *tree.getRawParameterValue ("cutoff");
        float resonance  = *tree.getRawParameterValue ("resonance");
        float drive      = *tree.getRawParameterValue ("drive");
        float envAmt     = *tree.getRawParameterValue ("filterEnvAmt");
        float lfoRate    = *tree.getRawParameterValue ("lfoRate");
        float lfoDepth   = *tree.getRawParameterValue ("lfoDepth");
        float chaosRaw   = *tree.getRawParameterValue ("chaos");
        float chaosAmt   = chaosRaw > 0.8f ? 0.8f : chaosRaw;
        float chaos2     = *tree.getRawParameterValue ("chaos2");
        float stutterAmt = *tree.getRawParameterValue ("stutter");
        float pwm        = *tree.getRawParameterValue ("pwm");
        float morphPos   = *tree.getRawParameterValue ("morph");
        float morphAuto  = *tree.getRawParameterValue ("morphAuto");
        float phaseDest  = *tree.getRawParameterValue ("phaseDest");
        float timeFold   = *tree.getRawParameterValue ("timeFold");
        // Granular Mix — max 0.6 → nie DAW-belastend
        float gran1Mix   = *tree.getRawParameterValue ("gran1");
        float gran2Mix   = *tree.getRawParameterValue ("gran2");
        float accent     = std::pow (velocity, 1.5f);

        const double sr = getSampleRate();

        for (int i = startSample; i < startSample + numSamples; ++i)
        {
            // ── Portamento ────────────────────────────────────────
            currentFreq += (targetFreq - currentFreq) * 0.002;

            // ── Morph Auto ────────────────────────────────────────
            if (morphAuto > 0.001f)
            {
                morphAutoPhase += (0.05 + morphAuto * 0.4) / sr;
                if (morphAutoPhase >= 1.0) morphAutoPhase -= 1.0;
                float autoMod = static_cast<float>(
                    std::sin (morphAutoPhase * juce::MathConstants<double>::twoPi));
                morphPos = juce::jlimit (0.0f, 4.0f, morphPos + autoMod * morphAuto * 2.0f);
            }

            // ── Shimmer / Chaos Pitch - Im Synth: "SMEAR" ─────────────────────────────
            shimmerPhase += (currentFreq * 1.0059) / sr;
            if (shimmerPhase >= 1.0) shimmerPhase -= 1.0;
            float shimSin = static_cast<float>(
                std::sin (shimmerPhase * juce::MathConstants<double>::twoPi));
            float chaosPitch = 1.0f + chaosAmt * 0.02f * shimSin;
            double chaosDrift =
                std::sin (shimmerPhase * 6.28318 * 0.23) *
                std::sin (shimmerPhase * 6.28318 * 1.37);

            // ── Phase Destruction ─────────────────────────────────
            double phaseInc = (currentFreq * chaosPitch *
                               (1.0 + chaosAmt * 0.01 * chaosDrift)) / sr;
            double phaseIncMod = phaseInc;

            if (phaseDest > 0.01f)
            {
                phaseDestPhase += (currentFreq * 0.007 * (1.0 + phaseDest * 3.0)) / sr;
                if (phaseDestPhase >= 1.0) phaseDestPhase -= 1.0;
                double pdMod =
                    std::sin (phaseDestPhase * juce::MathConstants<double>::twoPi * 2.3) *
                    std::cos (phaseDestPhase * juce::MathConstants<double>::twoPi * 3.7);
                double pdFB = juce::jlimit (-0.3, 0.3,
                    static_cast<double>(pdFeedback) * phaseDest * 0.5);
                phaseIncMod = phaseInc * (1.0 + phaseDest * pdMod * 0.4) + pdFB * phaseInc;
            }

            phase += phaseIncMod;
            if (phase >= 1.0) phase -= 1.0;
            if (phase < 0.0)  phase += 1.0;

            // ── Hauptoszillator ───────────────────────────────────
            float osc = 0.0f;

            if (waveIdx == 2)
            {
                grainPhase += (currentFreq * 0.15) / sr;
                if (grainPhase >= 1.0f) {
                    grainPhase -= 1.0f;
                    grainPos = juce::Random::getSystemRandom().nextFloat();
                }
                float lp = grainPos + grainPhase * (0.05f + chaosAmt * 0.2f);
                lp -= std::floor (lp);
                float gs = waveEngine.getSample (WaveEngine::WaveType::Lorenz, lp);
                float ge = 0.5f - 0.5f * std::cos (grainPhase * juce::MathConstants<float>::twoPi);
                osc = gs * std::pow (ge, 1.5f);
            }
            else if (waveIdx == 3)
                osc = WaveEngine::sawPolyBLEP (phase, phaseInc);
            else if (waveIdx == 4)
                osc = WaveEngine::squarePWM (phase, phaseInc, pwm);
            else
                osc = waveEngine.getSample (static_cast<WaveEngine::WaveType>(
                    juce::jlimit(0,2,waveIdx)), phase);

            // Morph überlagern:
            {
                float morphOsc = waveEngine.getMorphSample (phase, morphPos);
                float morphMix = juce::jlimit (0.0f, 1.0f,
                    morphAuto * 0.6f + (morphPos > 0.05f ? 0.3f : 0.0f));
                osc = osc * (1.0f - morphMix) + morphOsc * morphMix;
            }

            pdFeedback = osc * 0.3f;

          
            //  GRANULAR 1 — langsam, texturell (Drift)
            //  Körner: lang (~80ms), zufällig in der Lorenz-Tabelle
            //  CPU-safe: eine Tabellen-Lesung pro Sample
         
            float gran1Out = 0.0f;
            if (gran1Mix > 0.001f)
            {
                // langsame Kornrate (1–3 Hz)
                float g1Rate = 6.0f + chaosAmt * 10.0f; // 6–16 Hz
                gran1Phase += g1Rate / static_cast<float>(sr);
                
                if (gran1Phase >= 1.0f)
                {
                    gran1Phase -= 1.0f;
                 
                    gran1Pos = std::fmod (gran1Pos + 0.07f +
                        juce::Random::getSystemRandom().nextFloat() * 0.15f, 1.0f);
                    gran1Env = 0.0f;
                }
                // Hanning Envelope
                float ge1 = 0.5f - 0.5f * std::cos (gran1Phase * juce::MathConstants<float>::twoPi);
                ge1 = std::pow (ge1, 0.5f); // mehr Präsenz // weicher als Granular 2

                // Leseposition: leicht pitch-verschoben (organisch)
                float lp1 = gran1Pos + gran1Phase * 0.2f;
                lp1 -= std::floor (lp1);
                gran1Out = waveEngine.getSample (WaveEngine::WaveType::Lorenz, lp1) * ge1;
            }

            
            //  GRANULAR 2 — schnell, glitchy (Körner)
            //  Kornrate: 8–20 Hz, FM-Feedback Tabelle
            //  Springt zufällig → körnig, instabil
        
            float gran2Out = 0.0f;
            if (gran2Mix > 0.001f)
            {
                // Schnelle Kornrate (8–20 Hz)
                float g2Rate = 8.0f + chaos2 * 12.0f;
                gran2Phase += g2Rate / static_cast<float>(sr);
                if (gran2Phase >= 1.0f)
                {
                    gran2Phase -= 1.0f;
                    // Zufälliger Sprung → glitchy
                    gran2Pos = juce::Random::getSystemRandom().nextFloat();
                }
                // Hanning Envelope — kurz und knackig
                float ge2 = 0.5f - 0.5f * std::cos (gran2Phase * juce::MathConstants<float>::twoPi);
                ge2 = std::pow (ge2, 2.0f);

                float lp2 = gran2Pos + gran2Phase * 0.04f;
                lp2 -= std::floor (lp2);
                gran2Out = waveEngine.getSample (WaveEngine::WaveType::FMFeedback, lp2) * ge2;
            }

            // Granular zum Hauptsignal addieren (capped)
            osc = juce::jlimit (-1.0f, 1.0f,
                                osc + gran1Out * gran1Mix * 1.8f + gran2Out * gran2Mix);
            
            float detune = 0.002f * std::sin(gran1Phase * 20.0f);
            float lp1 = gran1Pos + gran1Phase * (0.2f + detune);
            lp1 -= std::floor(lp1);
           

            // ── Drive ─────────────────────────────────────────────
            if (drive > 0.01f)
            {
                float dg = 1.0f + drive * 14.0f;
                float dd = std::tanh (dg);
                if (dd < 1e-6f) dd = 1e-6f;
                osc = std::tanh (osc * dg) / dd;
            }

            // ── LFO ───────────────────────────────────────────────
            lfoPhase += lfoRate / sr;
            if (lfoPhase >= 1.0) lfoPhase -= 1.0;
            float lfo = static_cast<float>(
                std::sin (lfoPhase * juce::MathConstants<double>::twoPi));

            // ── Filter ────────────────────────────────────────────
            float fEnvVal = filterEnv.getNextSample();
            float fc = juce::jlimit (30.0f, 17000.0f,
                cutoff
                + fEnvVal * envAmt * (6000.0f + accent * 4000.0f)
                + lfo * lfoDepth * 3000.0f);
            filter.setParameters (fc, resonance);
            float sig = filter.process (osc);

            // ── Fraktale Verzerrung ─────────────────────
            if (chaos2 > 0.01f)
            {
                float x = sig;
                for (int k = 0; k < 2; ++k)
                    x = std::sin (x * (2.0f + chaos2 * 8.0f));
                sig = sig * (1.0f - chaos2 * 0.7f) + x * (chaos2 * 0.7f);
            }

            sig = std::tanh (sig * (1.0f + resonance * 1.5f));

            // ── Time Folding ──────────────────────────────────────
            timeFoldBuf[timeFoldPos % TIME_FOLD_SIZE] = sig;
            if (timeFold > 0.01f)
            {
                float tfPhase = static_cast<float>(timeFoldPos % TIME_FOLD_SIZE)
                                / static_cast<float>(TIME_FOLD_SIZE);
                float foldedPhase = tfPhase + timeFold * 0.3f *
                    std::sin (tfPhase * juce::MathConstants<float>::twoPi * 2.0f);
                foldedPhase -= std::floor (foldedPhase);
                int readPos = static_cast<int>(foldedPhase * TIME_FOLD_SIZE) % TIME_FOLD_SIZE;
                int revPos  = (TIME_FOLD_SIZE - readPos - 1) % TIME_FOLD_SIZE;
                float tfMix = timeFold * 0.5f;
                sig = sig * (1.0f - tfMix)
                    + timeFoldBuf[readPos] * (tfMix * 0.6f)
                    + timeFoldBuf[revPos]  * (tfMix * 0.4f);
                sig = std::tanh (sig * 1.2f) / 1.2f;
            }
            ++timeFoldPos;
            if (timeFoldPos >= TIME_FOLD_SIZE * 16) timeFoldPos = 0;

            // ── Amplitude - ADSR ────────────────────────────────────
            float ampVal = ampEnv.getNextSample();
            sig *= ampVal * (0.3f + accent * 0.5f);

            // ── Chaos Chain ───────────────────────────────────────
            if (chaosAmt > 0.01f)
            {
                flangerPhase += 0.25 / sr;
                if (flangerPhase >= 1.0) flangerPhase -= 1.0;
                float fm = static_cast<float>(
                    std::sin (flangerPhase * juce::MathConstants<double>::twoPi));
                int dIdx = (delayWrite + 300 + static_cast<int>(fm * 180.0f)) & (DELAY_SIZE - 1);
                float chaosMix = chaosAmt * 0.55f;
                sig += delayLine[dIdx] * chaosAmt * 0.7f;
                sig += chaosAmt * 0.10f * std::tanh (sig * shimSin * 1.5f);

                float rv0    = sig * 0.45f + reverbBuf[0] * 0.46f;
                reverbBuf[0] = sig * 0.45f + rv0 * 0.44f;
                float rv1    = rv0 * 0.4f + reverbBuf[1] * 0.40f;
                reverbBuf[1] = rv0 * 0.4f + rv1 * 0.38f;
                rv1 = juce::jlimit (-1.0f, 1.0f, rv1);
                sig += rv1 * chaosAmt * 0.15f;
                sig = std::tanh (sig);
            }

            delayLine[delayWrite] = sig;
            delayWrite = (delayWrite + 1) & (DELAY_SIZE - 1);

            // ── Stutter ───────────────────────────────────────────
            if (stutterAmt > 0.01f)
            {
                int div = (int)*tree.getRawParameterValue("stutterDiv");

                float beatTime = 60.0f / bpm;

                // Mapping:
                float division = 0.25f; // default 1/4

                switch (div)
                {
                    case 0: division = 1.0f; break;    // 1/4
                    case 1: division = 0.5f; break;    // 1/8
                    case 2: division = 0.25f; break;   // 1/16 usw.
                    case 3: division = 0.125f; break;
                    case 4: division = 0.0625f; break;
                    case 5: division = 0.03125f; break;

                    // Triolen (×2/3)
                    case 6: division = 1.0f * (2.0f/3.0f); break;
                    case 7: division = 0.5f * (2.0f/3.0f); break;
                    case 8: division = 0.25f * (2.0f/3.0f); break;

                    // Quintolen (×4/5)
                    case 9: division = 1.0f * (4.0f/5.0f); break;
                    case 10: division = 0.5f * (4.0f/5.0f); break;

                    // Septolen (×4/7)
                    case 11: division = 1.0f * (4.0f/7.0f); break;
                    case 12: division = 0.5f * (4.0f/7.0f); break;
                }

                float stutterHz = 1.0f / (beatTime * division);
                stutterPhase += stutterHz / sr;
                
               
                if (stutterPhase >= 1.0) stutterPhase -= 1.0;
                float gate = (stutterPhase < (0.5f - stutterAmt * 0.38f)) ? 1.0f : 0.0f;
                gate *= (0.7f + 0.3f * static_cast<float>(std::sin(stutterPhase * 50.0)));
                smoothedGate += (gate - smoothedGate) * 0.002f;
                sig *= smoothedGate;
            }

            // Hard limiter
            sig = juce::jlimit (-1.0f, 1.0f, sig);

            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
                outputBuffer.addSample (ch, i, sig);
        }

        if (!ampEnv.isActive())
            clearCurrentNote();
    }

    void prepareVoice (double sampleRate)
    {
        filter.setSampleRate (sampleRate);

        juce::ADSR::Parameters ap;
        ap.attack = 0.005f; ap.decay = 0.3f; ap.sustain = 0.7f; ap.release = 0.5f;
        ampEnv.setSampleRate (sampleRate);
        ampEnv.setParameters (ap);

        juce::ADSR::Parameters fp;
        fp.attack = 0.002f; fp.decay = 0.2f; fp.sustain = 0.0f; fp.release = 0.1f;
        filterEnv.setSampleRate (sampleRate);
        filterEnv.setParameters (fp);

        std::fill (std::begin(delayLine), std::end(delayLine), 0.0f);
        timeFoldBuf.fill (0.0f);
        delayWrite = timeFoldPos = 0;
        reverbBuf[0] = reverbBuf[1] = 0.0f;
        pdFeedback = 0.0f;
    }

    void updateADSR (float a, float d, float s, float r)
    {
        juce::ADSR::Parameters p;
        p.attack = a; p.decay = d; p.sustain = s; p.release = r;
        ampEnv.setParameters (p);
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

private:
    const WaveEngine&                   waveEngine;
    juce::AudioProcessorValueTreeState& tree;

    double frequency = 440.0, currentFreq = 0.0, targetFreq = 440.0;
    double phase = 0.0, lfoPhase = 0.0, stutterPhase = 0.0;
    double flangerPhase = 0.0, shimmerPhase = 0.0;
    double morphAutoPhase = 0.0, phaseDestPhase = 0.0;

    float velocity = 1.0f, smoothedGate = 0.0f, pdFeedback = 0.0f;
    float bpm = 120.0f;

    // Haupt-Granular
    float grainPhase = 0.0f, grainPos = 0.0f;

    // Granular 1 (langsam)
    float gran1Phase = 0.0f, gran1Pos = 0.0f, gran1Env = 0.0f;

    // Granular 2 (schnell/ etwas glitchy)
    float gran2Phase = 0.0f, gran2Pos = 0.0f, gran2Env = 0.0f;

    juce::ADSR   ampEnv, filterEnv;
    LadderFilter filter;

    static constexpr int DELAY_SIZE    = 4096;
    static constexpr int TIME_FOLD_SIZE = 512;

    float delayLine[DELAY_SIZE] = {};
    int   delayWrite = 0;

    std::array<float, TIME_FOLD_SIZE> timeFoldBuf {};
    int   timeFoldPos = 0;

    float reverbBuf[2] = {};
};
