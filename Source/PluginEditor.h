#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class DAroomAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit DAroomAudioProcessorEditor (DAroomAudioProcessor&);
    ~DAroomAudioProcessorEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DAroomAudioProcessor& audioProcessor;
    juce::Image background;
    juce::Slider mix, predelay, decay, size;
    juce::ComboBox mode, presetSelector;
    juce::Label mixValue, predelayValue, decayValue, sizeValue;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment, predelayAttachment, decayAttachment, sizeAttachment;
    std::unique_ptr<ComboAttachment> modeAttachment;

    void styleSlider (juce::Slider&, const juce::String& name);
    void styleValue (juce::Label&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DAroomAudioProcessorEditor)
};
