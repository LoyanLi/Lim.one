#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include "Clipper.h"
#include "Limiter.h"
#include "Ebu128LoudnessMeter.h"

class ClipperLimiterAudioProcessor  : public juce::AudioProcessor,
                                      private juce::ValueTree::Listener,
                                      public juce::AudioProcessorValueTreeState::Listener,
                                      private juce::Timer
{
public:
    ClipperLimiterAudioProcessor();
    ~ClipperLimiterAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // APVTS
    juce::AudioProcessorValueTreeState apvts;
    
    // Real-time metering data
    std::atomic<float> outputMeterLeft { 0.0f };
    std::atomic<float> outputMeterRight { 0.0f };
    std::atomic<float> clipperGR { 0.0f };
    std::atomic<float> limiterGR { 0.0f };
    std::atomic<float> clipperGRLeft { 0.0f };
    std::atomic<float> clipperGRRight { 0.0f };
    std::atomic<float> limiterGRLeft { 0.0f };
    std::atomic<float> limiterGRRight { 0.0f };
    
    // Timestamp for idle detection (DAW pause)
    std::atomic<int64_t> lastProcessTime { 0 };
    
    // LUFS Meter
    Ebu128LoudnessMeter lufsMeter;

    int readVizPoints(juce::Array<float>& inDb, juce::Array<float>& outDb, int maxPoints);
    int getVizHopSamples() const noexcept { return vizHopSamples; }

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void resetLinks();

    // Gesture Handling
    void beginGesture(const juce::String& parameterID);
    void endGesture(const juce::String& parameterID);
    bool isParameterGesturing(const juce::String& parameterID) const;

private:
    std::atomic<bool> isUpdatingFromCharacter { false };
    std::atomic<bool> isLoadingPreset { false };

    // Gesture state
    juce::CriticalSection gestureLock;
    std::set<juce::String> activeGestures;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::File getUserDefaultsFile() const;
    void loadUserDefaultsFromDisk();
    void saveUserDefaultsToDisk();

    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void timerCallback() override;

    int computeOversamplingLatencySamples() const;
    int computeCurrentPathLatencySamples(bool clipperBypassed) const;
    int computeMaxOversamplingLatencySamples() const;
    int computeTotalLatencySamples(float lookaheadMs) const;
    void updateLatencySamples(float lookaheadMs);
    void applyClipperLatencyDelay(juce::AudioBuffer<float>& buffer, int delaySamples);
    void applyDeltaDelay(juce::AudioBuffer<float>& buffer, int delaySamples);
    
    Clipper clipperModule;
    Limiter limiterModule;
    
    // 4x Oversampling (Factor 2 = 2^2 = 4)
    // Pro-L 2 Style: Use high-quality oversampling stages to reduce aliasing
    // We pre-allocate oversamplers for 2x, 4x, 8x, 16x to allow realtime switching
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[4];
    int currentLimiterOversamplingIndex = 0; // 0=Off, 1=2x, 2=4x, 3=8x, 4=16x
    int currentClipperOversamplingIndex = 2; // min 4x

    std::unique_ptr<juce::dsp::Oversampling<float>> tpSafetyOversampler;
    float tpSafetyGain = 1.0f;
    
    double currentSampleRate = 44100.0;
    std::atomic<bool> userDefaultsDirty { false };
    std::atomic<int64_t> userDefaultsLastChangeMs { 0 };
    bool suppressUserDefaultsListener = false;

    int lastReportedLatencySamples = -1;
    int maxOversamplingLatencySamples = 0;
    int clipperLatencyDelaySamples = 0;
    int clipperLatencyDelayWritePos = 0;
    int clipperLatencyDelayBufferSize = 0;
    std::vector<float> clipperLatencyDelayL;
    std::vector<float> clipperLatencyDelayR;

    int deltaDelaySamples = 0;
    int deltaDelayWritePos = 0;
    int deltaDelayBufferSize = 0;
    int maxLookaheadSamples = 0;
    std::vector<float> deltaDelayL;
    std::vector<float> deltaDelayR;
    juce::AudioBuffer<float> deltaBuffer;

    static constexpr int vizFifoCapacity = 8192;
    juce::AbstractFifo vizFifo { vizFifoCapacity };
    std::array<float, vizFifoCapacity> vizInDb {};
    std::array<float, vizFifoCapacity> vizOutDb {};

    int vizHopSamples = 256;
    float vizInPeak = 0.0f;
    float vizOutPeak = 0.0f;
    int64_t vizSampleCursor = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipperLimiterAudioProcessor)
};
