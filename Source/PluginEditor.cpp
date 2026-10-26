#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    // ---- artwork geometry (653 x 184 reference image) -------------------------
    const juce::StringArray modeNames { "ROOM", "HALL", "PLATE", "CHAMBER", "AMBIENCE" };

    const juce::Rectangle<int> mixReadout      { 23, 137, 38, 14 };
    const juce::Rectangle<int> predelayReadout { 99, 137, 54, 14 };
    const juce::Rectangle<int> decayReadout    { 495, 137, 58, 14 };
    const juce::Rectangle<int> sizeReadout     { 585, 137, 43, 14 };
    const juce::Rectangle<int> titleArea       { 18, 12, 102, 20 };
    const juce::Rectangle<int> titleTextRect   { 20, 12, 110, 21 };
    const juce::Rectangle<int> presetNameArea  { 250, 15, 150, 20 };
    const juce::Rectangle<int> modeStripBox    { 168, 158, 322, 22 };
    const juce::Rectangle<int> slotARect       { 576, 12, 26, 24 };
    const juce::Rectangle<int> slotBRect       { 604, 12, 26, 24 };
    const juce::Rectangle<int> leftArrowArea   { 228, 14, 22, 22 };
    const juce::Rectangle<int> rightArrowArea  { 401, 14, 22, 22 };

    struct KnobGeom { juce::Point<float> centre; float radius; };
    const KnobGeom knobs[4] = { { { 42.0f, 95.5f }, 23.0f },
                                { { 126.0f, 95.5f }, 23.0f },
                                { { 524.0f, 94.5f }, 23.0f },
                                { { 606.0f, 94.5f }, 23.0f } };

    // ---- colours sampled from the reference artwork --------------------------
    const juce::Colour mixEraseColour      { 0xff030e13 };
    const juce::Colour predelayEraseColour { 0xff040f14 };
    const juce::Colour decayEraseColour    { 0xff061117 };
    const juce::Colour sizeEraseColour     { 0xff030c11 };
    const juce::Colour presetEraseColour   { 0xff0b1a21 };
    const juce::Colour titleTopColour      { 0xff040f15 };
    const juce::Colour titleBottomColour   { 0xff030e13 };
    const juce::Colour titleTextColour     { 0xffe8f4f7 };
    const juce::Colour stripTopColour      { 0xff071218 };
    const juce::Colour stripBottomColour   { 0xff050f15 };
    const juce::Colour modeIdleColour      { 0xff8b9aa2 };
    const juce::Colour modeActiveColour    { 0xffeef4f7 };
    const juce::Colour knobFaceColour      { 0xff0a1118 };
    const juce::Colour knobRimColour       { 0xff46545e };
    const juce::Colour knobTrackColour     { 0xff1d2831 };
    const juce::Colour knobValueColour     { 0xffd9e6ec };
    const juce::Colour knobPointerColour   { 0xffe6f0f4 };
    const juce::Colour slotActiveLine      { 0xffbfe3f2 };
    const juce::Colour slotActiveFill      { 0x26ffffff };
}

//==============================================================================
/** Mode selector drawn over the artwork strip: clicking a name selects it
    directly - no popup menu, no dropdown arrow. */
