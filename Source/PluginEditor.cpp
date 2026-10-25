#include "PluginEditor.h"
#include "BinaryData.h"

DAroomAudioProcessorEditor::DAroomAudioProcessorEditor (DAroomAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    background = juce::ImageFileFormat::loadFrom(BinaryData::DAroom_UI_Reference_png,
                                                  BinaryData::DAroom_UI_Reference_pngSize);
    setSize (653, 184);
    setResizable (false, false);

    styleSlider(mix, "MIX");
    styleSlider(predelay, "PREDELAY");
    styleSlider(decay, "DECAY");
    styleSlider(size, "SIZE");

    // Transparent rotary hit areas sit over the knobs in the supplied artwork.
    // The artwork remains pixel-identical to the reference at 653 x 185.
    addAndMakeVisible(mix);
    addAndMakeVisible(predelay);
    addAndMakeVisible(decay);
    addAndMakeVisible(size);

    for (auto* label : { &mixValue, &predelayValue, &decayValue, &sizeValue })
    {
        styleValue(*label);
        addAndMakeVisible(*label);
    }

    // Preset browser overlays the preset strip in the reference artwork.
    const juce::StringArray presetNames {
        "Small Studio", "Vocal Booth", "Living Room", "Wood Room", "Drum Room", "Bright Room",
        "Tight Booth", "Tracking Room", "Small Hall", "Concert Hall", "Dark Hall", "Bright Hall",
        "Cathedral", "Arena", "Vintage Plate", "Vocal Plate", "Bright Plate", "Drum Plate",
        "Deep Chamber", "Stone Chamber", "Warm Chamber", "Dark Chamber", "Gold Chamber",
        "Ambient Cloud", "Dream Space", "Cosmic Drift", "Frozen Air", "Soft Focus", "Wide Synth",
        "Cinematic Impact", "Piano Bloom", "Guitar Springless", "Snare Bloom", "Orchestral Depth",
        "Whisper Room", "Endless Horizon"
    };
    for (int i = 0; i < presetNames.size(); ++i)
        presetSelector.addItem(presetNames[i], i + 1);
    presetSelector.setSelectedId(19, juce::dontSendNotification); // Deep Chamber
    presetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xee071018));
    presetSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xffdbe6ec));
    presetSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0x00ffffff));
    presetSelector.onChange = [this]
    {
        const int id = presetSelector.getSelectedId();
        switch (id)
        {
            case 1: audioProcessor.applyPreset(18, 8, 0.65f, 38, 0); break;
            case 2: audioProcessor.applyPreset(12, 4, 0.45f, 24, 0); break;
            case 3: audioProcessor.applyPreset(24, 12, 1.10f, 48, 0); break;
            case 4: audioProcessor.applyPreset(22, 10, 0.90f, 42, 0); break;
            case 5: audioProcessor.applyPreset(28, 6, 0.85f, 58, 0); break;
            case 6: audioProcessor.applyPreset(20, 9, 1.25f, 52, 0); break;
            case 7: audioProcessor.applyPreset(10, 2, 0.30f, 18, 0); break;
            case 8: audioProcessor.applyPreset(22, 14, 1.40f, 55, 0); break;
            case 9: audioProcessor.applyPreset(24, 18, 1.90f, 58, 1); break;
            case 10: audioProcessor.applyPreset(28, 22, 3.20f, 74, 1); break;
            case 11: audioProcessor.applyPreset(24, 12, 3.80f, 70, 1); break;
            case 12: audioProcessor.applyPreset(22, 18, 2.70f, 68, 1); break;
            case 13: audioProcessor.applyPreset(22, 28, 6.80f, 84, 1); break;
            case 14: audioProcessor.applyPreset(26, 32, 5.40f, 92, 1); break;
            case 15: audioProcessor.applyPreset(30, 10, 2.20f, 60, 2); break;
            case 16: audioProcessor.applyPreset(24, 18, 1.80f, 55, 2); break;
            case 17: audioProcessor.applyPreset(26, 8, 2.60f, 62, 2); break;
            case 18: audioProcessor.applyPreset(18, 4, 1.35f, 50, 2); break;
            case 19: audioProcessor.applyPreset(26, 25, 4.50f, 78, 3); break;
            case 20: audioProcessor.applyPreset(24, 20, 3.60f, 72, 3); break;
            case 21: audioProcessor.applyPreset(30, 20, 2.40f, 68, 3); break;
            case 22: audioProcessor.applyPreset(20, 16, 3.20f, 64, 3); break;
            case 23: audioProcessor.applyPreset(28, 14, 2.80f, 70, 3); break;
            case 24: audioProcessor.applyPreset(32, 35, 8.50f, 95, 4); break;
            case 25: audioProcessor.applyPreset(35, 30, 7.20f, 90, 4); break;
            case 26: audioProcessor.applyPreset(28, 42, 10.0f, 100, 4); break;
            case 27: audioProcessor.applyPreset(22, 28, 5.80f, 86, 4); break;
            case 28: audioProcessor.applyPreset(18, 24, 4.20f, 78, 4); break;
            case 29: audioProcessor.applyPreset(30, 18, 5.20f, 88, 4); break;
            case 30: audioProcessor.applyPreset(20, 12, 4.80f, 90, 3); break;
            case 31: audioProcessor.applyPreset(24, 16, 3.10f, 66, 1); break;
            case 32: audioProcessor.applyPreset(18, 10, 1.60f, 48, 2); break;
            case 33: audioProcessor.applyPreset(22, 5, 1.75f, 54, 2); break;
            case 34: audioProcessor.applyPreset(25, 24, 4.60f, 82, 1); break;
            case 35: audioProcessor.applyPreset(12, 18, 1.30f, 44, 0); break;
            case 36: audioProcessor.applyPreset(30, 50, 12.0f, 100, 4); break;
            default: break;
        }
    };
    addAndMakeVisible(presetSelector);

    mode.addItem("ROOM", 1);
    mode.addItem("HALL", 2);
    mode.addItem("PLATE", 3);
    mode.addItem("CHAMBER", 4);
    mode.addItem("AMBIENCE", 5);
    mode.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xEE071018));
    mode.setColour(juce::ComboBox::textColourId, juce::Colour(0xffdbe6ec));
    mode.setColour(juce::ComboBox::outlineColourId, juce::Colour(0x00ffffff));
    addAndMakeVisible(mode);

    auto& state = audioProcessor.parameters;
    mixAttachment = std::make_unique<SliderAttachment>(state, "mix", mix);
    predelayAttachment = std::make_unique<SliderAttachment>(state, "predelay", predelay);
    decayAttachment = std::make_unique<SliderAttachment>(state, "decay", decay);
    sizeAttachment = std::make_unique<SliderAttachment>(state, "size", size);
    modeAttachment = std::make_unique<ComboAttachment>(state, "mode", mode);

    auto updateValues = [this]
    {
        mixValue.setText(juce::String(mix.getValue(), 0) + "%", juce::dontSendNotification);
        predelayValue.setText(juce::String(predelay.getValue(), 0) + " ms", juce::dontSendNotification);
        decayValue.setText(juce::String(decay.getValue(), 2) + " s", juce::dontSendNotification);
        sizeValue.setText(juce::String(size.getValue(), 0) + "%", juce::dontSendNotification);
    };
    mix.onValueChange = updateValues;
    predelay.onValueChange = updateValues;
    decay.onValueChange = updateValues;
    size.onValueChange = updateValues;
    updateValues();
}

