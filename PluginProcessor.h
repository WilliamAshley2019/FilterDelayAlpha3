// === File: PluginProcessor.h ===
#pragma once

#include <JuceHeader.h>

class DelayFilterPluginAudioProcessor : public juce::AudioProcessor
{
public:
    DelayFilterPluginAudioProcessor();
    ~DelayFilterPluginAudioProcessor() override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DelayFilterPlugin"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter state
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

private:
    // Delay buffer and indices
    juce::AudioBuffer<float> delayBuffer;
    int writePosition{ 0 };
    double currentSampleRate{ 44100.0 };
    double lfoPhase{ 0.0 };

    // IIR
    std::array<juce::dsp::IIR::Filter<float>, 2> iirFilter;
    juce::dsp::IIR::Coefficients<float>::Ptr iirCoeffs;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothFilterFreq;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothQ;

    // Phaser allpass states (2 stages, stereo)
    float ap_x1[2]{}, ap_y1[2]{}, ap_x2[2]{}, ap_y2[2]{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayFilterPluginAudioProcessor)
};