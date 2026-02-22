#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LimiterCurves.h"
#include <cstring>
#include <cmath>
#include <functional>

namespace
{
    const juce::StringArray linkedParams = {
        "modern_hold_ms",
        "modern_hold_release_ms",
        "modern_attack_tau_div",
        "modern_release_smooth_base_ms",
        "modern_release_smooth_range_ms",
        "modern_adapt_fast_strength",
        "modern_adapt_slow_strength",
        "modern_sc_hpf_hz",
        "modern_transient_mix",
        "modern_release_fast_ms",
        "modern_release_slow_ms",
        "modern_link_transients",
        "modern_link_release"
    };
}

ClipperLimiterAudioProcessor::ClipperLimiterAudioProcessor()
     : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Load global preference parameters (ui_scale, viz_ticks, viz_range, viz_speed)
    // These parameters persist across all projects and are not affected by DAW state
    loadGlobalPreferences();

    // Do not load user defaults here - DAW will provide project state via setStateInformation()
    // User defaults are loaded lazily in prepareToPlay() if DAW hasn't provided state
    apvts.state.addListener (this);

    apvts.addParameterListener("character", this);
    for (const auto& id : linkedParams)
    {
        apvts.addParameterListener(id, this);
        apvts.addParameterListener(id + "_link", this);
    }

    resetLinks();
    // Timer removed - no longer auto-saving parameters to global file
}

ClipperLimiterAudioProcessor::~ClipperLimiterAudioProcessor()
{
    apvts.removeParameterListener("character", this);
    for (const auto& id : linkedParams)
    {
        apvts.removeParameterListener(id, this);
        apvts.removeParameterListener(id + "_link", this);
    }

    apvts.state.removeListener (this);
    stopTimer();
    if (userDefaultsDirty.load())
        saveUserDefaultsToDisk();
}

juce::File ClipperLimiterAudioProcessor::getUserDefaultsFile() const
{
    auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    return base.getChildFile ("LuminousLab").getChildFile ("Lim.one").getChildFile ("user_defaults.xml");
}

void ClipperLimiterAudioProcessor::loadUserDefaultsFromDisk()
{
    const auto f = getUserDefaultsFile();
    if (! f.existsAsFile())
        return;

    const auto xmlText = f.loadFileAsString();
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (xmlText));
    if (! xml)
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid() || state.getType() != apvts.state.getType())
        return;

    const juce::ScopedValueSetter<bool> scoped (suppressUserDefaultsListener, true);
    apvts.replaceState (state);
}

void ClipperLimiterAudioProcessor::saveUserDefaultsToDisk()
{
    auto f = getUserDefaultsFile();
    f.getParentDirectory().createDirectory();

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (! xml)
        return;

    f.replaceWithText (xml->toString());
}

void ClipperLimiterAudioProcessor::loadUserDefaultsIfNeeded()
{
    // Completely disabled: No user defaults are loaded globally
    // This ensures parameters are never contaminated from previous projects
    // All new plugin instances start with clean, built-in defaults
    // Parameters are managed exclusively by DAW via setStateInformation()
}

void ClipperLimiterAudioProcessor::loadGlobalPreferences()
{
    // Load global preference parameters that persist across all projects
    // These parameters: ui_scale, viz_ticks, viz_range, viz_speed
    const auto f = getUserDefaultsFile();
    if (!f.existsAsFile())
        return;

    const auto xmlText = f.loadFileAsString();
    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(xmlText));
    if (!xml)
        return;

    auto state = juce::ValueTree::fromXml(*xml);
    if (!state.isValid())
        return;

    // Only load specific global preference parameters
    const std::vector<juce::String> globalParams = {
        "ui_scale", "viz_ticks", "viz_range", "viz_speed"
    };

    const juce::ScopedValueSetter<bool> scoped(suppressUserDefaultsListener, true);
    for (const auto& paramId : globalParams)
    {
        if (auto* param = apvts.getParameter(paramId))
        {
            auto paramValue = state.getProperty(paramId, param->getValue());
            param->setValueNotifyingHost(float(paramValue));
        }
    }
}

void ClipperLimiterAudioProcessor::saveGlobalPreferences()
{
    // Save global preference parameters to user_defaults.xml
    // Only saves: ui_scale, viz_ticks, viz_range, viz_speed
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (!xml)
        return;

    // Filter to only global preference parameters
    const std::vector<juce::String> globalParams = {
        "ui_scale", "viz_ticks", "viz_range", "viz_speed"
    };

    auto globalPrefsXml = std::make_unique<juce::XmlElement>("GlobalPreferences");
    for (const auto& paramId : globalParams)
    {
        if (auto* param = apvts.getParameter(paramId))
        {
            globalPrefsXml->setAttribute(paramId, param->getValue());
        }
    }

    auto f = getUserDefaultsFile();
    f.getParentDirectory().createDirectory();
    f.replaceWithText(globalPrefsXml->toString());
}

void ClipperLimiterAudioProcessor::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&)
{
    // Disabled: No longer auto-saving parameters to global file
    // Parameters are managed by DAW via setStateInformation() / getStateInformation()
    // User defaults are loaded once at prepareToPlay() if DAW hasn't provided state
}

void ClipperLimiterAudioProcessor::timerCallback()
{
    // Disabled: Parameter auto-save mechanism removed
    // No longer need to save parameter changes to disk automatically
}