void DAroomAudioProcessorEditor::styleSlider (juce::Slider& s, const juce::String& name)
{
    s.setName(name);
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0x00ffffff));
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0x00ffffff));
    s.setColour(juce::Slider::thumbColourId, juce::Colour(0x00ffffff));
    s.setAlpha(0.015f); // Invisible interaction layer over the artwork.
}

void DAroomAudioProcessorEditor::styleValue (juce::Label& l)
{
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, juce::Colour(0xffd9e4ea));
    l.setFont(juce::Font(10.0f));
    l.setInterceptsMouseClicks(false, false);
}

void DAroomAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
        g.drawImage(background, getLocalBounds().toFloat());
    else
        g.fillAll(juce::Colour(0xff071018));
}

void DAroomAudioProcessorEditor::resized()
{
    // Hit regions align to the four rotary controls in the exact supplied image.
    mix.setBounds(17, 72, 50, 47);
    predelay.setBounds(100, 72, 52, 47);
    decay.setBounds(497, 70, 54, 49);
    size.setBounds(579, 70, 54, 49);

    // Value overlays replace the baked-in static values, so the numbers follow controls.
    mixValue.setBounds(23, 138, 38, 13);
    predelayValue.setBounds(99, 138, 54, 13);
    decayValue.setBounds(495, 138, 58, 13);
    sizeValue.setBounds(585, 138, 43, 13);

    // The mode selector overlays the mode strip from the artwork.
    mode.setBounds(168, 155, 324, 22);
    presetSelector.setBounds(227, 12, 199, 24);
}
