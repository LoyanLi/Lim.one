#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

//==============================================================================
class ClipperLimiterAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                            public juce::Timer
{
public:
    ClipperLimiterAudioProcessorEditor (ClipperLimiterAudioProcessor&);
    ~ClipperLimiterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    // Timer for meter and parameter updates
    void timerCallback() override;

private:
    ClipperLimiterAudioProcessor& audioProcessor;
    
    std::unique_ptr<juce::WebBrowserComponent> webView;
    juce::File localWebAssetsDir;
    bool useResourceProvider = true;

    void requestUpdateCheck (const juce::String& urlText, int requestId);
    juce::ThreadPool updateThreadPool { 1 };
    std::atomic<int> updateRequestCounter { 0 };
    
    // Resource Provider
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);
    
    // Helpers
    void setupWebBrowser();
    void handleWebEvent(const juce::var& eventData);
    void sendWebEvent(const juce::String& type, const juce::var& data);
    
    // Cache for parameter polling
    std::map<juce::String, float> lastParamValues;
    
    // Smooth Metering
    float smoothLimitGR = 0.0f;
    float smoothClipGR = 0.0f;
    float smoothLimitGRL = 0.0f;
    float smoothLimitGRR = 0.0f;
    float smoothClipGRL = 0.0f;
    float smoothClipGRR = 0.0f;
    float smoothOutL = -100.0f;
    float smoothOutR = -100.0f;
    float holdLimitGRL = 0.0f;
    float holdLimitGRR = 0.0f;
    float holdClipGRL = 0.0f;
    float holdClipGRR = 0.0f;
    
    // Cache for filtering redundant updates
    float lastSentLimitGR = 100.0f;
    float lastSentClipGR = 100.0f;
    float lastSentLimitGRL = 100.0f;
    float lastSentLimitGRR = 100.0f;
    float lastSentClipGRL = 100.0f;
    float lastSentClipGRR = 100.0f;
    float lastSentOutL = -100.0f;
    float lastSentOutR = -100.0f;
    float lastSentHoldLimitGRL = 100.0f;
    float lastSentHoldLimitGRR = 100.0f;
    float lastSentHoldClipGRL = 100.0f;
    float lastSentHoldClipGRR = 100.0f;
    float lastSentLufsInt = -100.0f;
    float lastSentLufsShort = -100.0f;
    float lastSentLufsMom = -100.0f;
    juce::uint32 lastIdleLufsUpdateMs = 0;
    float idleStartLufsShort = -300.0f;
    float idleStartLufsMom = -300.0f;

    const int baseUiWidth = 1280;
    const int baseUiHeight = 724;
    float lastUiScalePct = 100.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipperLimiterAudioProcessorEditor)
};