juce::AudioProcessorValueTreeState::ParameterLayout ClipperLimiterAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("in_gain", "Input Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

    // Clipper Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>("drive", "Input Drive", 0.0f, 18.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("knee", "Knee", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("clipper_mode", "Clipper Mode", juce::StringArray { "Soft", "Hard" }, 0));
    
    // Limiter Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>("limiter_drive", "Limiter Drive", 0.0f, 18.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ceiling", "Ceiling", -1.0f, 0.0f, -0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("character", "Character", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("out_gain", "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 1.0f, 0.01f), -0.1f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("true_peak", "True Peak", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>("delta", "Delta", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("limiter_mode", "Limiter Mode", juce::StringArray { "Classic", "Modern" }, 1));
    
    // Oversampling: Off, 2x, 4x, 8x, 16x
    juce::StringArray oversamplingChoices;
    oversamplingChoices.add("Off");
    oversamplingChoices.add("2x");
    oversamplingChoices.add("4x");
    oversamplingChoices.add("8x");
    oversamplingChoices.add("16x");
    params.push_back(std::make_unique<juce::AudioParameterChoice>("oversampling", "Oversampling", oversamplingChoices, 2));

    // Lookahead
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lookahead", "Lookahead", juce::NormalisableRange<float>(0.0f, 5.0f, 0.01f), 2.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_hold_ms", "Modern Hold", juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_hold_ms_link", "Link Modern Hold", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_hold_release_ms", "Modern Hold Release", juce::NormalisableRange<float>(0.05f, 10.0f, 0.01f), 1.5f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_hold_release_ms_link", "Link Modern Hold Release", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_attack_tau_div", "Modern Attack Tau Div", juce::NormalisableRange<float>(0.25f, 8.0f, 0.01f), 2.5f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_attack_tau_div_link", "Link Modern Attack Tau Div", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_release_smooth_base_ms", "Modern Release Smooth Base", juce::NormalisableRange<float>(0.0f, 20.0f, 0.01f), 1.5f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_release_smooth_base_ms_link", "Link Modern Release Smooth Base", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_release_smooth_range_ms", "Modern Release Smooth Range", juce::NormalisableRange<float>(0.0f, 50.0f, 0.01f), 6.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_release_smooth_range_ms_link", "Link Modern Release Smooth Range", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_adapt_fast_strength", "Modern Adapt Fast Strength", juce::NormalisableRange<float>(0.0f, 8.0f, 0.01f), 1.5f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_adapt_fast_strength_link", "Link Modern Adapt Fast Strength", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_adapt_slow_strength", "Modern Adapt Slow Strength", juce::NormalisableRange<float>(0.0f, 16.0f, 0.01f), 3.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_adapt_slow_strength_link", "Link Modern Adapt Slow Strength", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_sc_hpf_hz", "Modern SC HPF", juce::NormalisableRange<float>(20.0f, 400.0f, 1.0f), 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_sc_hpf_hz_link", "Link Modern SC HPF", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_ratio_base", "Modern Ratio Base", juce::NormalisableRange<float>(0.1f, 3.0f, 0.01f), 1.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_ratio_slope", "Modern Ratio Slope", juce::NormalisableRange<float>(0.0f, 1.5f, 0.01f), 0.9f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_transient_mix", "Modern Transient Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_transient_mix_link", "Link Modern Transient Mix", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_soft_safety_strength", "Modern Soft Safety Strength", juce::NormalisableRange<float>(0.0f, 20.0f, 0.01f), 6.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_release_fast_ms", "Modern Release Fast", juce::NormalisableRange<float>(0.05f, 20.0f, 0.01f), 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_release_fast_ms_link", "Link Modern Release Fast", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_release_slow_ms", "Modern Release Slow", juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f), 30.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_release_slow_ms_link", "Link Modern Release Slow", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_link_transients", "Modern Link Transients", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_link_transients_link", "Link Modern Link Transients", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("modern_link_release", "Modern Link Release", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.95f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("modern_link_release_link", "Link Modern Link Release", true));

    // Visualization parameters (global preferences - not affected by DAW project)
    params.push_back(std::make_unique<juce::AudioParameterChoice>("viz_ticks", "Viz Ticks",
        juce::StringArray { "Off", "6dB", "3dB" }, 2));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("viz_range", "Viz Range",
        juce::StringArray { "12dB", "18dB", "30dB" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("viz_speed", "Viz Speed",
        juce::StringArray { "Fast", "Med", "Slow" }, 1));

    // UI Scale parameter (global preference - not affected by DAW project)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ui_scale", "UI Scale", juce::NormalisableRange<float>(50.0f, 200.0f, 1.0f), 100.0f));

    return { params.begin(), params.end() };
}

const juce::String ClipperLimiterAudioProcessor::getName() const
{
    return "Lim.one";
}

bool ClipperLimiterAudioProcessor::acceptsMidi() const
{
    return false;
}

bool ClipperLimiterAudioProcessor::producesMidi() const
{
    return false;
}

bool ClipperLimiterAudioProcessor::isMidiEffect() const
{
    return false;
}

double ClipperLimiterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ClipperLimiterAudioProcessor::getNumPrograms()
{
    return 1;
}

int ClipperLimiterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ClipperLimiterAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String ClipperLimiterAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void ClipperLimiterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

int ClipperLimiterAudioProcessor::computeOversamplingLatencySamples() const
{
    float latency = 0.0f;
    if (currentClipperOversamplingIndex > 0)
        latency += oversamplers[currentClipperOversamplingIndex - 1]->getLatencyInSamples();
    if (currentLimiterOversamplingIndex > 0 && currentLimiterOversamplingIndex != currentClipperOversamplingIndex)
        latency += oversamplers[currentLimiterOversamplingIndex - 1]->getLatencyInSamples();
    return (int) std::lround(latency);
}

int ClipperLimiterAudioProcessor::computeCurrentPathLatencySamples(bool clipperBypassed) const
{
    float latency = 0.0f;
    if (currentLimiterOversamplingIndex >= 2)
    {
        latency += oversamplers[currentLimiterOversamplingIndex - 1]->getLatencyInSamples();
    }
    else
    {
        if (! clipperBypassed && currentClipperOversamplingIndex > 0)
            latency += oversamplers[currentClipperOversamplingIndex - 1]->getLatencyInSamples();

        if (currentLimiterOversamplingIndex > 0)
            latency += oversamplers[currentLimiterOversamplingIndex - 1]->getLatencyInSamples();
    }
    return (int) std::lround(latency);
}

