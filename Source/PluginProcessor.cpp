#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float decayMin = 0.2f;
    constexpr float decayMax = 12.0f;

    // Per-mode voicing: room-size scale, high-frequency damping and stereo width.
    const float modeSizeScale[DAroomAudioProcessor::numModes] = { 0.86f, 1.00f, 0.94f, 0.90f, 0.80f };
    const float modeDamping  [DAroomAudioProcessor::numModes] = { 0.62f, 0.42f, 0.22f, 0.50f, 0.72f };
    const float modeWidth    [DAroomAudioProcessor::numModes] = { 0.85f, 1.00f, 1.00f, 0.90f, 0.70f };
}

DAroomAudioProcessor::DAroomAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "DAroomState", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout DAroomAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<APF> ("mix", "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 26.0f, "%"));
    layout.add (std::make_unique<APF> ("predelay", "Predelay", juce::NormalisableRange<float> (0.0f, 250.0f, 0.1f), 25.0f, " ms"));
    layout.add (std::make_unique<APF> ("decay", "Decay", juce::NormalisableRange<float> (decayMin, decayMax, 0.01f), 4.5f, " s"));
    layout.add (std::make_unique<APF> ("size", "Size", juce::NormalisableRange<float> (10.0f, 100.0f, 0.1f), 78.0f, "%"));
    layout.add (std::make_unique<juce::AudioParameterChoice> ("mode", "Mode",
        juce::StringArray { "Room", "Hall", "Plate", "Chamber", "Ambience" }, 3));
    return layout;
}

void DAroomAudioProcessor::setParameterValues (float mixPercent, float predelayMs, float decaySeconds, float sizePercent, int modeIndex)
{
    auto setParam = [this] (const char* id, float value)
    {
        if (auto* p = parameters.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };

    setParam ("mix", mixPercent);
    setParam ("predelay", predelayMs);
    setParam ("decay", juce::jlimit (decayMin, decayMax, decaySeconds));
    setParam ("size", sizePercent);
    setParam ("mode", (float) juce::jlimit (0, numModes - 1, modeIndex));
}

void DAroomAudioProcessor::applyPreset (float mixPercent, float predelayMs, float decaySeconds, float sizePercent, int modeIndex)
{
    setParameterValues (mixPercent, predelayMs, decaySeconds, sizePercent, modeIndex);
}

void DAroomAudioProcessor::captureCurrent (SlotState& slot) const
{
    slot.valid         = true;
    slot.mix           = parameters.getRawParameterValue ("mix")->load();
    slot.predelayMs    = parameters.getRawParameterValue ("predelay")->load();
    slot.decaySeconds  = parameters.getRawParameterValue ("decay")->load();
    slot.sizePercent   = parameters.getRawParameterValue ("size")->load();
    slot.mode          = (int) parameters.getRawParameterValue ("mode")->load();
    slot.presetIndex   = presetIndex.load();
}

void DAroomAudioProcessor::selectSlot (int slot)
{
    const int s = (slot == 1) ? 1 : 0;
    if (s == activeSlot)
        return;

    if (! slots[s].valid)
        captureCurrent (slots[s]);   // empty slot: take a copy of the current sound
    else
        setParameterValues (slots[s].mix, slots[s].predelayMs, slots[s].decaySeconds,
                            slots[s].sizePercent, slots[s].mode);

    if (slots[s].presetIndex != unsetPresetIndex)
        presetIndex.store (slots[s].presetIndex);

    activeSlot = s;
}

void DAroomAudioProcessor::storeSlot (int slot)
{
    const int s = (slot == 1) ? 1 : 0;
    captureCurrent (slots[s]);
    activeSlot = s;
}

void DAroomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    reverb.reset();
    reverb.setSampleRate (currentSampleRate);

    ensurePredelayBuffer (samplesPerBlock);

    wetBuffer.setSize (2, juce::jmax (64, samplesPerBlock), false, false, false);
    wetBuffer.clear();

    // Start the mix smoothers at the *current* parameter value so recalling a
    // preset or restoring a session does not cause an audible jump on the
    // first processed block.
    const float mix = parameters.getRawParameterValue ("mix")->load() * 0.01f;
    wetSmoother.reset (currentSampleRate, 0.03);
    drySmoother.reset (currentSampleRate, 0.03);
    wetSmoother.setCurrentAndTargetValue (mix);
    drySmoother.setCurrentAndTargetValue (1.0f - mix);
}

