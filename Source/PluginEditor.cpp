#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    // Artwork geometry (653 x 184 reference image).
    const juce::StringArray modeNames { "ROOM", "HALL", "PLATE", "CHAMBER", "AMBIENCE" };
    const float modeLabelCentres[] = { 183.0f, 244.0f, 303.0f, 372.0f, 451.0f };

    const juce::Rectangle<int> mixReadout      { 23, 137, 38, 14 };
    const juce::Rectangle<int> predelayReadout { 99, 137, 54, 14 };
    const juce::Rectangle<int> decayReadout    { 495, 137, 58, 14 };
    const juce::Rectangle<int> sizeReadout     { 585, 137, 43, 14 };
    const juce::Rectangle<int> presetNameArea  { 250, 15, 150, 20 };
    const juce::Rectangle<int> modeStripArea   { 168, 160, 322, 18 };
    const juce::Rectangle<int> leftArrowArea   { 228, 14, 22, 22 };
    const juce::Rectangle<int> rightArrowArea  { 401, 14, 22, 22 };

    // Colours sampled from the reference artwork so the erased regions blend in.
    const juce::Colour mixEraseColour      { 0xff030e13 };
    const juce::Colour predelayEraseColour { 0xff040f14 };
    const juce::Colour decayEraseColour    { 0xff061117 };
    const juce::Colour sizeEraseColour     { 0xff030c11 };
    const juce::Colour presetEraseColour   { 0xff0b1a21 };
    const juce::Colour stripTopColour      { 0xff071218 };
    const juce::Colour stripBottomColour   { 0xff050f15 };
    const juce::Colour modeIdleColour      { 0xff8b9aa2 };
    const juce::Colour modeActiveColour    { 0xffeef4f7 };
}