int ClipperLimiterAudioProcessor::computeMaxOversamplingLatencySamples() const
{
    int maxLatency = 0;
    for (int i = 0; i < 4; ++i)
        maxLatency = std::max(maxLatency, (int) std::lround(oversamplers[i]->getLatencyInSamples()));

    const int latency2x = (int) std::lround(oversamplers[0]->getLatencyInSamples());
    const int latency4x = (int) std::lround(oversamplers[1]->getLatencyInSamples());
    maxLatency = std::max(maxLatency, latency4x + latency2x);
    return maxLatency;
}

int ClipperLimiterAudioProcessor::computeTotalLatencySamples(float lookaheadMs) const
{
    const float clampedMs = std::max(0.0f, lookaheadMs);
    const int lookaheadSamples = (int) std::lround(clampedMs * 0.001f * (float) currentSampleRate);
    return maxOversamplingLatencySamples + lookaheadSamples;
}

void ClipperLimiterAudioProcessor::updateLatencySamples(float lookaheadMs)
{
    const int total = computeTotalLatencySamples(lookaheadMs);
    if (total != lastReportedLatencySamples)
    {
        setLatencySamples(total);
        lastReportedLatencySamples = total;
    }
}

void ClipperLimiterAudioProcessor::applyClipperLatencyDelay(juce::AudioBuffer<float>& buffer, int delaySamples)
{
    if (delaySamples <= 0 || clipperLatencyDelayBufferSize <= 1)
        return;

    if (delaySamples > clipperLatencyDelayBufferSize - 1)
        delaySamples = clipperLatencyDelayBufferSize - 1;

    if (delaySamples != clipperLatencyDelaySamples)
        clipperLatencyDelaySamples = delaySamples;

    auto* pL = buffer.getWritePointer(0);
    auto* pR = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : nullptr;
    const int numSamples = buffer.getNumSamples();
    const int size = clipperLatencyDelayBufferSize;

    for (int i = 0; i < numSamples; ++i)
    {
        clipperLatencyDelayL[(size_t) clipperLatencyDelayWritePos] = pL[i];
        int readPos = clipperLatencyDelayWritePos - delaySamples;
        if (readPos < 0) readPos += size;
        pL[i] = clipperLatencyDelayL[(size_t) readPos];

        if (pR != nullptr)
        {
            clipperLatencyDelayR[(size_t) clipperLatencyDelayWritePos] = pR[i];
            pR[i] = clipperLatencyDelayR[(size_t) readPos];
        }

        if (++clipperLatencyDelayWritePos >= size)
            clipperLatencyDelayWritePos = 0;
    }
}

void ClipperLimiterAudioProcessor::applyDeltaDelay(juce::AudioBuffer<float>& buffer, int delaySamples)
{
    if (delaySamples <= 0 || deltaDelayBufferSize <= 1)
        return;

    if (delaySamples > deltaDelayBufferSize - 1)
        delaySamples = deltaDelayBufferSize - 1;

    if (delaySamples != deltaDelaySamples)
        deltaDelaySamples = delaySamples;

    auto* pL = buffer.getWritePointer(0);
    auto* pR = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : nullptr;
    const int numSamples = buffer.getNumSamples();
    const int size = deltaDelayBufferSize;

    for (int i = 0; i < numSamples; ++i)
    {
        deltaDelayL[(size_t) deltaDelayWritePos] = pL[i];
        int readPos = deltaDelayWritePos - delaySamples;
        if (readPos < 0) readPos += size;
        pL[i] = deltaDelayL[(size_t) readPos];

        if (pR != nullptr)
        {
            deltaDelayR[(size_t) deltaDelayWritePos] = pR[i];
            pR[i] = deltaDelayR[(size_t) readPos];
        }

        if (++deltaDelayWritePos >= size)
            deltaDelayWritePos = 0;
    }
}

void ClipperLimiterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Lazy-load user defaults if DAW hasn't provided project state
    loadUserDefaultsIfNeeded();

    clipperModule.prepare(sampleRate);
    limiterModule.prepare(sampleRate);

    vizFifo.reset();
    vizInPeak = 0.0f;
    vizOutPeak = 0.0f;
    vizSampleCursor = 0;
    
    // Initialize oversamplers for factors 1, 2, 3, 4 (2x, 4x, 8x, 16x)
    for (int i = 0; i < 4; ++i)
    {
        size_t factor = (size_t)(i + 1);
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>>(2, factor, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, false);
        oversamplers[i]->initProcessing((size_t) samplesPerBlock);
        oversamplers[i]->reset();
    }

    tpSafetyOversampler = std::make_unique<juce::dsp::Oversampling<float>>(2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, false);
    tpSafetyOversampler->initProcessing((size_t) samplesPerBlock);
    tpSafetyOversampler->reset();
    tpSafetyGain = 1.0f;
    
    const int userOversamplingIndex = (int) *apvts.getRawParameterValue("oversampling");

    currentLimiterOversamplingIndex = userOversamplingIndex;
    currentClipperOversamplingIndex = std::max(currentLimiterOversamplingIndex, 2);

    const double clipperFactor = std::pow(2.0, (double) currentClipperOversamplingIndex);
    const double limiterFactor = (currentLimiterOversamplingIndex > 0)
        ? std::pow(2.0, (double) currentLimiterOversamplingIndex)
        : 1.0;

    clipperModule.prepare(currentSampleRate * clipperFactor);
    limiterModule.prepare(currentSampleRate * limiterFactor);

    if (currentClipperOversamplingIndex > 0)
        oversamplers[currentClipperOversamplingIndex - 1]->reset();
    if (currentLimiterOversamplingIndex > 0 && currentLimiterOversamplingIndex != currentClipperOversamplingIndex)
        oversamplers[currentLimiterOversamplingIndex - 1]->reset();

    maxOversamplingLatencySamples = computeMaxOversamplingLatencySamples();
    clipperLatencyDelayBufferSize = std::max(1, maxOversamplingLatencySamples + 1);
    clipperLatencyDelayL.assign((size_t) clipperLatencyDelayBufferSize, 0.0f);
    clipperLatencyDelayR.assign((size_t) clipperLatencyDelayBufferSize, 0.0f);
    clipperLatencyDelayWritePos = 0;
    clipperLatencyDelaySamples = 0;

    maxLookaheadSamples = (int) std::ceil(0.005 * currentSampleRate);
    deltaDelayBufferSize = std::max(1, maxOversamplingLatencySamples + maxLookaheadSamples + 1);
    deltaDelayL.assign((size_t) deltaDelayBufferSize, 0.0f);
    deltaDelayR.assign((size_t) deltaDelayBufferSize, 0.0f);
    deltaDelayWritePos = 0;
    deltaDelaySamples = 0;

    deltaBuffer.setSize(2, samplesPerBlock, false, false, true);

    updateLatencySamples(*apvts.getRawParameterValue("lookahead"));
    
    // LUFS Meter
    lufsMeter.prepareToPlay(sampleRate, getTotalNumOutputChannels(), samplesPerBlock, 20);

}

