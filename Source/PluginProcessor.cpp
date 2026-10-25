#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    layout.add (std::make_unique<APF>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 26.0f, "%"));
    layout.add (std::make_unique<APF>("predelay", "Predelay", juce::NormalisableRange<float>(0.0f, 250.0f, 0.1f), 25.0f, " ms"));
    layout.add (std::make_unique<APF>("decay", "Decay", juce::NormalisableRange<float>(0.2f, 12.0f, 0.01f), 4.5f, " s"));
    layout.add (std::make_unique<APF>("size", "Size", juce::NormalisableRange<float>(10.0f, 100.0f, 0.1f), 76.0f, "%"));
    layout.add (std::make_unique<juce::AudioParameterChoice>("mode", "Mode",
        juce::StringArray {"Room", "Hall", "Plate", "Chamber", "Ambience"}, 3));
    return layout;
}

void DAroomAudioProcessor::applyPreset (float mixPercent, float predelayMs, float decaySeconds, float sizePercent, int modeIndex)
{
    auto setParam = [this] (const char* id, float value)
    {
        if (auto* p = parameters.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(value));
    };
    setParam("mix", mixPercent);
    setParam("predelay", predelayMs);
    setParam("decay", decaySeconds);
    setParam("size", sizePercent);
    if (auto* p = parameters.getParameter("mode"))
        p->setValueNotifyingHost(p->convertTo0to1((float) modeIndex));
}

void DAroomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    reverb.reset();
    reverb.setSampleRate(sampleRate);
    predelayBuffer.setSize(2, juce::jmax(1, (int) std::ceil(sampleRate * 0.30) + samplesPerBlock));
    predelayBuffer.clear();
    predelayWritePosition = 0;
    wetSmoother.reset(sampleRate, 0.03);
    drySmoother.reset(sampleRate, 0.03);
    wetSmoother.setCurrentAndTargetValue(0.26f);
    drySmoother.setCurrentAndTargetValue(0.74f);
}

void DAroomAudioProcessor::releaseResources()
{
    predelayBuffer.setSize(0, 0);
}

bool DAroomAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void DAroomAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, samples);

    const float mix = parameters.getRawParameterValue("mix")->load() / 100.0f;
    const float predelayMs = parameters.getRawParameterValue("predelay")->load();
    const float decay = parameters.getRawParameterValue("decay")->load();
    const float size = parameters.getRawParameterValue("size")->load() / 100.0f;
    const int mode = (int) parameters.getRawParameterValue("mode")->load();

    // A compact, real-time-safe algorithmic reverb foundation. Mode changes tune
    // JUCE's stereo reverb model; this is an original implementation, not a clone.
    const float modeSize[] = { 0.52f, 0.78f, 0.92f, 0.68f, 0.40f };
    const float modeDamping[] = { 0.62f, 0.42f, 0.22f, 0.50f, 0.72f };
    reverbParams.roomSize = juce::jlimit(0.05f, 0.98f, 0.22f + size * 0.55f + (decay / 12.0f) * 0.20f);
    reverbParams.damping = modeDamping[juce::jlimit(0, 4, mode)];
    reverbParams.wetLevel = 1.0f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    wetSmoother.setTargetValue(mix);
    drySmoother.setTargetValue(1.0f - mix);

    const int delaySamples = juce::jlimit(0, predelayBuffer.getNumSamples() - 1,
        (int) std::round(predelayMs * 0.001 * currentSampleRate));

    for (int i = 0; i < samples; ++i)
    {
        float inL = buffer.getSample(0, i);
        float inR = channels > 1 ? buffer.getSample(1, i) : inL;
        float delayedL = inL, delayedR = inR;

        if (predelayBuffer.getNumSamples() > 1)
        {
            if (delaySamples > 0)
            {
                const int readPos = (predelayWritePosition - delaySamples + predelayBuffer.getNumSamples()) % predelayBuffer.getNumSamples();
                delayedL = predelayBuffer.getSample(0, readPos);
                delayedR = channels > 1 ? predelayBuffer.getSample(1, readPos) : delayedL;
            }
            predelayBuffer.setSample(0, predelayWritePosition, inL);
            predelayBuffer.setSample(1, predelayWritePosition, inR);
            predelayWritePosition = (predelayWritePosition + 1) % predelayBuffer.getNumSamples();
        }

        float wetL = delayedL, wetR = delayedR;
        reverb.processStereo(wetL, wetR);
        const float wet = wetSmoother.getNextValue();
        const float dry = drySmoother.getNextValue();
        buffer.setSample(0, i, inL * dry + wetL * wet);
        if (channels > 1)
            buffer.setSample(1, i, inR * dry + wetR * wet);
    }
}

juce::AudioProcessorEditor* DAroomAudioProcessor::createEditor()
{
    return new DAroomAudioProcessorEditor (*this);
}

void DAroomAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DAroomAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DAroomAudioProcessor();
}