class DAroomAudioProcessorEditor::ModeStrip final : public juce::Component,
                                                     private juce::Timer
{
public:
    explicit ModeStrip (DAroomAudioProcessor& p) : processor (p) { startTimerHz (20); }

    void paint (juce::Graphics& g) override
    {
        g.setGradientFill (juce::ColourGradient (stripTopColour, 0.0f, 0.0f,
                                                 stripBottomColour, 0.0f, (float) getHeight(),
                                                 false));
        g.fillAll();

        const int selected = currentMode();
        const float w = (float) getWidth();
        const float h = (float) getHeight();

        for (int i = 0; i < modeNames.size(); ++i)
        {
            const bool active = (i == selected);
            g.setColour (active ? modeActiveColour : modeIdleColour);
            g.setFont (juce::Font (juce::FontOptions (h * (active ? 0.46f : 0.43f),
                                                      active ? juce::Font::bold : juce::Font::plain)));
            const float centre = proportions[i] * w;
            g.drawText (modeNames[i],
                        juce::Rectangle<int> ((int) (centre - w * 0.13f), (int) (h * 0.18f),
                                              (int) (w * 0.26f), (int) (h * 0.64f)),
                        juce::Justification::centred, false);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const float x = (float) e.getPosition().x;
        int best = 0;
        float bestDistance = 1.0e9f;

        for (int i = 0; i < 5; ++i)
        {
            const float d = std::abs (proportions[i] * (float) getWidth() - x);
            if (d < bestDistance) { bestDistance = d; best = i; }
        }

        setMode (best);
    }

private:
    int currentMode() const
    {
        return juce::jlimit (0, 4, (int) processor.parameters.getRawParameterValue ("mode")->load());
    }

    void setMode (int m)
    {
        if (m == currentMode())
            return;

        if (auto* p = processor.parameters.getParameter ("mode"))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) m));
            p->endChangeGesture();
        }
    }

    void timerCallback() override
    {
        const int m = currentMode();
        if (m != lastMode) { lastMode = m; repaint(); }
    }

    DAroomAudioProcessor& processor;
    int lastMode = -1;

    // label centres of the artwork strip, as proportions of the strip width
    static constexpr float proportions[5] = { 15.0f / 322.0f, 76.0f / 322.0f, 135.0f / 322.0f,
                                              204.0f / 322.0f, 283.0f / 322.0f };
};

//==============================================================================
/** A/B slot pad: click = switch (empty slots capture the current sound),
    double-click = store the current sound into the slot. */
class DAroomAudioProcessorEditor::SlotButton final : public juce::Component
{
public:
    SlotButton (DAroomAudioProcessor& p, int slotIndex, std::function<void()> onChanged)
        : processor (p), slot (slotIndex), onChange (std::move (onChanged)) {}

    void paint (juce::Graphics& g) override
    {
        if (processor.getActiveSlot() != slot)
            return;

        const auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (slotActiveFill);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (slotActiveLine);
        g.drawRoundedRectangle (r, 5.0f, 1.5f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.getNumberOfClicks() >= 2)
            processor.storeSlot (slot);
        else
            processor.selectSlot (slot);

        if (onChange != nullptr)
            onChange();
    }

private:
    DAroomAudioProcessor& processor;
    int slot;
    std::function<void()> onChange;
};

namespace
{
    /** Appends a line to a small log file so editor-creation problems can be
        diagnosed on the user's machine (Documents/DARoom-editor.log). */
    void appendEditorLog (const juce::String& message)
    {
        try
        {
            auto log = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                         .getChildFile ("DARoom-editor.log");
            log.appendText (juce::Time::getCurrentTime().toString (true, true) + "  " + message + juce::newLine);
        }
        catch (...) {}
    }
}

