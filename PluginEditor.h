// === File: PluginEditor.h ===
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class DelayFilterPluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    DelayFilterPluginAudioProcessorEditor(DelayFilterPluginAudioProcessor&);
    ~DelayFilterPluginAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DelayFilterPluginAudioProcessor& audioProcessor;

    // GUI components bound to parameters
    juce::ComboBox filterChoice, iirTypeChoice;
    juce::Slider mixSlider, delayMsSlider, feedbackSlider, tapsSlider, tapGainSlider, filterFreqSlider, iirQSlider, lfoRateSlider, lfoDepthSlider;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterChoiceAttachment, iirTypeAttachment;
    std::unique_ptr<Attachment> mixAttachment, delayMsAttachment, feedbackAttachment, tapsAttachment, tapGainAttachment, filterFreqAttachment, iirQAttachment, lfoRateAttachment, lfoDepthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayFilterPluginAudioProcessorEditor)
};