juce::Array<DAroomAudioProcessorEditor::PresetDef> DAroomAudioProcessorEditor::loadFactoryPresets()
{
    struct Resource { const char* file; const char* data; int size; };

    // The .drpreset files shipped in Presets/ are embedded as binary data, so
    // the plug-in and the on-disk presets can never drift apart.
    static const Resource resources[] =
    {
    { "small-studio.drpreset", BinaryData::smallstudio_drpreset, BinaryData::smallstudio_drpresetSize },  // 1: Small Studio
    { "vocal-booth.drpreset", BinaryData::vocalbooth_drpreset, BinaryData::vocalbooth_drpresetSize },  // 2: Vocal Booth
    { "living-room.drpreset", BinaryData::livingroom_drpreset, BinaryData::livingroom_drpresetSize },  // 3: Living Room
    { "wood-room.drpreset", BinaryData::woodroom_drpreset, BinaryData::woodroom_drpresetSize },  // 4: Wood Room
    { "drum-room.drpreset", BinaryData::drumroom_drpreset, BinaryData::drumroom_drpresetSize },  // 5: Drum Room
    { "bright-room.drpreset", BinaryData::brightroom_drpreset, BinaryData::brightroom_drpresetSize },  // 6: Bright Room
    { "tight-booth.drpreset", BinaryData::tightbooth_drpreset, BinaryData::tightbooth_drpresetSize },  // 7: Tight Booth
    { "tracking-room.drpreset", BinaryData::trackingroom_drpreset, BinaryData::trackingroom_drpresetSize },  // 8: Tracking Room
    { "small-hall.drpreset", BinaryData::smallhall_drpreset, BinaryData::smallhall_drpresetSize },  // 9: Small Hall
    { "concert-hall.drpreset", BinaryData::concerthall_drpreset, BinaryData::concerthall_drpresetSize },  // 10: Concert Hall
    { "dark-hall.drpreset", BinaryData::darkhall_drpreset, BinaryData::darkhall_drpresetSize },  // 11: Dark Hall
    { "bright-hall.drpreset", BinaryData::brighthall_drpreset, BinaryData::brighthall_drpresetSize },  // 12: Bright Hall
    { "cathedral.drpreset", BinaryData::cathedral_drpreset, BinaryData::cathedral_drpresetSize },  // 13: Cathedral
    { "arena.drpreset", BinaryData::arena_drpreset, BinaryData::arena_drpresetSize },  // 14: Arena
    { "vintage-plate.drpreset", BinaryData::vintageplate_drpreset, BinaryData::vintageplate_drpresetSize },  // 15: Vintage Plate
    { "vocal-plate.drpreset", BinaryData::vocalplate_drpreset, BinaryData::vocalplate_drpresetSize },  // 16: Vocal Plate
    { "bright-plate.drpreset", BinaryData::brightplate_drpreset, BinaryData::brightplate_drpresetSize },  // 17: Bright Plate
    { "drum-plate.drpreset", BinaryData::drumplate_drpreset, BinaryData::drumplate_drpresetSize },  // 18: Drum Plate
    { "deep-chamber.drpreset", BinaryData::deepchamber_drpreset, BinaryData::deepchamber_drpresetSize },  // 19: Deep Chamber
    { "stone-chamber.drpreset", BinaryData::stonechamber_drpreset, BinaryData::stonechamber_drpresetSize },  // 20: Stone Chamber
    { "warm-chamber.drpreset", BinaryData::warmchamber_drpreset, BinaryData::warmchamber_drpresetSize },  // 21: Warm Chamber
    { "dark-chamber.drpreset", BinaryData::darkchamber_drpreset, BinaryData::darkchamber_drpresetSize },  // 22: Dark Chamber
    { "gold-chamber.drpreset", BinaryData::goldchamber_drpreset, BinaryData::goldchamber_drpresetSize },  // 23: Gold Chamber
    { "ambient-cloud.drpreset", BinaryData::ambientcloud_drpreset, BinaryData::ambientcloud_drpresetSize },  // 24: Ambient Cloud
    { "dream-space.drpreset", BinaryData::dreamspace_drpreset, BinaryData::dreamspace_drpresetSize },  // 25: Dream Space
    { "cosmic-drift.drpreset", BinaryData::cosmicdrift_drpreset, BinaryData::cosmicdrift_drpresetSize },  // 26: Cosmic Drift
    { "frozen-air.drpreset", BinaryData::frozenair_drpreset, BinaryData::frozenair_drpresetSize },  // 27: Frozen Air
    { "soft-focus.drpreset", BinaryData::softfocus_drpreset, BinaryData::softfocus_drpresetSize },  // 28: Soft Focus
    { "wide-synth.drpreset", BinaryData::widesynth_drpreset, BinaryData::widesynth_drpresetSize },  // 29: Wide Synth
    { "cinematic-impact.drpreset", BinaryData::cinematicimpact_drpreset, BinaryData::cinematicimpact_drpresetSize },  // 30: Cinematic Impact
    { "piano-bloom.drpreset", BinaryData::pianobloom_drpreset, BinaryData::pianobloom_drpresetSize },  // 31: Piano Bloom
    { "guitar-springless.drpreset", BinaryData::guitarspringless_drpreset, BinaryData::guitarspringless_drpresetSize },  // 32: Guitar Springless
    { "snare-bloom.drpreset", BinaryData::snarebloom_drpreset, BinaryData::snarebloom_drpresetSize },  // 33: Snare Bloom
    { "orchestral-depth.drpreset", BinaryData::orchestraldepth_drpreset, BinaryData::orchestraldepth_drpresetSize },  // 34: Orchestral Depth
    { "whisper-room.drpreset", BinaryData::whisperroom_drpreset, BinaryData::whisperroom_drpresetSize },  // 35: Whisper Room
    { "endless-horizon.drpreset", BinaryData::endlesshorizon_drpreset, BinaryData::endlesshorizon_drpresetSize },  // 36: Endless Horizon
    };

    const juce::StringArray modeStrings { "Room", "Hall", "Plate", "Chamber", "Ambience" };

    juce::Array<PresetDef> result;

    for (const auto& r : resources)
    {
        const auto parsed = juce::JSON::parse (juce::String::fromUTF8 (r.data, r.size));
        const auto params = parsed["parameters"];

        PresetDef d;
        d.name = parsed["name"].toString();
        if (d.name.isEmpty())
            d.name = r.file;
        d.mix          = (float) params["mix"];
        d.predelayMs   = (float) params["predelay_ms"];
        d.decaySeconds = (float) params["decay_seconds"];
        d.sizePercent  = (float) params["size_percent"];
        d.mode         = juce::jlimit (0, modeStrings.size() - 1, modeStrings.indexOf (params["mode"].toString(), false));
        result.add (d);
    }

    return result;
}

