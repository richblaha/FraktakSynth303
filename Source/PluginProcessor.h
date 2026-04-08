#pragma once
#include <JuceHeader.h>
#include "WaveEngine.h"
#include "SynthVoice.h"
#include "SynthSound.h"

class FraktalSynth303AudioProcessor : public juce::AudioProcessor
{
public:
    FraktalSynth303AudioProcessor();
    ~FraktalSynth303AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FraktalSynth303"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    juce::Synthesiser synth;
    WaveEngine waveEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FraktalSynth303AudioProcessor)
};
