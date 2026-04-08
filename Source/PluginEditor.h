#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class FraktalSynth303AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    FraktalSynth303AudioProcessorEditor (FraktalSynth303AudioProcessor&);
    ~FraktalSynth303AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FraktalSynth303AudioProcessor& processor;

    const juce::Colour C_BG     { 0xFF080808 };
    const juce::Colour C_ACCENT { 0xFFC8FF00 };
    const juce::Colour C_DIM    { 0xFF2A2A2A };
    const juce::Colour C_MUTED  { 0xFF555555 };

    juce::ComboBox waveSelector;
    juce::ComboBox stutterModeBox;
    juce::ComboBox stutterDivBox;

    juce::Slider cutoffKnob, resonanceKnob, driveKnob, filterEnvKnob;
    juce::Slider attackKnob, decayKnob, sustainKnob, releaseKnob;
    juce::Slider lfoRateKnob, lfoDepthKnob;
    juce::Slider chaosKnob, chaos2Knob, stutterKnob, pwmKnob, volumeKnob;
    juce::Slider morphKnob, morphAutoKnob;
    juce::Slider phaseDestKnob, timeFoldKnob;

 
    juce::Slider gran1Knob, gran2Knob;

    juce::Label waveLabel;
    juce::Label cutoffLabel, resonanceLabel, driveLabel, filterEnvLabel;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;
    juce::Label lfoRateLabel, lfoDepthLabel;
    juce::Label chaosLabel, chaos2Label, stutterLabel, pwmLabel, volumeLabel;
    juce::Label morphLabel, morphAutoLabel, phaseDestLabel, timeFoldLabel;
    juce::Label gran1Label, gran2Label;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboAttach>  waveAttach;
    std::unique_ptr<ComboAttach>  stutterModeAttach;
    std::unique_ptr<ComboAttach>  stutterDivAttach;
    std::unique_ptr<SliderAttach> cutoffAttach, resonanceAttach, driveAttach, filterEnvAttach;
    std::unique_ptr<SliderAttach> attackAttach, decayAttach, sustainAttach, releaseAttach;
    std::unique_ptr<SliderAttach> lfoRateAttach, lfoDepthAttach;
    std::unique_ptr<SliderAttach> chaosAttach, chaos2Attach, stutterAttach, pwmAttach, volumeAttach;
    std::unique_ptr<SliderAttach> morphAttach, morphAutoAttach, phaseDestAttach, timeFoldAttach;
    std::unique_ptr<SliderAttach> gran1Attach, gran2Attach;

    void styleKnob (juce::Slider&, juce::Label&, const juce::String&, juce::Colour);
    void drawSection (juce::Graphics&, const juce::String&, juce::Rectangle<int>, juce::Colour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FraktalSynth303AudioProcessorEditor)
};
