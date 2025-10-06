// === File: PluginProcessor.cpp ===
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

DelayFilterPluginAudioProcessor::DelayFilterPluginAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameters())
{
}

DelayFilterPluginAudioProcessor::~DelayFilterPluginAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout DelayFilterPluginAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Global
    params.push_back(std::make_unique<juce::AudioParameterChoice>("filterType", "Filter Type",
        juce::StringArray{ "Comb", "FIR", "IIR", "Phaser", "Flanger" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // Delay / comb / flanger
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayMs", "Delay (ms)", juce::NormalisableRange<float>(0.1f, 1000.0f), 20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("feedback", "Feedback", juce::NormalisableRange<float>(-0.99f, 0.99f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("taps", "Taps", juce::NormalisableRange<float>(1.0f, 16.0f, 1.0f), 2.0f));

    // FIR specific: windowed gain control for taps (single master for simplicity)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tapGain", "Tap Gain", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // Unified filter frequency
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filterFreq", "Filter Freq Hz", juce::NormalisableRange<float>(20.0f, 20000.0f), 1000.0f));

    // IIR specific
    params.push_back(std::make_unique<juce::AudioParameterFloat>("iirQ", "IIR Q", juce::NormalisableRange<float>(0.1f, 20.0f), 0.707f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("iirType", "IIR Type",
        juce::StringArray{ "Low-pass", "High-pass", "Band-pass" }, 0));

    // Phaser / flanger LFO
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfoRate", "LFO Rate Hz", juce::NormalisableRange<float>(0.01f, 10.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfoDepth", "LFO Depth ms", juce::NormalisableRange<float>(0.0f, 10.0f), 2.0f));

    return { params.begin(), params.end() };
}

void DelayFilterPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    (void)samplesPerBlock;
    currentSampleRate = sampleRate;
    int maxDelaySamples = static_cast<int>(sampleRate * 2.0); // 2 seconds max
    delayBuffer.setSize(2, maxDelaySamples);
    delayBuffer.clear();
    writePosition = 0;
    lfoPhase = 0.0;

    juce::dsp::ProcessSpec spec{ static_cast<float>(sampleRate), static_cast<juce::uint32>(1), static_cast<juce::uint32>(samplesPerBlock) };
    for (auto& f : iirFilter)
        f.prepare(spec);

    smoothFilterFreq.reset(static_cast<float>(sampleRate), 0.05f);
    smoothQ.reset(static_cast<float>(sampleRate), 0.05f);

    std::fill(std::begin(ap_x1), std::end(ap_x1), 0.0f);
    std::fill(std::begin(ap_y1), std::end(ap_y1), 0.0f);
    std::fill(std::begin(ap_x2), std::end(ap_x2), 0.0f);
    std::fill(std::begin(ap_y2), std::end(ap_y2), 0.0f);
}

void DelayFilterPluginAudioProcessor::releaseResources() {}

bool DelayFilterPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    return true;
}

void DelayFilterPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameter values (atomic-safe snapshot)
    int filterType = static_cast<int>(*apvts.getRawParameterValue("filterType"));
    float mix = *apvts.getRawParameterValue("mix");
    float delayMs = *apvts.getRawParameterValue("delayMs");
    float feedback = *apvts.getRawParameterValue("feedback");
    int taps = static_cast<int>(*apvts.getRawParameterValue("taps"));
    float tapGain = *apvts.getRawParameterValue("tapGain");
    float lfoRate = *apvts.getRawParameterValue("lfoRate");
    float lfoDepthMs = *apvts.getRawParameterValue("lfoDepth");
    int iirType = static_cast<int>(*apvts.getRawParameterValue("iirType"));
    float filterFreq = *apvts.getRawParameterValue("filterFreq");
    float iirQ = *apvts.getRawParameterValue("iirQ");

    const int numSamples = buffer.getNumSamples();
    const int maxDelaySamples = delayBuffer.getNumSamples();
    const int numCh = 2;
    float maxDelaySamplesF = static_cast<float>(maxDelaySamples);

    // Compute effective delay based on filterFreq for non-IIR modes
    float effectiveDelayMs = delayMs;
    if (filterFreq > 0.0f && (filterType == 0 || filterType == 1 || filterType == 4))
    {
        float targetDelaySamples = static_cast<float>(currentSampleRate) / filterFreq;
        effectiveDelayMs = (targetDelaySamples / static_cast<float>(currentSampleRate)) * 1000.0f;
    }
    if (filterType == 1)
    {
        // For FIR, adjust span to have tap spacing corresponding to filterFreq
        if (taps > 1)
            effectiveDelayMs = (1000.0f / filterFreq) * static_cast<float>(taps - 1);
    }

    // Handle IIR separately with block processing
    juce::AudioBuffer<float> dryBuffer(numCh, numSamples);
    bool isIIR = (filterType == 2);
    if (isIIR)
    {
        dryBuffer.makeCopyOf(buffer);

        // Smooth and update coefficients
        smoothFilterFreq.setTargetValue(filterFreq);
        smoothQ.setTargetValue(iirQ);
        float cutoff = smoothFilterFreq.getNextValue();
        float qVal = smoothQ.getNextValue();

        switch (iirType)
        {
        case 0: iirCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, cutoff, qVal); break;
        case 1: iirCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, cutoff, qVal); break;
        case 2: iirCoeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(currentSampleRate, cutoff, qVal); break;
        default: iirCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, cutoff, qVal); break;
        }
        if (iirCoeffs != nullptr)
        {
            for (auto& f : iirFilter)
                f.coefficients = iirCoeffs;
        }

        auto block = juce::dsp::AudioBlock<float>(buffer);
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto monoBlock = block.getSingleChannelBlock(ch);
            auto ctx = juce::dsp::ProcessContextReplacing<float>(monoBlock);
            iirFilter[ch].process(ctx);
        }
    }

    // Cache pointers
    std::vector<float*> channelPtrs(numCh);
    std::vector<float*> delayPtrs(numCh);
    for (int ch = 0; ch < numCh; ++ch)
    {
        channelPtrs[ch] = buffer.getWritePointer(ch);
        delayPtrs[ch] = delayBuffer.getWritePointer(ch);
    }

    // LFO setup
    double currentLfoPhase = lfoPhase;
    double lfoIncrement = 2.0 * juce::MathConstants<double>::pi * lfoRate / currentSampleRate;

    // Process per sample, synced across channels
    int currentWritePos = writePosition;
    for (int i = 0; i < numSamples; ++i)
    {
        // Advance smoothing for IIR (call per sample to properly smooth over time)
        if (isIIR)
        {
            (void)smoothFilterFreq.getNextValue();
            (void)smoothQ.getNextValue();
        }

        // Compute LFO
        double lfoValD = std::sin(currentLfoPhase);
        currentLfoPhase += lfoIncrement;
        if (currentLfoPhase >= 2.0 * juce::MathConstants<double>::pi)
            currentLfoPhase -= 2.0 * juce::MathConstants<double>::pi;
        float lfoVal = static_cast<float>(lfoValD);
        float modMs = lfoDepthMs * lfoVal;

        // Read inputs for this sample (dry)
        float ins[2];
        for (int ch = 0; ch < numCh; ++ch)
            ins[ch] = isIIR ? dryBuffer.getSample(ch, i) : channelPtrs[ch][i];

        // Process each channel
        float wets[2];
        float toWrites[2];
        for (int ch = 0; ch < numCh; ++ch)
        {
            float in = ins[ch];
            float wet = in;

            if (!isIIR)
            {
                switch (filterType)
                {
                case 0: // Comb: feedforward single tap with feedback, interpolated
                {
                    float delaySamplesF = effectiveDelayMs * 0.001f * static_cast<float>(currentSampleRate);
                    float readPos = static_cast<float>(currentWritePos) - delaySamplesF + static_cast<float>(maxDelaySamples);
                    readPos = fmodf(readPos, maxDelaySamplesF);
                    if (readPos < 0.0f) readPos += maxDelaySamplesF;
                    int idx0 = static_cast<int>(readPos);
                    float frac = readPos - static_cast<float>(idx0);
                    int idx1 = (idx0 + 1) % maxDelaySamples;
                    float delayed = (1.0f - frac) * delayPtrs[ch][idx0] + frac * delayPtrs[ch][idx1];
                    wet = in + feedback * delayed;
                    toWrites[ch] = wet;
                    break;
                }
                case 1: // FIR: multi-tap feedforward, interpolated, with Hann window
                {
                    if (taps < 1) taps = 1;
                    float gainPerTap = tapGain / static_cast<float>(taps);
                    wet = 0.0f;
                    float totalSpanMs = effectiveDelayMs;
                    // t=0: current input
                    if (taps > 0)
                    {
                        float frac_t = 0.0f;
                        float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * frac_t));
                        wet += in * gainPerTap * window;
                    }
                    // t=1 to taps-1: past samples
                    for (int t = 1; t < taps; ++t)
                    {
                        float frac_t = static_cast<float>(t) / static_cast<float>(taps - 1);
                        float tapDms = frac_t * totalSpanMs;
                        float tapDsF = tapDms * 0.001f * static_cast<float>(currentSampleRate);
                        float tapReadPos = static_cast<float>(currentWritePos) - tapDsF + static_cast<float>(maxDelaySamples);
                        tapReadPos = fmodf(tapReadPos, maxDelaySamplesF);
                        if (tapReadPos < 0.0f) tapReadPos += maxDelaySamplesF;
                        int idx0 = static_cast<int>(tapReadPos);
                        float frac = tapReadPos - static_cast<float>(idx0);
                        int idx1 = (idx0 + 1) % maxDelaySamples;
                        float tapVal = (1.0f - frac) * delayPtrs[ch][idx0] + frac * delayPtrs[ch][idx1];
                        // Hann window
                        float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * frac_t));
                        wet += tapVal * gainPerTap * window;
                    }
                    toWrites[ch] = in;
                    break;
                }
                case 3: // Phaser: 2 allpass stages, frequency dependent
                {
                    double w0 = 2.0 * juce::MathConstants<double>::pi * filterFreq / currentSampleRate;
                    double tan_half = std::tan(w0 / 2.0);
                    float base_a = static_cast<float>((1.0 - tan_half) / (1.0 + tan_half));
                    float mod_amount = lfoDepthMs / 20.0f; // Scale to reasonable modulation
                    float a = base_a + lfoVal * mod_amount;
                    a = juce::jlimit(-0.99f, 0.99f, a);
                    // Stage 1
                    float out1 = ap_x1[ch] + a * (in - ap_y1[ch]);
                    // Stage 2
                    float out2 = ap_x2[ch] + a * (out1 - ap_y2[ch]);
                    wet = in + feedback * (out2 - in); // Mix dry + (allpass - dry) for phasing
                    // Update states
                    ap_y1[ch] = out1;
                    ap_x1[ch] = in;
                    ap_y2[ch] = out2;
                    ap_x2[ch] = out1;
                    toWrites[ch] = in;
                    break;
                }
                case 4: // Flanger: modulated feedback comb, interpolated
                {
                    float modDelayMs = effectiveDelayMs + modMs;
                    float delaySamplesF = juce::jlimit(0.1f, 1000.0f, modDelayMs) * 0.001f * static_cast<float>(currentSampleRate);
                    float readPos = static_cast<float>(currentWritePos) - delaySamplesF + static_cast<float>(maxDelaySamples);
                    readPos = fmodf(readPos, maxDelaySamplesF);
                    if (readPos < 0.0f) readPos += maxDelaySamplesF;
                    int idx0 = static_cast<int>(readPos);
                    float frac = readPos - static_cast<float>(idx0);
                    int idx1 = (idx0 + 1) % maxDelaySamples;
                    float delayed = (1.0f - frac) * delayPtrs[ch][idx0] + frac * delayPtrs[ch][idx1];
                    wet = in + feedback * delayed;
                    toWrites[ch] = wet;
                    break;
                }
                default:
                    wet = in;
                    toWrites[ch] = in;
                }
            }
            else
            {
                // For IIR, wet is already in buffer
                wet = channelPtrs[ch][i];
                toWrites[ch] = in; // not used
            }
            wets[ch] = wet;

            // Apply mix
            float dry = ins[ch];
            float outSample = dry * (1.0f - mix) + wet * mix;
            channelPtrs[ch][i] = outSample;
        }

        // Write to delay buffer for this sample (only for delay-based filters)
        bool writeDelay = (filterType == 0 || filterType == 1 || filterType == 4);
        if (writeDelay)
        {
            for (int ch = 0; ch < numCh; ++ch)
            {
                delayPtrs[ch][currentWritePos] = toWrites[ch];
            }
        }

        // Advance write position
        currentWritePos = (currentWritePos + 1) % maxDelaySamples;
    }

    // Update LFO phase and write position
    lfoPhase = currentLfoPhase;
    writePosition = currentWritePos;
}

void DelayFilterPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (state.isValid())
    {
        if (auto xml = state.createXml())
        {
            copyXmlToBinary(*xml, destData);
        }
    }
}

void DelayFilterPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        juce::ValueTree tree(juce::ValueTree::fromXml(*xmlState));
        if (tree.isValid() && tree.getType() == apvts.state.getType())
            apvts.replaceState(std::move(tree));
    }
}

void DelayFilterPluginAudioProcessor::setCurrentProgram(int index)
{
    (void)index;
}

const juce::String DelayFilterPluginAudioProcessor::getProgramName(int index)
{
    (void)index;
    return {};
}

void DelayFilterPluginAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    (void)index;
    (void)newName;
}

juce::AudioProcessorEditor* DelayFilterPluginAudioProcessor::createEditor()
{
    return new DelayFilterPluginAudioProcessorEditor(*this);
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayFilterPluginAudioProcessor();
}