void ClipperLimiterAudioProcessor::releaseResources()
{
}

bool ClipperLimiterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void ClipperLimiterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);
    lastProcessTime = juce::Time::getMillisecondCounter();
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Get Parameters
    float inGainDb = *apvts.getRawParameterValue ("in_gain");
    float clipperDrive = *apvts.getRawParameterValue("drive");
    float knee = *apvts.getRawParameterValue("knee");
    float limiterDrive = *apvts.getRawParameterValue("limiter_drive");
    float ceiling = *apvts.getRawParameterValue("ceiling");
    float character = *apvts.getRawParameterValue("character");
    float outGainDb = *apvts.getRawParameterValue ("out_gain");
    // const int uiMode = (int)*apvts.getRawParameterValue("ui_mode"); // Removed
    bool truePeak = *apvts.getRawParameterValue("true_peak") > 0.5f;
    bool deltaEnabled = *apvts.getRawParameterValue("delta") > 0.5f;
    int mode = (int)*apvts.getRawParameterValue("limiter_mode");
    int clipperMode = (int)*apvts.getRawParameterValue("clipper_mode");
    float lookaheadMs = *apvts.getRawParameterValue("lookahead");

    float modernHoldMs = *apvts.getRawParameterValue("modern_hold_ms");
    float modernHoldReleaseMs = *apvts.getRawParameterValue("modern_hold_release_ms");
    float modernAttackTauDiv = *apvts.getRawParameterValue("modern_attack_tau_div");
    float modernReleaseSmoothBaseMs = *apvts.getRawParameterValue("modern_release_smooth_base_ms");
    float modernReleaseSmoothRangeMs = *apvts.getRawParameterValue("modern_release_smooth_range_ms");
    float modernAdaptFastStrength = *apvts.getRawParameterValue("modern_adapt_fast_strength");
    float modernAdaptSlowStrength = *apvts.getRawParameterValue("modern_adapt_slow_strength");
    float modernScHpfHz = *apvts.getRawParameterValue("modern_sc_hpf_hz");
    float modernRatioBase = *apvts.getRawParameterValue("modern_ratio_base");
    float modernRatioSlope = *apvts.getRawParameterValue("modern_ratio_slope");
    float modernTransientMix = *apvts.getRawParameterValue("modern_transient_mix");
    float modernSoftSafetyStrength = *apvts.getRawParameterValue("modern_soft_safety_strength");
    float modernReleaseFastMs = *apvts.getRawParameterValue("modern_release_fast_ms");
    float modernReleaseSlowMs = *apvts.getRawParameterValue("modern_release_slow_ms");
    float modernLinkTransients = *apvts.getRawParameterValue("modern_link_transients");
    float modernLinkRelease = *apvts.getRawParameterValue("modern_link_release");

    const bool clipperBypassed = (std::abs(clipperDrive) < 1.0e-6f && knee < 1.0e-6f);

    // Oversampling Logic
    // Choice: 0=Off, 1=2x, 2=4x, 3=8x, 4=16x
    // Policy:
    // - Clipper always runs at min 4x (index >= 2)
    // - Limiter runs at the user-selected rate (including Off / 2x)
    const int requestedLimiterOversamplingIndex = (int) *apvts.getRawParameterValue("oversampling");
    const int newLimiterOversamplingIndex = requestedLimiterOversamplingIndex;
    const int newClipperOversamplingIndex = std::max(newLimiterOversamplingIndex, 2);

    if (newLimiterOversamplingIndex != currentLimiterOversamplingIndex
        || newClipperOversamplingIndex != currentClipperOversamplingIndex)
    {
        currentLimiterOversamplingIndex = newLimiterOversamplingIndex;
        currentClipperOversamplingIndex = newClipperOversamplingIndex;

        const double clipperFactor = std::pow(2.0, (double) currentClipperOversamplingIndex);
        const double limiterFactor = (currentLimiterOversamplingIndex > 0)
            ? std::pow(2.0, (double) currentLimiterOversamplingIndex)
            : 1.0;

        clipperModule.prepare(currentSampleRate * clipperFactor);
        limiterModule.prepare(currentSampleRate * limiterFactor);

        if (currentClipperOversamplingIndex > 0)
            oversamplers[currentClipperOversamplingIndex - 1]->reset();
        if (currentLimiterOversamplingIndex > 0 && currentLimiterOversamplingIndex != currentClipperOversamplingIndex)
            oversamplers[currentLimiterOversamplingIndex - 1]->reset();
    }

    updateLatencySamples(lookaheadMs);
    const int currentPathLatencySamples = computeCurrentPathLatencySamples(clipperBypassed);
    int extraDelaySamples = maxOversamplingLatencySamples - currentPathLatencySamples;
    if (extraDelaySamples < 0)
        extraDelaySamples = 0;

    const float inGainLin = juce::Decibels::decibelsToGain (inGainDb);
    const float outGainLin = juce::Decibels::decibelsToGain (outGainDb);
    bool transportPlaying = true;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
            transportPlaying = pos->getIsPlaying();
    }

    if (inGainLin != 1.0f)
        buffer.applyGain (inGainLin);

    if (deltaEnabled)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        for (int ch = 0; ch < numChannels; ++ch)
            deltaBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    const auto linToDb = [](float x) {
        if (x <= 1.0e-6f) return -100.0f;
        const float db = 20.0f * std::log10(x);
        return std::max(-100.0f, std::min(db, 0.0f));
    };

    constexpr int maxVizPointsPerBlock = 512;
    std::array<float, maxVizPointsPerBlock> vizInTmp {};
    std::array<float, maxVizPointsPerBlock> vizOutTmp {};
    int vizInCount = 0;
    int vizOutCount = 0;

    const int hop = std::max(1, vizHopSamples);
    const int64_t cursorStart = vizSampleCursor;

    if (transportPlaying)
    {
        const auto* inL = buffer.getReadPointer(0);
        const auto* inR = (totalNumInputChannels > 1) ? buffer.getReadPointer(1) : nullptr;
        float peak = vizInPeak;

        const int numSamples = buffer.getNumSamples();
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float L = inL[sample];
            const float R = (inR != nullptr) ? inR[sample] : L;
            const float v = std::max(std::abs(L), std::abs(R));
            if (v > peak) peak = v;

            const int64_t pos = cursorStart + (int64_t) sample + 1;
            const bool endOfHop = (pos % hop) == 0;
            if (endOfHop)
            {
                if (vizInCount < maxVizPointsPerBlock)
                    vizInTmp[(size_t) vizInCount++] = linToDb(peak);
                peak = 0.0f;
            }
        }

        vizInPeak = peak;
    }
    
    // Update DSP Modules
    clipperModule.setAlgorithm(clipperMode);
    clipperModule.setParameters(clipperDrive, knee);

    limiterModule.setAdvancedParameters(modernHoldMs,
        modernHoldReleaseMs,
        modernAttackTauDiv,
        modernReleaseSmoothBaseMs,
        modernReleaseSmoothRangeMs,
        modernAdaptFastStrength,
        modernAdaptSlowStrength,
        modernScHpfHz,
        modernRatioBase,
        modernRatioSlope,
        modernTransientMix,
        modernSoftSafetyStrength,
        modernReleaseFastMs,
        modernReleaseSlowMs,
        modernLinkTransients,
        modernLinkRelease);

    limiterModule.setParameters(limiterDrive, clipperDrive, ceiling, character, mode, truePeak, lookaheadMs);
    
    // Process Audio
    juce::dsp::AudioBlock<float> block(buffer);

    float maxL = 0.0f;
    float maxR = 0.0f;
    float maxClipperGR = 0.0f;
    float maxLimiterGR = 0.0f;
    float maxClipperGR_L = 0.0f;
    float maxClipperGR_R = 0.0f;
    float maxLimiterGR_L = 0.0f;
    float maxLimiterGR_R = 0.0f;

    if (currentLimiterOversamplingIndex >= 2)
    {
        auto processBlock = oversamplers[currentLimiterOversamplingIndex - 1]->processSamplesUp(block);
        auto* pL = processBlock.getChannelPointer(0);
        auto* pR = (processBlock.getNumChannels() > 1) ? processBlock.getChannelPointer(1) : nullptr;

        for (size_t sample = 0; sample < processBlock.getNumSamples(); ++sample)
        {
            float L = pL[sample];
            float R = (pR != nullptr) ? pR[sample] : L;

            if (!clipperBypassed)
            {
                clipperModule.process(L, R);

                float cGR = clipperModule.getGainReduction();
                if (cGR > maxClipperGR) maxClipperGR = cGR;

                float cGRL = clipperModule.getGainReductionLeft();
                float cGRR = clipperModule.getGainReductionRight();
                if (cGRL > maxClipperGR_L) maxClipperGR_L = cGRL;
                if (cGRR > maxClipperGR_R) maxClipperGR_R = cGRR;
            }

            limiterModule.process(L, R);

            float lGR = limiterModule.getGainReduction();
            if (lGR > maxLimiterGR) maxLimiterGR = lGR;

            float lGRL = limiterModule.getGainReductionLeft();
            float lGRR = limiterModule.getGainReductionRight();
            if (lGRL > maxLimiterGR_L) maxLimiterGR_L = lGRL;
            if (lGRR > maxLimiterGR_R) maxLimiterGR_R = lGRR;

            pL[sample] = L;
            if (pR != nullptr) pR[sample] = R;
        }

        oversamplers[currentLimiterOversamplingIndex - 1]->processSamplesDown(block);
    }
    else
    {
        if (!clipperBypassed)
        {
            auto clipBlock = oversamplers[currentClipperOversamplingIndex - 1]->processSamplesUp(block);
            auto* pL = clipBlock.getChannelPointer(0);
            auto* pR = (clipBlock.getNumChannels() > 1) ? clipBlock.getChannelPointer(1) : nullptr;

            for (size_t sample = 0; sample < clipBlock.getNumSamples(); ++sample)
            {
                float L = pL[sample];
                float R = (pR != nullptr) ? pR[sample] : L;

                clipperModule.process(L, R);

                float cGR = clipperModule.getGainReduction();
                if (cGR > maxClipperGR) maxClipperGR = cGR;

                float cGRL = clipperModule.getGainReductionLeft();
                float cGRR = clipperModule.getGainReductionRight();
                if (cGRL > maxClipperGR_L) maxClipperGR_L = cGRL;
                if (cGRR > maxClipperGR_R) maxClipperGR_R = cGRR;

                pL[sample] = L;
                if (pR != nullptr) pR[sample] = R;
            }

            oversamplers[currentClipperOversamplingIndex - 1]->processSamplesDown(block);
        }

        if (currentLimiterOversamplingIndex == 0)
        {
            auto* pL = buffer.getWritePointer(0);
            auto* pR = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : nullptr;

            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                float L = pL[sample];
                float R = (pR != nullptr) ? pR[sample] : L;

                limiterModule.process(L, R);

                float lGR = limiterModule.getGainReduction();
                if (lGR > maxLimiterGR) maxLimiterGR = lGR;

                float lGRL = limiterModule.getGainReductionLeft();
                float lGRR = limiterModule.getGainReductionRight();
                if (lGRL > maxLimiterGR_L) maxLimiterGR_L = lGRL;
                if (lGRR > maxLimiterGR_R) maxLimiterGR_R = lGRR;

                pL[sample] = L;
                if (pR != nullptr) pR[sample] = R;
            }
        }
        else
        {
            auto limitBlock = oversamplers[currentLimiterOversamplingIndex - 1]->processSamplesUp(block);
            auto* pL = limitBlock.getChannelPointer(0);
            auto* pR = (limitBlock.getNumChannels() > 1) ? limitBlock.getChannelPointer(1) : nullptr;

            for (size_t sample = 0; sample < limitBlock.getNumSamples(); ++sample)
            {
                float L = pL[sample];
                float R = (pR != nullptr) ? pR[sample] : L;

                limiterModule.process(L, R);

                float lGR = limiterModule.getGainReduction();
                if (lGR > maxLimiterGR) maxLimiterGR = lGR;

                float lGRL = limiterModule.getGainReductionLeft();
                float lGRR = limiterModule.getGainReductionRight();
                if (lGRL > maxLimiterGR_L) maxLimiterGR_L = lGRL;
                if (lGRR > maxLimiterGR_R) maxLimiterGR_R = lGRR;

                pL[sample] = L;
                if (pR != nullptr) pR[sample] = R;
            }

            oversamplers[currentLimiterOversamplingIndex - 1]->processSamplesDown(block);
        }
    }

    const float oversamplingCompDb = 0.1f;
    const float oversamplingCompLin = juce::Decibels::decibelsToGain(oversamplingCompDb);
    if (oversamplingCompLin != 1.0f)
        buffer.applyGain(oversamplingCompLin);

    if (deltaEnabled)
    {
        const float clipperGainLin = std::pow(10.0f, clipperDrive / 20.0f);
        const float limiterGainLin = std::pow(10.0f, limiterDrive / 20.0f);
        const float totalGain = clipperGainLin * limiterGainLin;
        if (totalGain > 0.0f && std::abs(totalGain - 1.0f) > 1.0e-9f)
            buffer.applyGain(1.0f / totalGain);
    }

    applyClipperLatencyDelay(buffer, extraDelaySamples);

    if (deltaEnabled)
    {
        const int lookaheadSamples = (int) std::lround(std::max(0.0f, lookaheadMs) * 0.001f * (float) currentSampleRate);
        const int totalDelaySamples = maxOversamplingLatencySamples + lookaheadSamples;
        applyDeltaDelay(deltaBuffer, totalDelaySamples);

        auto* deltaL = deltaBuffer.getReadPointer(0);
        auto* deltaR = (deltaBuffer.getNumChannels() > 1) ? deltaBuffer.getReadPointer(1) : nullptr;
        auto* pL = buffer.getWritePointer(0);
        auto* pR = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : nullptr;
        const int numSamples = buffer.getNumSamples();
        for (int i = 0; i < numSamples; ++i)
        {
            pL[i] -= deltaL[i];
            if (pR != nullptr)
                pR[i] -= (deltaR != nullptr) ? deltaR[i] : deltaL[i];
        }
    }

    if (outGainLin != 1.0f)
        buffer.applyGain (outGainLin);

    if (truePeak && currentLimiterOversamplingIndex < 2 && tpSafetyOversampler)
    {
        auto safetyBlock = tpSafetyOversampler->processSamplesUp(block);
        auto* pL = safetyBlock.getChannelPointer(0);
        auto* pR = (safetyBlock.getNumChannels() > 1) ? safetyBlock.getChannelPointer(1) : nullptr;

        float peak = 0.0f;
        for (size_t sample = 0; sample < safetyBlock.getNumSamples(); ++sample)
        {
            const float L = std::abs(pL[sample]);
            const float R = (pR != nullptr) ? std::abs(pR[sample]) : L;
            const float v = std::max(L, R);
            if (v > peak) peak = v;
        }

        const float ceilingLin = juce::Decibels::decibelsToGain(ceiling);
        const float target = (peak > ceilingLin) ? (ceilingLin / peak) : 1.0f;

        if (target < tpSafetyGain)
        {
            tpSafetyGain = target;
        }
        else
        {
            const float releaseMs = 10.0f;
            const float releaseCoeff = 1.0f - std::exp(-(float) buffer.getNumSamples() / ((releaseMs * 0.001f) * (float) currentSampleRate));
            tpSafetyGain += releaseCoeff * (target - tpSafetyGain);
        }

        if (tpSafetyGain < 1.0f)
            buffer.applyGain(tpSafetyGain);
    }
    else
    {
        tpSafetyGain = 1.0f;
    }

    auto* channelLeft = buffer.getWritePointer(0);
    auto* channelRight = (totalNumInputChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    // 3. Final Metering
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float L = channelLeft[sample];
        float R = (channelRight != nullptr) ? channelRight[sample] : L;
        if (std::abs(L) > maxL) maxL = std::abs(L);
        if (std::abs(R) > maxR) maxR = std::abs(R);
    }
    
    const float prevOutL = outputMeterLeft.load();
    const float prevOutR = outputMeterRight.load();
    if (maxL > prevOutL) outputMeterLeft.store(maxL);
    if (maxR > prevOutR) outputMeterRight.store(maxR);
    
    if (transportPlaying)
    {
        float peak = vizOutPeak;
        const int numSamples = buffer.getNumSamples();
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float outL0 = channelLeft[sample];
            const float outR0 = (channelRight != nullptr) ? channelRight[sample] : outL0;
            const float v = std::max(std::abs(outL0), std::abs(outR0));
            if (v > peak) peak = v;

            const int64_t pos = cursorStart + (int64_t) sample + 1;
            const bool endOfHop = (pos % hop) == 0;
            if (endOfHop)
            {
                if (vizOutCount < maxVizPointsPerBlock)
                    vizOutTmp[(size_t) vizOutCount++] = linToDb(peak);
                peak = 0.0f;
            }
        }

        vizOutPeak = peak;

        const int toWrite = std::min(vizInCount, vizOutCount);
        if (toWrite > 0)
        {
            int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
            vizFifo.prepareToWrite(toWrite, start1, size1, start2, size2);

            for (int i = 0; i < size1; ++i)
            {
                vizInDb[(size_t) (start1 + i)] = vizInTmp[(size_t) i];
                vizOutDb[(size_t) (start1 + i)] = vizOutTmp[(size_t) i];
            }
            for (int i = 0; i < size2; ++i)
            {
                vizInDb[(size_t) (start2 + i)] = vizInTmp[(size_t) (size1 + i)];
                vizOutDb[(size_t) (start2 + i)] = vizOutTmp[(size_t) (size1 + i)];
            }

            vizFifo.finishedWrite(size1 + size2);
        }
    }

    vizSampleCursor += buffer.getNumSamples();
    
    // LUFS Meter
    lufsMeter.processBlock(buffer);

    // Convert Linear Reduction (0.0 - 1.0) to dB (0.0 to -inf)
    // GR_linear = 1.0 - (out/in)
    // out/in = 1.0 - GR_linear
    
    auto grLinearToDb = [](float grLinear) {
        if (grLinear <= 0.0001f) return 0.0f;
        float ratio = 1.0f - grLinear;
        if (ratio < 0.0001f) ratio = 0.0001f;
        return 20.0f * std::log10(ratio);
    };

    float cGR_dB = grLinearToDb(maxClipperGR);
    float lGR_dB = grLinearToDb(maxLimiterGR);
    float cGRL_dB = grLinearToDb(maxClipperGR_L);
    float cGRR_dB = grLinearToDb(maxClipperGR_R);
    float lGRL_dB = grLinearToDb(maxLimiterGR_L);
    float lGRR_dB = grLinearToDb(maxLimiterGR_R);

    const bool hasRight = (channelRight != nullptr);
    if (!hasRight)
    {
        cGRR_dB = cGRL_dB;
        lGRR_dB = lGRL_dB;
    }

    cGR_dB = std::min(cGRL_dB, cGRR_dB);
    lGR_dB = std::min(lGRL_dB, lGRR_dB);

    // Atomic update: Keep the minimum (most negative) value since last read
    // This ensures fast transients are captured by the UI polling
    float currentClipper = clipperGR.load();
    while (cGR_dB < currentClipper && !clipperGR.compare_exchange_weak(currentClipper, cGR_dB));
    
    float currentLimiter = limiterGR.load();
    while (lGR_dB < currentLimiter && !limiterGR.compare_exchange_weak(currentLimiter, lGR_dB));

    float currentClipperL = clipperGRLeft.load();
    while (cGRL_dB < currentClipperL && !clipperGRLeft.compare_exchange_weak(currentClipperL, cGRL_dB));

    float currentClipperR = clipperGRRight.load();
    while (cGRR_dB < currentClipperR && !clipperGRRight.compare_exchange_weak(currentClipperR, cGRR_dB));

    float currentLimiterL = limiterGRLeft.load();
    while (lGRL_dB < currentLimiterL && !limiterGRLeft.compare_exchange_weak(currentLimiterL, lGRL_dB));

    float currentLimiterR = limiterGRRight.load();
    while (lGRR_dB < currentLimiterR && !limiterGRRight.compare_exchange_weak(currentLimiterR, lGRR_dB));
}