void DAroomAudioProcessor::releaseResources()
{
    // Keep the allocated buffers (hosts cycle prepare/release often); just
    // silence the tail so a resumed stream cannot play stale audio.
    predelayBuffer.clear();
    predelayWritePosition = 0;
    wetBuffer.clear();
}

bool DAroomAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void DAroomAudioProcessor::ensurePredelayBuffer (int samplesPerBlock)
{
    const int needed = (int) std::ceil (currentSampleRate * 0.30) + juce::jmax (64, samplesPerBlock);

    if (predelayBuffer.getNumChannels() < 2 || predelayBuffer.getNumSamples() < needed)
    {
        predelayBuffer.setSize (2, needed);
        predelayBuffer.clear();
        predelayWritePosition = 0;
    }
}

void DAroomAudioProcessor::updateReverbParameters (float sizeNorm, float decayNorm, int mode) noexcept
{
    const int m = juce::jlimit (0, numModes - 1, mode);

    juce::Reverb::Parameters p;
    p.roomSize   = juce::jlimit (0.25f, 0.98f, (0.30f + 0.42f * sizeNorm + 0.26f * decayNorm) * modeSizeScale[m]);
    p.damping    = juce::jlimit (0.05f, 0.95f, modeDamping[m] * (1.0f - 0.30f * decayNorm));
    p.wetLevel   = 1.0f;
    p.dryLevel   = 0.0f;
    p.width      = modeWidth[m];
    p.freezeMode = 0.0f;
    reverb.setParameters (p);
}

void DAroomAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn      = getTotalNumInputChannels();
    const int numOut     = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numIn <= 0 || numOut <= 0 || numSamples <= 0)
        return;

    // Some hosts may deliver audio before prepareToPlay(); stay robust.
    if (predelayBuffer.getNumSamples() == 0)
        ensurePredelayBuffer (numSamples);

    const float mix        = parameters.getRawParameterValue ("mix")->load() * 0.01f;
    const float predelayMs = parameters.getRawParameterValue ("predelay")->load();
    const float decay      = parameters.getRawParameterValue ("decay")->load();
    const float size       = parameters.getRawParameterValue ("size")->load() * 0.01f;
    const int   mode       = (int) parameters.getRawParameterValue ("mode")->load();

    updateReverbParameters (size, (decay - decayMin) / (decayMax - decayMin), mode);

    wetSmoother.setTargetValue (mix);
    drySmoother.setTargetValue (1.0f - mix);

    const bool stereo = numIn > 1;

    float* in0 = buffer.getWritePointer (0);
    float* in1 = stereo ? buffer.getWritePointer (1) : nullptr;

    // ---- predelay ring buffer (wet path only) -------------------------------
    const int ringLen = predelayBuffer.getNumSamples();
    const int clampedDelay = juce::jlimit (0, ringLen - 1,
                                           juce::roundToInt (predelayMs * 0.001 * currentSampleRate));

    float* pd0 = predelayBuffer.getWritePointer (0);
    float* pd1 = predelayBuffer.getWritePointer (1);

    if (wetBuffer.getNumSamples() < numSamples)
        wetBuffer.setSize (2, numSamples, false, false, false);

    float* wet0 = wetBuffer.getWritePointer (0);
    float* wet1 = wetBuffer.getWritePointer (1);

    int writePos = predelayWritePosition;

    for (int i = 0; i < numSamples; ++i)
    {
        const float l = in0[i];
        const float r = stereo ? in1[i] : l;

        float dl = l, dr = r;

        if (clampedDelay > 0)
        {
            int readPos = writePos - clampedDelay;
            if (readPos < 0)
                readPos += ringLen;
            dl = pd0[readPos];
            dr = pd1[readPos];
        }

        pd0[writePos] = l;
        pd1[writePos] = r;
        writePos = (writePos + 1 >= ringLen) ? 0 : writePos + 1;

        wet0[i] = dl;
        wet1[i] = dr;
    }

    predelayWritePosition = writePos;

    // ---- reverb --------------------------------------------------------------
    if (stereo)
        reverb.processStereo (wet0, wet1, numSamples);
    else
        reverb.processMono (wet0, numSamples);

    // ---- mix with smoothing ----------------------------------------------------
    for (int i = 0; i < numSamples; ++i)
    {
        const float wet = wetSmoother.getNextValue();
        const float dry = drySmoother.getNextValue();

        in0[i] = in0[i] * dry + wet0[i] * wet;

        if (stereo)
            in1[i] = in1[i] * dry + wet1[i] * wet;
    }
}