DAroomAudioProcessorEditor::DAroomAudioProcessorEditor (DAroomAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    background = juce::ImageFileFormat::loadFrom (BinaryData::DAroom_UI_Reference_png,
                                                  BinaryData::DAroom_UI_Reference_pngSize);
    factoryPresets = loadFactoryPresets();

    setSize (653, 184);
    setResizable (false, false);

    styleSlider (mix, "MIX");
    styleSlider (predelay, "PREDELAY");
    styleSlider (decay, "DECAY");
    styleSlider (size, "SIZE");

    // Transparent rotary hit areas sit over the knobs in the supplied artwork.
    addAndMakeVisible (mix);
    addAndMakeVisible (predelay);
    addAndMakeVisible (decay);
    addAndMakeVisible (size);

    for (auto* label : { &mixValue, &predelayValue, &decayValue, &sizeValue })
    {
        styleValue (*label);
        addAndMakeVisible (*label);
    }

    // Preset browser: id 1 = "Manual", ids 2..37 = factory presets, so the
    // selection can be persisted as (id - 1) inside the plug-in state.
    presetSelector.addItem ("Manual", 1);
    for (int i = 0; i < factoryPresets.size(); ++i)
        presetSelector.addItem (factoryPresets[i].name, i + 2);

    presetSelector.setJustificationType (juce::Justification::centred);
    presetSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0x00ffffff));
    presetSelector.setColour (juce::ComboBox::textColourId, juce::Colour (0xffd5dde1));
    presetSelector.setColour (juce::ComboBox::outlineColourId, juce::Colour (0x00ffffff));
    presetSelector.setColour (juce::ComboBox::arrowColourId, juce::Colour (0x00ffffff));
    presetSelector.onChange = [this]
    {
        const int idx = presetSelector.getSelectedId() - 1;
        if (idx >= 1)
            recallPreset (idx);
        else
            audioProcessor.setPresetIndex (0);
    };
    addAndMakeVisible (presetSelector);

    for (int i = 0; i < modeNames.size(); ++i)
        mode.addItem (modeNames[i], i + 1);
    mode.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0x00ffffff));
    mode.setColour (juce::ComboBox::textColourId, juce::Colour (0x00ffffff)); // strip is redrawn in paint()
    mode.setColour (juce::ComboBox::outlineColourId, juce::Colour (0x00ffffff));
    mode.setColour (juce::ComboBox::arrowColourId, juce::Colour (0x00ffffff));
    mode.onChange = [this] { repaint (modeStripArea); };
    addAndMakeVisible (mode);

    auto& state = audioProcessor.parameters;
    mixAttachment      = std::make_unique<SliderAttachment> (state, "mix", mix);
    predelayAttachment = std::make_unique<SliderAttachment> (state, "predelay", predelay);
    decayAttachment    = std::make_unique<SliderAttachment> (state, "decay", decay);
    sizeAttachment     = std::make_unique<SliderAttachment> (state, "size", size);
    modeAttachment     = std::make_unique<ComboAttachment> (state, "mode", mode);

    // Restore the last recalled preset name (or the artwork default).
    int stored = audioProcessor.getPresetIndex();
    if (stored == DAroomAudioProcessor::unsetPresetIndex || stored < 0 || stored > factoryPresets.size())
    {
        stored = 1;
        for (int i = 0; i < factoryPresets.size(); ++i)
            if (factoryPresets[i].name == "Deep Chamber")
                stored = i + 1;
    }
    audioProcessor.setPresetIndex (stored);
    presetSelector.setSelectedId (stored + 1, juce::dontSendNotification);

    auto updateValues = [this]
    {
        mixValue.setText (juce::String (mix.getValue(), 0) + "%", juce::dontSendNotification);
        predelayValue.setText (juce::String (predelay.getValue(), 0) + " ms", juce::dontSendNotification);
        decayValue.setText (juce::String (decay.getValue(), 2) + " s", juce::dontSendNotification);
        sizeValue.setText (juce::String (size.getValue(), 0) + "%", juce::dontSendNotification);
    };
    const auto dragStarted = [this] { markManual(); };

    mix.onValueChange = updateValues;
    predelay.onValueChange = updateValues;
    decay.onValueChange = updateValues;
    size.onValueChange = updateValues;
    mix.onDragStart = dragStarted;
    predelay.onDragStart = dragStarted;
    decay.onDragStart = dragStarted;
    size.onDragStart = dragStarted;
    updateValues();
}

void DAroomAudioProcessorEditor::recallPreset (int presetIndex)
{
    if (! factoryPresets.isValidIndex (presetIndex - 1))
        return;

    const auto& d = factoryPresets.getReference (presetIndex - 1);
    audioProcessor.applyPreset (d.mix, d.predelayMs, d.decaySeconds, d.sizePercent, d.mode);
    audioProcessor.setPresetIndex (presetIndex);
}

