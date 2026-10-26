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
    class ModeStrip;    // click-through mode selector (no popup)
    class SlotButton;   // A/B comparison slot

    DAroomAudioProcessor& audioProcessor;
    juce::Image background;
    juce::Slider mix, predelay, decay, size;
    juce::ComboBox presetSelector;
    juce::Label mixValue, predelayValue, decayValue, sizeValue;
    juce::Array<PresetDef> factoryPresets;
    std::unique_ptr<ModeStrip> modeStrip;
    std::unique_ptr<SlotButton> slotA, slotB;
    std::unique_ptr<juce::ComponentBoundsConstrainer> aspectConstrainer;
    float uiScale = 1.0f;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment, predelayAttachment, decayAttachment, sizeAttachment;

    static juce::Array<PresetDef> loadFactoryPresets();
    void recallPreset (int presetIndex);          // 1..50
    void markManual();
    void stepPreset (int delta);
    void syncPresetCombo();
    void repaintKnobs();
    void styleSlider (juce::Slider&, const juce::String& name);
    void styleValue (juce::Label&);
    juce::Rectangle<int> scaled (juce::Rectangle<int> base) const;
    void drawKnob (juce::Graphics&, juce::Point<float> centre, float radius, float proportion);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DAroomAudioProcessorEditor)
};
