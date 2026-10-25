#pragma once
#include <JuceHeader.h>

#include <atomic>
#include <cmath>

class DAroomAudioProcessor final : public juce::AudioProcessor
{
public:
    DAroomAudioProcessor();
    ~DAroomAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    void applyPreset (float mixPercent, float predelayMs, float decaySeconds, float sizePercent, int modeIndex);

    /** Index of the last recalled factory preset (1..36), 0 = "Manual",
        unsetPresetIndex = state never stored one. Persisted inside the plug-in state. */
    static constexpr int unsetPresetIndex = -1;
    int getPresetIndex() const noexcept { return presetIndex.load(); }
    void setPresetIndex (int newIndex) noexcept { presetIndex.store (newIndex); }

    static constexpr int numModes = 5;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateReverbParameters (float sizeNorm, float decayNorm, int mode) noexcept;
    void ensurePredelayBuffer (int samplesPerBlock);

    juce::Reverb reverb;
    juce::AudioBuffer<float> predelayBuffer;
    juce::AudioBuffer<float> wetBuffer;
    int predelayWritePosition = 0;
    double currentSampleRate = 44100.0;
    juce::LinearSmoothedValue<float> wetSmoother, drySmoother;
    std::atomic<int> presetIndex { unsetPresetIndex };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DAroomAudioProcessor)
};