int ClipperLimiterAudioProcessor::readVizPoints(juce::Array<float>& inDb, juce::Array<float>& outDb, int maxPoints)
{
    const int avail = vizFifo.getNumReady();
    const int toRead = std::min(avail, std::max(0, maxPoints));
    if (toRead <= 0) return 0;

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    vizFifo.prepareToRead(toRead, start1, size1, start2, size2);

    inDb.ensureStorageAllocated(inDb.size() + size1 + size2);
    outDb.ensureStorageAllocated(outDb.size() + size1 + size2);

    for (int i = 0; i < size1; ++i)
    {
        inDb.add(vizInDb[(size_t) (start1 + i)]);
        outDb.add(vizOutDb[(size_t) (start1 + i)]);
    }
    for (int i = 0; i < size2; ++i)
    {
        inDb.add(vizInDb[(size_t) (start2 + i)]);
        outDb.add(vizOutDb[(size_t) (start2 + i)]);
    }

    vizFifo.finishedRead(size1 + size2);
    return size1 + size2;
}

bool ClipperLimiterAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ClipperLimiterAudioProcessor::createEditor()
{
    return new ClipperLimiterAudioProcessorEditor (*this);
}

void ClipperLimiterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ClipperLimiterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Mark that we received initial state from DAW
    hasReceivedInitialState.store(true);

    isLoadingPreset.store(true);

    // 1. Save current Link states
    std::map<juce::String, float> linkStates;
    for (const auto& id : linkedParams) {
        auto* p = apvts.getParameter(id + "_link");
        if (p) linkStates[id] = p->getValue();
    }

    // 2. Save current global preference parameters (ui_scale, viz_ticks, viz_range, viz_speed)
    // These should not be overwritten by DAW project state
    std::map<juce::String, float> globalParamValues;
    const std::vector<juce::String> globalParams = {
        "ui_scale", "viz_ticks", "viz_range", "viz_speed"
    };
    for (const auto& id : globalParams) {
        if (auto* p = apvts.getParameter(id))
            globalParamValues[id] = p->getValue();
    }

    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xmlState);

            std::function<void(juce::ValueTree&)> migrate = [&] (juce::ValueTree& t)
            {
                if (t.hasProperty("transients") && !t.hasProperty("character"))
                    t.setProperty("character", t.getProperty("transients"), nullptr);

                auto oldChild = t.getChildWithName(juce::Identifier("transients"));
                auto newChild = t.getChildWithName(juce::Identifier("character"));
                if (oldChild.isValid() && !newChild.isValid())
                {
                    juce::ValueTree copied(juce::Identifier("character"));
                    copied.copyPropertiesFrom(oldChild, nullptr);
                    for (int i = 0; i < oldChild.getNumChildren(); ++i)
                        copied.addChild(oldChild.getChild(i).createCopy(), -1, nullptr);
                    t.addChild(copied, -1, nullptr);
                }

                for (int i = 0; i < t.getNumChildren(); ++i)
                {
                    auto c = t.getChild(i);
                    migrate(c);
                }
            };

            if (state.isValid())
                migrate(state);

            apvts.replaceState(state);
        }

    // Restore Link states
    for (const auto& pair : linkStates) {
        auto* p = apvts.getParameter(pair.first + "_link");
        if (p) p->setValueNotifyingHost(pair.second);
    }

    // Restore global preference parameters (ui_scale, viz_ticks, viz_range, viz_speed)
    // These parameters should not be changed by DAW project loading
    const juce::ScopedValueSetter<bool> scoped(suppressUserDefaultsListener, true);
    for (const auto& [id, value] : globalParamValues) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(value);
    }

    // Recalculate Linked parameters based on new Character
    float character = *apvts.getRawParameterValue("character");
    isUpdatingFromCharacter.store(true); 
    for (const auto& id : linkedParams) {
        auto* linkParam = apvts.getParameter(id + "_link");
        if (linkParam && linkParam->getValue() > 0.5f) {
             auto curve = LimiterCurves::getCurve(id);
             if (curve.has_value()) {
                 float val = LimiterCurves::calculate(character, *curve);
                 auto* p = apvts.getParameter(id);
                 if (p) {
                     auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p);
                     if (rp) {
                         rp->setValueNotifyingHost(rp->convertTo0to1(val));
                     }
                 }
             }
        }
    }
    isUpdatingFromCharacter.store(false);

    isLoadingPreset.store(false);
}

