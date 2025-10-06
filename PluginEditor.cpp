// === File: PluginEditor.cpp ===
#include "PluginEditor.h"

DelayFilterPluginAudioProcessorEditor::DelayFilterPluginAudioProcessorEditor(DelayFilterPluginAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(600, 400);

    // Filter choice
    filterChoice.addItem("Comb", 1);
    filterChoice.addItem("FIR", 2);
    filterChoice.addItem("IIR", 3);
    filterChoice.addItem("Phaser", 4);
    filterChoice.addItem("Flanger", 5);
    addAndMakeVisible(filterChoice);
    filterChoiceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "filterType", filterChoice);

    // IIR type choice
    iirTypeChoice.addItem("Low-pass", 1);
    iirTypeChoice.addItem("High-pass", 2);
    iirTypeChoice.addItem("Band-pass", 3);
    addAndMakeVisible(iirTypeChoice);
    iirTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "iirType", iirTypeChoice);

    auto makeSlider = [&](juce::Slider& s, const juce::String& paramID, const juce::String& name, std::unique_ptr<Attachment>& attach)
        {
            (void)name; // unreferenced
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 60, 18);
            addAndMakeVisible(s);
            attach = std::make_unique<Attachment>(audioProcessor.apvts, paramID, s);
        };

    makeSlider(mixSlider, "mix", "Mix", mixAttachment);
    makeSlider(delayMsSlider, "delayMs", "Delay (ms)", delayMsAttachment);
    makeSlider(feedbackSlider, "feedback", "Feedback", feedbackAttachment);
    makeSlider(tapsSlider, "taps", "Taps", tapsAttachment);
    makeSlider(tapGainSlider, "tapGain", "Tap Gain", tapGainAttachment);
    makeSlider(filterFreqSlider, "filterFreq", "Filter Freq", filterFreqAttachment);
    makeSlider(iirQSlider, "iirQ", "IIR Q", iirQAttachment);
    makeSlider(lfoRateSlider, "lfoRate", "LFO Rate", lfoRateAttachment);
    makeSlider(lfoDepthSlider, "lfoDepth", "LFO Depth", lfoDepthAttachment);
}

DelayFilterPluginAudioProcessorEditor::~DelayFilterPluginAudioProcessorEditor() = default;

void DelayFilterPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("Delay-Filter Plugin (JUCE)", getLocalBounds().reduced(10, 10), juce::Justification::centredTop, 1);
}

void DelayFilterPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(12);
    auto topArea = area.removeFromTop(40);
    filterChoice.setBounds(topArea.removeFromLeft(150).reduced(8));
    iirTypeChoice.setBounds(topArea.removeFromRight(150).reduced(8));

    auto row1 = area.removeFromTop(140);
    int nSlidersRow1 = 5;
    int colW1 = row1.getWidth() / nSlidersRow1;
    mixSlider.setBounds(row1.removeFromLeft(colW1).reduced(5));
    delayMsSlider.setBounds(row1.removeFromLeft(colW1).reduced(5));
    feedbackSlider.setBounds(row1.removeFromLeft(colW1).reduced(5));
    tapsSlider.setBounds(row1.removeFromLeft(colW1).reduced(5));
    tapGainSlider.setBounds(row1.removeFromLeft(colW1).reduced(5));

    auto row2 = area.removeFromTop(140);
    int nSlidersRow2 = 4;
    int colW2 = row2.getWidth() / nSlidersRow2;
    filterFreqSlider.setBounds(row2.removeFromLeft(colW2).reduced(5));
    iirQSlider.setBounds(row2.removeFromLeft(colW2).reduced(5));
    lfoRateSlider.setBounds(row2.removeFromLeft(colW2).reduced(5));
    lfoDepthSlider.setBounds(row2.removeFromLeft(colW2).reduced(5));
}