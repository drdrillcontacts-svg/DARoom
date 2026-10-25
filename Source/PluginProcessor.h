#pragma once
#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

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
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    void applyPreset (float mixPercent, float predelayMs, float decaySeconds, float sizePercent, int modeIndex);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;
    juce::AudioBuffer<float> predelayBuffer;
    int predelayWritePosition = 0;
    double currentSampleRate = 44100.0;
    juce::LinearSmoothedValue<float> wetSmoother, drySmoother;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DAroomAudioProcessor)
};