//==============================================================================
juce::Array<DAroomAudioProcessorEditor::PresetDef> DAroomAudioProcessorEditor::loadFactoryPresets()
{
    struct Resource { const char* file; const char* data; int size; };

    // The .drpreset files shipped in Presets/ are embedded as binary data, so
    // the plug-in and the on-disk presets can never drift apart.
    static const Resource resources[] =
    {
    { "small-studio.drpreset", BinaryData::smallstudio_drpreset, BinaryData::smallstudio_drpresetSize },  // 1: Small Studio [Rooms]
    { "vocal-booth.drpreset", BinaryData::vocalbooth_drpreset, BinaryData::vocalbooth_drpresetSize },  // 2: Vocal Booth [Rooms]
    { "living-room.drpreset", BinaryData::livingroom_drpreset, BinaryData::livingroom_drpresetSize },  // 3: Living Room [Rooms]
    { "wood-room.drpreset", BinaryData::woodroom_drpreset, BinaryData::woodroom_drpresetSize },  // 4: Wood Room [Rooms]
    { "drum-room.drpreset", BinaryData::drumroom_drpreset, BinaryData::drumroom_drpresetSize },  // 5: Drum Room [Rooms]
    { "bright-room.drpreset", BinaryData::brightroom_drpreset, BinaryData::brightroom_drpresetSize },  // 6: Bright Room [Rooms]
    { "tight-booth.drpreset", BinaryData::tightbooth_drpreset, BinaryData::tightbooth_drpresetSize },  // 7: Tight Booth [Rooms]
    { "tracking-room.drpreset", BinaryData::trackingroom_drpreset, BinaryData::trackingroom_drpresetSize },  // 8: Tracking Room [Rooms]
    { "whisper-room.drpreset", BinaryData::whisperroom_drpreset, BinaryData::whisperroom_drpresetSize },  // 9: Whisper Room [Rooms]
    { "cozy-loft.drpreset", BinaryData::cozyloft_drpreset, BinaryData::cozyloft_drpresetSize },  // 10: Cozy Loft [Rooms]
    { "brick-garage.drpreset", BinaryData::brickgarage_drpreset, BinaryData::brickgarage_drpresetSize },  // 11: Brick Garage [Rooms]
    { "studio-b.drpreset", BinaryData::studiob_drpreset, BinaryData::studiob_drpresetSize },  // 12: Studio B [Rooms]
    { "small-hall.drpreset", BinaryData::smallhall_drpreset, BinaryData::smallhall_drpresetSize },  // 13: Small Hall [Halls]
    { "concert-hall.drpreset", BinaryData::concerthall_drpreset, BinaryData::concerthall_drpresetSize },  // 14: Concert Hall [Halls]
    { "dark-hall.drpreset", BinaryData::darkhall_drpreset, BinaryData::darkhall_drpresetSize },  // 15: Dark Hall [Halls]
    { "bright-hall.drpreset", BinaryData::brighthall_drpreset, BinaryData::brighthall_drpresetSize },  // 16: Bright Hall [Halls]
    { "cathedral.drpreset", BinaryData::cathedral_drpreset, BinaryData::cathedral_drpresetSize },  // 17: Cathedral [Halls]
    { "arena.drpreset", BinaryData::arena_drpreset, BinaryData::arena_drpresetSize },  // 18: Arena [Halls]
    { "piano-bloom.drpreset", BinaryData::pianobloom_drpreset, BinaryData::pianobloom_drpresetSize },  // 19: Piano Bloom [Halls]
    { "orchestral-depth.drpreset", BinaryData::orchestraldepth_drpreset, BinaryData::orchestraldepth_drpresetSize },  // 20: Orchestral Depth [Halls]
    { "grand-hall.drpreset", BinaryData::grandhall_drpreset, BinaryData::grandhall_drpresetSize },  // 21: Grand Hall [Halls]
    { "nordic-hall.drpreset", BinaryData::nordichall_drpreset, BinaryData::nordichall_drpresetSize },  // 22: Nordic Hall [Halls]
    { "opera-house.drpreset", BinaryData::operahouse_drpreset, BinaryData::operahouse_drpresetSize },  // 23: Opera House [Halls]
    { "tape-bloom.drpreset", BinaryData::tapebloom_drpreset, BinaryData::tapebloom_drpresetSize },  // 24: Tape Bloom [Halls]
    { "vintage-plate.drpreset", BinaryData::vintageplate_drpreset, BinaryData::vintageplate_drpresetSize },  // 25: Vintage Plate [Plates]
    { "vocal-plate.drpreset", BinaryData::vocalplate_drpreset, BinaryData::vocalplate_drpresetSize },  // 26: Vocal Plate [Plates]
    { "bright-plate.drpreset", BinaryData::brightplate_drpreset, BinaryData::brightplate_drpresetSize },  // 27: Bright Plate [Plates]
    { "drum-plate.drpreset", BinaryData::drumplate_drpreset, BinaryData::drumplate_drpresetSize },  // 28: Drum Plate [Plates]
    { "guitar-springless.drpreset", BinaryData::guitarspringless_drpreset, BinaryData::guitarspringless_drpresetSize },  // 29: Guitar Springless [Plates]
    { "snare-bloom.drpreset", BinaryData::snarebloom_drpreset, BinaryData::snarebloom_drpresetSize },  // 30: Snare Bloom [Plates]
    { "silver-plate.drpreset", BinaryData::silverplate_drpreset, BinaryData::silverplate_drpresetSize },  // 31: Silver Plate [Plates]
    { "studio-plate.drpreset", BinaryData::studioplate_drpreset, BinaryData::studioplate_drpresetSize },  // 32: Studio Plate [Plates]
    { "deep-chamber.drpreset", BinaryData::deepchamber_drpreset, BinaryData::deepchamber_drpresetSize },  // 33: Deep Chamber [Chambers]
    { "stone-chamber.drpreset", BinaryData::stonechamber_drpreset, BinaryData::stonechamber_drpresetSize },  // 34: Stone Chamber [Chambers]
    { "warm-chamber.drpreset", BinaryData::warmchamber_drpreset, BinaryData::warmchamber_drpresetSize },  // 35: Warm Chamber [Chambers]
    { "dark-chamber.drpreset", BinaryData::darkchamber_drpreset, BinaryData::darkchamber_drpresetSize },  // 36: Dark Chamber [Chambers]
    { "gold-chamber.drpreset", BinaryData::goldchamber_drpreset, BinaryData::goldchamber_drpresetSize },  // 37: Gold Chamber [Chambers]
    { "echo-chamber.drpreset", BinaryData::echochamber_drpreset, BinaryData::echochamber_drpresetSize },  // 38: Echo Chamber [Chambers]
    { "marble-chamber.drpreset", BinaryData::marblechamber_drpreset, BinaryData::marblechamber_drpresetSize },  // 39: Marble Chamber [Chambers]
    { "ambient-cloud.drpreset", BinaryData::ambientcloud_drpreset, BinaryData::ambientcloud_drpresetSize },  // 40: Ambient Cloud [Ambiences]
    { "dream-space.drpreset", BinaryData::dreamspace_drpreset, BinaryData::dreamspace_drpresetSize },  // 41: Dream Space [Ambiences]
    { "cosmic-drift.drpreset", BinaryData::cosmicdrift_drpreset, BinaryData::cosmicdrift_drpresetSize },  // 42: Cosmic Drift [Ambiences]
    { "frozen-air.drpreset", BinaryData::frozenair_drpreset, BinaryData::frozenair_drpresetSize },  // 43: Frozen Air [Ambiences]
    { "soft-focus.drpreset", BinaryData::softfocus_drpreset, BinaryData::softfocus_drpresetSize },  // 44: Soft Focus [Ambiences]
    { "wide-synth.drpreset", BinaryData::widesynth_drpreset, BinaryData::widesynth_drpresetSize },  // 45: Wide Synth [Ambiences]
    { "cinematic-impact.drpreset", BinaryData::cinematicimpact_drpreset, BinaryData::cinematicimpact_drpresetSize },  // 46: Cinematic Impact [Ambiences]
    { "endless-horizon.drpreset", BinaryData::endlesshorizon_drpreset, BinaryData::endlesshorizon_drpresetSize },  // 47: Endless Horizon [Ambiences]
    { "airy-pad.drpreset", BinaryData::airypad_drpreset, BinaryData::airypad_drpresetSize },  // 48: Airy Pad [Ambiences]
    { "nebula.drpreset", BinaryData::nebula_drpreset, BinaryData::nebula_drpresetSize },  // 49: Nebula [Ambiences]
    { "glacier.drpreset", BinaryData::glacier_drpreset, BinaryData::glacier_drpresetSize },  // 50: Glacier [Ambiences]
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

    const juce::PluginHostType hostType;
    appendEditorLog ("editor constructing; host=" + juce::String (hostType.getHostDescription())
                     + "; os=" + juce::SystemStats::getOperatingSystemName());

    setSize (653, 184);
    setResizable (true, true);
    setResizeLimits (457, 129, 1959, 552);

    styleSlider (mix, "MIX");
    styleSlider (predelay, "PREDELAY");
    styleSlider (decay, "DECAY");
    styleSlider (size, "SIZE");

    addAndMakeVisible (mix);
    addAndMakeVisible (predelay);
    addAndMakeVisible (decay);
    addAndMakeVisible (size);

    for (auto* label : { &mixValue, &predelayValue, &decayValue, &sizeValue })
    {
        styleValue (*label);
        addAndMakeVisible (*label);
    }

    // Preset browser grouped by space type: id 1 = "Manual", ids 2..51 = presets.
    presetSelector.addItem ("Manual", 1);
    const juce::StringArray headings { "Rooms", "Halls", "Plates", "Chambers", "Ambiences" };
    for (int m = 0; m < headings.size(); ++m)
    {
        presetSelector.addSectionHeading (headings[m]);
        for (int i = 0; i < factoryPresets.size(); ++i)
            if (factoryPresets[i].mode == m)
                presetSelector.addItem (factoryPresets[i].name, i + 2);
    }

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

    modeStrip = std::make_unique<ModeStrip> (audioProcessor);
    addAndMakeVisible (*modeStrip);

    const auto slotChanged = [this] { syncPresetCombo(); repaint(); };
    slotA = std::make_unique<SlotButton> (audioProcessor, 0, slotChanged);
    slotB = std::make_unique<SlotButton> (audioProcessor, 1, slotChanged);
    addAndMakeVisible (*slotA);
    addAndMakeVisible (*slotB);

    auto& state = audioProcessor.parameters;
    mixAttachment      = std::make_unique<SliderAttachment> (state, "mix", mix);
    predelayAttachment = std::make_unique<SliderAttachment> (state, "predelay", predelay);
    decayAttachment    = std::make_unique<SliderAttachment> (state, "decay", decay);
    sizeAttachment     = std::make_unique<SliderAttachment> (state, "size", size);

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
        repaintKnobs();
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

    appendEditorLog ("editor ready; background=" + juce::String (background.isValid() ? "ok" : "MISSING")
                     + "; presets=" + juce::String (factoryPresets.size())
                     + "; size=" + juce::String (getWidth()) + "x" + juce::String (getHeight()));
}

void DAroomAudioProcessorEditor::recallPreset (int presetIndex)
{
    if (presetIndex < 1 || presetIndex > factoryPresets.size())
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

void DAroomAudioProcessorEditor::syncPresetCombo()
{
    const int idx = juce::jlimit (0, factoryPresets.size(), audioProcessor.getPresetIndex());
    presetSelector.setSelectedId (idx + 1, juce::dontSendNotification);
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
    if (scaled (leftArrowArea).contains (e.getPosition()))
    {
        stepPreset (-1);
        return;
    }
    if (scaled (rightArrowArea).contains (e.getPosition()))
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

juce::Rectangle<int> DAroomAudioProcessorEditor::scaled (juce::Rectangle<int> base) const
{
    return { juce::roundToInt ((float) base.getX() * uiScaleX),
             juce::roundToInt ((float) base.getY() * uiScaleY),
             juce::roundToInt ((float) base.getWidth() * uiScaleX),
             juce::roundToInt ((float) base.getHeight() * uiScaleY) };
}

void DAroomAudioProcessorEditor::repaintKnobs()
{
    for (const auto& k : knobs)
        repaint (scaled ({ (int) (k.centre.x - k.radius), (int) (k.centre.y - k.radius),
                           (int) (k.radius * 2.0f), (int) (k.radius * 2.0f) }));
}

void DAroomAudioProcessorEditor::drawKnob (juce::Graphics& g, juce::Point<float> centre,
                                           float radius, float proportion)
{
    const auto face = juce::Rectangle<float> (centre.x - radius, centre.y - radius,
                                              radius * 2.0f, radius * 2.0f);
    g.setColour (knobFaceColour);
    g.fillEllipse (face);
    g.setColour (knobRimColour);
    g.drawEllipse (face.reduced (0.6f), 1.2f);

    const float start = 0.75f * juce::MathConstants<float>::pi;
    const float end   = 2.25f * juce::MathConstants<float>::pi;
    const float sweep = start + juce::jlimit (0.0f, 1.0f, proportion) * (end - start);
    const float arcR  = radius - 4.5f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, start, end, false);
    g.setColour (knobTrackColour);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (proportion > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, start, sweep, false);
        g.setColour (knobValueColour);
        g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const auto dir = juce::Point<float> (std::cos (sweep), std::sin (sweep));
    g.setColour (knobPointerColour);
    g.drawLine ({ centre + dir * (radius * 0.30f), centre + dir * (radius * 0.62f) }, 2.5f);
}

void DAroomAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());
    else
        g.fillAll (juce::Colour (0xff071018));

    // Everything below is drawn in artwork (653 x 184) coordinates.
    g.addTransform (juce::AffineTransform::scale (uiScaleX, uiScaleY));

    // The artwork bakes a static title and readouts into the image; erase them
    // with sampled background colours and redraw live content on top.
    g.setGradientFill (juce::ColourGradient (titleTopColour,
                                             (float) titleArea.getX(), (float) titleArea.getY(),
                                             titleBottomColour,
                                             (float) titleArea.getX(), (float) titleArea.getBottom(),
                                             false));
    g.fillRect (titleArea);
    g.setColour (titleTextColour);
    g.setFont (juce::Font (juce::FontOptions (20.0f * uiScaleY)));
    g.drawText ("D.A Room", titleTextRect, juce::Justification::centredLeft, false);

    g.setColour (mixEraseColour);      g.fillRect (mixReadout);
    g.setColour (predelayEraseColour); g.fillRect (predelayReadout);
    g.setColour (decayEraseColour);    g.fillRect (decayReadout);
    g.setColour (sizeEraseColour);     g.fillRect (sizeReadout);
    g.setColour (presetEraseColour);   g.fillRect (presetNameArea);

    // Live rotary indicators over the baked knob artwork.
    juce::Slider* sliders[4] = { &mix, &predelay, &decay, &size };
    for (int i = 0; i < 4; ++i)
        drawKnob (g, knobs[i].centre, knobs[i].radius,
                  (float) sliders[i]->valueToProportionOfLength (sliders[i]->getValue()));
}

void DAroomAudioProcessorEditor::resized()
{
    uiScaleX = (float) getWidth()  / 653.0f;
    uiScaleY = (float) getHeight() / 184.0f;

    mix.setBounds (scaled ({ 17, 72, 50, 47 }));
    predelay.setBounds (scaled ({ 100, 72, 52, 47 }));
    decay.setBounds (scaled ({ 497, 70, 54, 49 }));
    size.setBounds (scaled ({ 579, 70, 54, 49 }));

    const juce::Rectangle<int> readouts[4] = { mixReadout, predelayReadout, decayReadout, sizeReadout };
    juce::Label* labels[4] = { &mixValue, &predelayValue, &decayValue, &sizeValue };
    for (int i = 0; i < 4; ++i)
    {
        labels[i]->setBounds (scaled (readouts[i]));
        labels[i]->setFont (juce::Font (juce::FontOptions (10.0f * uiScaleY)));
    }

    presetSelector.setBounds (scaled (presetNameArea));
    modeStrip->setBounds (scaled (modeStripBox));
    slotA->setBounds (scaled (slotARect));
    slotB->setBounds (scaled (slotBRect));
}