double DAroomAudioProcessor::getTailLengthSeconds() const
{
    const double decay    = (double) parameters.getRawParameterValue ("decay")->load();
    const double predelay = (double) parameters.getRawParameterValue ("predelay")->load() * 0.001;
    return juce::jlimit (0.1, 30.0, decay + predelay + 0.5);
}

juce::AudioProcessorEditor* DAroomAudioProcessor::createEditor()
{
    return new DAroomAudioProcessorEditor (*this);
}

void DAroomAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());

    if (xml != nullptr)
    {
        xml->setAttribute ("presetIndex", presetIndex.load());

        auto* slotsXml = new juce::XmlElement ("slots");
        slotsXml->setAttribute ("active", activeSlot);
        for (int i = 0; i < numSlots; ++i)
        {
            auto* e = new juce::XmlElement ("slot");
            e->setAttribute ("id", i);
            e->setAttribute ("valid", slots[i].valid ? 1 : 0);
            e->setAttribute ("mix", (double) slots[i].mix);
            e->setAttribute ("predelay", (double) slots[i].predelayMs);
            e->setAttribute ("decay", (double) slots[i].decaySeconds);
            e->setAttribute ("size", (double) slots[i].sizePercent);
            e->setAttribute ("mode", slots[i].mode);
            e->setAttribute ("preset", slots[i].presetIndex);
            slotsXml->addChildElement (e);
        }
        xml->addChildElement (slotsXml);

        copyXmlToBinary (*xml, destData);
    }
}

void DAroomAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
    {
        presetIndex.store (xml->getIntAttribute ("presetIndex", unsetPresetIndex));

        if (auto* slotsXml = xml->getChildByName ("slots"))
        {
            activeSlot = juce::jlimit (0, numSlots - 1, slotsXml->getIntAttribute ("active", 0));
            for (auto* e : slotsXml->getChildWithTagNameIterator ("slot"))
            {
                const int id = juce::jlimit (0, numSlots - 1, e->getIntAttribute ("id", 0));
                slots[id].valid         = e->getBoolAttribute ("valid", false);
                slots[id].mix           = (float) e->getDoubleAttribute ("mix", slots[id].mix);
                slots[id].predelayMs    = (float) e->getDoubleAttribute ("predelay", slots[id].predelayMs);
                slots[id].decaySeconds  = (float) e->getDoubleAttribute ("decay", slots[id].decaySeconds);
                slots[id].sizePercent   = (float) e->getDoubleAttribute ("size", slots[id].sizePercent);
                slots[id].mode          = juce::jlimit (0, numModes - 1, e->getIntAttribute ("mode", slots[id].mode));
                slots[id].presetIndex   = e->getIntAttribute ("preset", unsetPresetIndex);
            }
        }

        parameters.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DAroomAudioProcessor();
}
