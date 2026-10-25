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
    void mouseDown (const juce::MouseEvent&) override;

    /** One factory preset as stored in the .drpreset JSON files. */
    struct PresetDef
    {
        juce::String name;
        float mix = 0.0f, predelayMs = 0.0f, decaySeconds = 1.0f, sizePercent = 50.0f;
        int mode = 0;
    };

private:
    DAroomAudioProcessor& audioProcessor;
    juce::Image background;
    juce::Slider mix, predelay, decay, size;
    juce::ComboBox mode, presetSelector;
    juce::Label mixValue, predelayValue, decayValue, sizeValue;
    juce::Array<PresetDef> factoryPresets;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment, predelayAttachment, decayAttachment, sizeAttachment;
    std::unique_ptr<ComboAttachment> modeAttachment;

    static juce::Array<PresetDef> loadFactoryPresets();
    void recallPreset (int presetIndex);          // 1..36
    void markManual();
    void stepPreset (int delta);
    void styleSlider (juce::Slider&, const juce::String& name);
    void styleValue (juce::Label&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DAroomAudioProcessorEditor)
};
