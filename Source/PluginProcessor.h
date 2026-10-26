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

    /** Index of the last recalled factory preset (1..50), 0 = "Manual",
        unsetPresetIndex = state never stored one. Persisted inside the plug-in state. */
    static constexpr int unsetPresetIndex = -1;
    int getPresetIndex() const noexcept { return presetIndex.load(); }
    void setPresetIndex (int newIndex) noexcept { presetIndex.store (newIndex); }

    static constexpr int numModes = 5;

    /** One A/B comparison slot: a full snapshot of the sound parameters. */
    struct SlotState
    {
        bool valid = false;
        float mix = 26.0f, predelayMs = 25.0f, decaySeconds = 4.5f, sizePercent = 78.0f;
        int mode = 3;
        int presetIndex = unsetPresetIndex;
    };

    static constexpr int numSlots = 2;
    int getActiveSlot() const noexcept { return activeSlot; }
    const SlotState& getSlot (int slot) const noexcept { return slots[slot == 1 ? 1 : 0]; }
    /** Switch to a slot: empty slots capture the current sound, filled ones are recalled. */
    void selectSlot (int slot);
    /** Overwrite a slot with the current sound (and make it active). */
    void storeSlot (int slot);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateReverbParameters (float sizeNorm, float decayNorm, int mode) noexcept;
    void ensurePredelayBuffer (int samplesPerBlock);
    void setParameterValues (float mixPercent, float predelayMs, float decaySeconds, float sizePercent, int modeIndex);
    void captureCurrent (SlotState& slot) const;

    juce::Reverb reverb;
    juce::AudioBuffer<float> predelayBuffer;
    juce::AudioBuffer<float> wetBuffer;
    int predelayWritePosition = 0;
    double currentSampleRate = 44100.0;
    juce::LinearSmoothedValue<float> wetSmoother, drySmoother;
    std::atomic<int> presetIndex { unsetPresetIndex };
    SlotState slots[numSlots];
    int activeSlot = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DAroomAudioProcessor)
};