void DAroomAudioProcessorEditor::markManual()
{
    audioProcessor.setPresetIndex (0);
    presetSelector.setSelectedId (1, juce::dontSendNotification);
}

void DAroomAudioProcessorEditor::stepPreset (int delta)
{
    const int count = factoryPresets.size();
    if (count == 0)
        return;

    int idx = audioProcessor.getPresetIndex();
    if (idx < 1 || idx > count)
        idx = presetSelector.getSelectedId() - 1;

    idx = ((idx - 1 + delta) % count + count) % count + 1;
    presetSelector.setSelectedId (idx + 1, juce::dontSendNotification);
    recallPreset (idx);
}

void DAroomAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    // The artwork's < > arrows step through the factory presets.
    if (leftArrowArea.contains (e.getPosition()))
    {
        stepPreset (-1);
        return;
    }
    if (rightArrowArea.contains (e.getPosition()))
    {
        stepPreset (1);
        return;
    }

    juce::AudioProcessorEditor::mouseDown (e);
}

void DAroomAudioProcessorEditor::styleSlider (juce::Slider& s, const juce::String& name)
{
    s.setName (name);
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0x00ffffff));
    s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0x00ffffff));
    s.setColour (juce::Slider::thumbColourId, juce::Colour (0x00ffffff));
    s.setAlpha (0.015f); // Invisible interaction layer over the artwork.
}

void DAroomAudioProcessorEditor::styleValue (juce::Label& l)
{
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, juce::Colour (0xffd9e4ea));
    l.setFont (juce::Font (juce::FontOptions (10.0f)));
    l.setInterceptsMouseClicks (false, false);
}

void DAroomAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());
    else
        g.fillAll (juce::Colour (0xff071018));

    // The artwork bakes static readouts ("26%", "25 ms", ...) into the image.
    // Erase them with sampled background colours so the live value labels do
    // not draw on top of stale numbers.
    g.setColour (mixEraseColour);      g.fillRect (mixReadout);
    g.setColour (predelayEraseColour); g.fillRect (predelayReadout);
    g.setColour (decayEraseColour);    g.fillRect (decayReadout);
    g.setColour (sizeEraseColour);     g.fillRect (sizeReadout);
    g.setColour (presetEraseColour);   g.fillRect (presetNameArea);

    // Redraw the mode strip with the *active* mode highlighted (the artwork
    // always shows CHAMBER selected).
    g.setGradientFill (juce::ColourGradient (stripTopColour,
                                             (float) modeStripArea.getX(), (float) modeStripArea.getY(),
                                             stripBottomColour,
                                             (float) modeStripArea.getX(), (float) modeStripArea.getBottom(),
                                             false));
    g.fillRect (modeStripArea);

    const int selected = juce::jlimit (0, modeNames.size() - 1, mode.getSelectedItemIndex());

    for (int i = 0; i < modeNames.size(); ++i)
    {
        const bool active = (i == selected);
        g.setColour (active ? modeActiveColour : modeIdleColour);
        g.setFont (juce::Font (juce::FontOptions (active ? 10.0f : 9.5f,
                                                  active ? juce::Font::bold : juce::Font::plain)));
        g.drawText (modeNames[i],
                    juce::Rectangle<int> ((int) modeLabelCentres[i] - 40, modeStripArea.getY() + 3, 80, 13),
                    juce::Justification::centred, false);
    }
}

void DAroomAudioProcessorEditor::resized()
{
    // Hit regions align to the four rotary controls in the exact supplied image.
    mix.setBounds (17, 72, 50, 47);
    predelay.setBounds (100, 72, 52, 47);
    decay.setBounds (497, 70, 54, 49);
    size.setBounds (579, 70, 54, 49);

    // Value overlays replace the baked-in static values, so the numbers follow controls.
    mixValue.setBounds (mixReadout);
    predelayValue.setBounds (predelayReadout);
    decayValue.setBounds (decayReadout);
    sizeValue.setBounds (sizeReadout);

    // The mode selector overlays the mode strip from the artwork; the preset
    // browser covers the name area between the two arrows.
    mode.setBounds (modeStripArea.withY (modeStripArea.getY() - 2).withHeight (22));
    presetSelector.setBounds (presetNameArea);
}
