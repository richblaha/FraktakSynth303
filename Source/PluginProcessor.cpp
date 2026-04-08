#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout
FraktalSynth303AudioProcessor::createParameters()
{
    using F = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<C> ("wave", "Waveform",
        juce::StringArray { "Lorenz", "FM Feedback", "Granular", "Saw", "Square" }, 3));

    layout.add (std::make_unique<F> ("cutoff", "Cutoff",
        juce::NormalisableRange<float> (30.0f, 17000.0f, 1.0f, 0.3f), 900.0f));
    layout.add (std::make_unique<F> ("resonance",    "Resonance",  0.0f,  0.85f, 0.25f));
    layout.add (std::make_unique<F> ("drive",        "Drive",      0.0f,  1.0f,  0.0f));
    layout.add (std::make_unique<F> ("filterEnvAmt", "Filter Env", 0.0f,  1.0f,  0.5f));

    layout.add (std::make_unique<F> ("lfoRate", "LFO Rate",
        juce::NormalisableRange<float> (0.05f, 12.0f, 0.01f, 0.5f), 0.5f));
    layout.add (std::make_unique<F> ("lfoDepth", "LFO Depth", 0.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<F> ("attack",  "Attack",  0.001f, 3.0f, 0.01f));
    layout.add (std::make_unique<F> ("decay",   "Decay",   0.01f,  3.0f, 0.3f));
    layout.add (std::make_unique<F> ("sustain", "Sustain", 0.0f,   1.0f, 0.7f));
    layout.add (std::make_unique<F> ("release", "Release", 0.01f,  4.0f, 0.5f));

    layout.add (std::make_unique<F> ("chaos",   "Chaos",   0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<F> ("chaos2",  "Fractal", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<F> ("stutter", "Stutter", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<F> ("pwm",     "PWM",     0.05f, 0.95f, 0.5f));

    layout.add (std::make_unique<C> ("stutterMode", "Stutter Mode",
        juce::StringArray { "Free", "Sync" }, 0));
    layout.add (std::make_unique<C> ("stutterDiv", "Stutter Division",
        juce::StringArray {  "1/4","1/8","1/16","1/32","1/64","1/128",
        "1/4T","1/8T","1/16T",
        "1/4Q","1/8Q",
        "1/4S","1/8S"
    }, 2));

    layout.add (std::make_unique<F> ("morph",     "Morph",
        juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<F> ("morphAuto", "Morph Auto", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<F> ("phaseDest", "Phase Dest", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<F> ("timeFold",  "Time Fold",  0.0f, 1.0f, 0.0f));

    // ── Granular-Reinmischer ───────────────────────────────
    // gran1: langsam driftender Granular-Osc (weich, texturell)
    // gran2: schnell getriggerter Granular-Osc (glitchy, körner)

    layout.add (std::make_unique<F> ("gran1", "Granular 1", 0.0f, 0.6f, 0.0f));
    layout.add (std::make_unique<F> ("gran2", "Granular 2", 0.0f, 0.6f, 0.0f));

    layout.add (std::make_unique<F> ("volume", "Volume", 0.0f, 1.0f, 0.8f));

    return layout;
}

FraktalSynth303AudioProcessor::FraktalSynth303AudioProcessor()
    : AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameters())
{
    synth.addSound (new SynthSound());
    for (int i = 0; i < 4; ++i)
        synth.addVoice (new SynthVoice (waveEngine, apvts));
}

FraktalSynth303AudioProcessor::~FraktalSynth303AudioProcessor() {}

void FraktalSynth303AudioProcessor::prepareToPlay (double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            v->prepareVoice (sampleRate);
}

void FraktalSynth303AudioProcessor::releaseResources() {}

void FraktalSynth303AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // BPM aus DAW holen
    float bpm = 120.0f;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (pos->getBpm().hasValue())
                bpm = static_cast<float>(*pos->getBpm());

    float a = *apvts.getRawParameterValue ("attack");
    float d = *apvts.getRawParameterValue ("decay");
    float s = *apvts.getRawParameterValue ("sustain");
    float r = *apvts.getRawParameterValue ("release");

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
        {
            v->updateADSR (a, d, s, r);
            v->setBPM (bpm);
        }

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    buffer.applyGain (*apvts.getRawParameterValue ("volume"));
}

void FraktalSynth303AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FraktalSynth303AudioProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FraktalSynth303AudioProcessor();
}