void ClipperLimiterAudioProcessor::beginGesture(const juce::String& parameterID)
{
    const juce::ScopedLock sl(gestureLock);
    activeGestures.insert(parameterID);
}

void ClipperLimiterAudioProcessor::endGesture(const juce::String& parameterID)
{
    const juce::ScopedLock sl(gestureLock);
    activeGestures.erase(parameterID);
}

bool ClipperLimiterAudioProcessor::isParameterGesturing(const juce::String& parameterID) const
{
    const juce::ScopedLock sl(const_cast<juce::CriticalSection&>(gestureLock));
    return activeGestures.find(parameterID) != activeGestures.end();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClipperLimiterAudioProcessor();
}

void ClipperLimiterAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "character")
    {
        for (const auto& id : linkedParams)
        {
            auto* linkParam = apvts.getParameter(id + "_link");
            if (linkParam && linkParam->getValue() > 0.5f)
            {
                auto curve = LimiterCurves::getCurve(id);
                if (curve.has_value())
                {
                    float val = LimiterCurves::calculate(newValue, *curve);
                    auto* p = apvts.getParameter(id);
                    if (p)
                    {
                        auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p);
                        if (rp)
                        {
                            float norm = rp->convertTo0to1(val);
                            rp->setValueNotifyingHost(norm);
                        }
                    }
                }
            }
        }
    }
    else if (parameterID.endsWith("_link"))
    {
        if (newValue > 0.5f)
        {
             juce::String targetID = parameterID.dropLastCharacters(5);
             auto curve = LimiterCurves::getCurve(targetID);
             if (curve.has_value())
             {
                 float character = *apvts.getRawParameterValue("character");
                 float val = LimiterCurves::calculate(character, *curve);
                 
                 auto* p = apvts.getParameter(targetID);
                 if (p)
                 {
                      auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p);
                      if (rp) rp->setValueNotifyingHost(rp->convertTo0to1(val));
                 }
             }
        }
    }
    else if (parameterID.startsWith("modern_"))
    {
        if (isLoadingPreset.load()) return;

        // Only unlink if the user is actively gesturing (dragging) this parameter
        if (!isParameterGesturing(parameterID)) return;

        auto* linkParam = apvts.getParameter(parameterID + "_link");
        if (linkParam && linkParam->getValue() > 0.5f)
        {
            linkParam->setValueNotifyingHost(0.0f);
        }
    }

    // Save global preference parameters if any of them changed
    const std::vector<juce::String> globalParams = {
        "ui_scale", "viz_ticks", "viz_range", "viz_speed"
    };
    if (std::find(globalParams.begin(), globalParams.end(), parameterID) != globalParams.end())
    {
        saveGlobalPreferences();
    }
}

void ClipperLimiterAudioProcessor::resetLinks()
{
    float character = *apvts.getRawParameterValue("character");
    
    for (const auto& id : linkedParams)
    {
         auto* linkParam = apvts.getParameter(id + "_link");
         if (linkParam) linkParam->setValueNotifyingHost(1.0f);
         
         auto curve = LimiterCurves::getCurve(id);
         if (curve.has_value())
         {
             float val = LimiterCurves::calculate(character, *curve);
             auto* p = apvts.getParameter(id);
             if (p)
             {
                 auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p);
                 if (rp)
                 {
                     float norm = rp->convertTo0to1(val);
                     rp->setValueNotifyingHost(norm);
                 }
             }
         }
    }
}
