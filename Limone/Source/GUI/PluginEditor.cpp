#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>
#include <limits>

//==============================================================================
ClipperLimiterAudioProcessorEditor::ClipperLimiterAudioProcessorEditor (ClipperLimiterAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
#if JUCE_WEB_BROWSER
    auto options = juce::WebBrowserComponent::Options()
                   .withNativeIntegrationEnabled()
#if JUCE_USE_WIN_WEBVIEW2
                   .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
                   .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2()
                                            .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                                                                 .getChildFile ("Lim.one")
                                                                 .getChildFile ("WebView2")))
#endif
                   .withResourceProvider ([this] (const auto& url) {
                       return getResource (url);
                   }, juce::WebBrowserComponent::getResourceProviderRoot())
                   .withNativeFunction ("hostEmit", [this] (const juce::Array<juce::var>& args, auto completion) {
                     // Check arguments
                     if (args.size() < 2) {
                         completion(juce::var());
                         return;
                     }

                     auto key = args[0].toString();
                     auto valVar = args[1];

                     // Debug logging
                     DBG("hostEmit: " << key << " -> " << valVar.toString());

                     // 1. Parameter Update
                     if (auto* paramToUpdate = audioProcessor.apvts.getParameter(key))
                     {
                         float value = 0.0f;
                         if (valVar.isDouble() || valVar.isInt()) value = (float)valVar;
                         else if (valVar.isBool()) value = valVar ? 1.0f : 0.0f;
                         else if (valVar.isString()) value = valVar.toString().getFloatValue();

                         auto range = paramToUpdate->getNormalisableRange();
                         auto normValue = range.convertTo0to1(value);
                         paramToUpdate->setValueNotifyingHost(normValue);
                     }
                     // 2. Gestures
                     else if (key == "beginGesture")
                    {
                        auto paramId = valVar.toString();
                        audioProcessor.beginGesture(paramId); // Notify processor
                        if (auto* paramToBegin = audioProcessor.apvts.getParameter(paramId))
                            paramToBegin->beginChangeGesture();
                    }
                    else if (key == "endGesture")
                    {
                        auto paramId = valVar.toString();
                        audioProcessor.endGesture(paramId); // Notify processor
                        if (auto* paramToEnd = audioProcessor.apvts.getParameter(paramId))
                            paramToEnd->endChangeGesture();
                    }
                     // 3. UI Ready
                     else if (key == "uiReady")
                     {
                         juce::var allParams(new juce::DynamicObject());
                         auto& params = audioProcessor.getParameters();
                         for (auto* paramPtr : params)
                         {
                             if (auto* paramToInit = dynamic_cast<juce::AudioProcessorParameterWithID*> (paramPtr))
                             {
                                 float val = *audioProcessor.apvts.getRawParameterValue(paramToInit->paramID);
                                 allParams.getDynamicObject()->setProperty(paramToInit->paramID, val);
                             }
                         }
                         sendWebEvent("init", allParams);
                     }
                     // 4. Reset Loudness
                     else if (key == "resetLoudness")
                    {
                        audioProcessor.lufsMeter.reset();
                    }
                    else if (key == "resetLinks")
                    {
                        audioProcessor.resetLinks();
                    }
                    else if (key == "getSettingsInfo")
                    {
                        juce::var info(new juce::DynamicObject());
                        info.getDynamicObject()->setProperty("name", audioProcessor.getName());
                        info.getDynamicObject()->setProperty("version", JucePlugin_VersionString);
                        info.getDynamicObject()->setProperty("company", JucePlugin_Manufacturer);
#if JUCE_DEBUG
                        info.getDynamicObject()->setProperty("build", "Debug");
#else
                        info.getDynamicObject()->setProperty("build", "Release");
#endif
                        info.getDynamicObject()->setProperty("os", juce::SystemStats::getOperatingSystemName());
                        info.getDynamicObject()->setProperty("cpuModel", juce::SystemStats::getCpuModel());

                        sendWebEvent("settingsInfo", info);
                    }
                    else if (key == "openExternalUrl")
                    {
                        juce::String urlText;
                        if (valVar.isObject())
                            urlText = valVar.getProperty ("url", juce::var()).toString();
                        else
                            urlText = valVar.toString();

                        const auto trimmed = urlText.trim();
                        bool ok = false;
                        juce::String error;

                        if (trimmed.isEmpty())
                        {
                            error = "Empty URL";
                        }
                        else
                        {
                            const juce::URL url (trimmed);
                            const auto scheme = url.getScheme().toLowerCase();

                            if (! (scheme == "https" || scheme == "http"))
                            {
                                error = "Invalid scheme";
                            }
                            else
                            {
                                ok = url.launchInDefaultBrowser();
                                if (! ok)
                                    ok = juce::Process::openDocument (trimmed, {});

                                if (! ok)
                                    error = "Launch failed";
                            }
                        }

                        juce::var result (new juce::DynamicObject());
                        result.getDynamicObject()->setProperty ("ok", ok);
                        result.getDynamicObject()->setProperty ("url", trimmed);
                        result.getDynamicObject()->setProperty ("error", error);
                        sendWebEvent ("openExternalUrlResult", result);
                    }
                    else if (key == "checkForUpdates")
                    {
                        juce::String urlText;
                        int requestId = 0;

                        if (valVar.isObject())
                        {
                            urlText = valVar.getProperty ("url", juce::var()).toString();
                            const auto ridVar = valVar.getProperty ("requestId", 0);
                            if (ridVar.isInt())
                            {
                                requestId = (int) ridVar;
                            }
                            else if (ridVar.isDouble())
                            {
                                const auto rid = (double) ridVar;
                                if (std::isfinite (rid) && rid >= 1.0 && rid <= (double) std::numeric_limits<int>::max())
                                    requestId = (int) rid;
                            }
                            else if (ridVar.isString())
                            {
                                const auto ridText = ridVar.toString().trim();
                                if (ridText.isNotEmpty())
                                    requestId = ridText.getIntValue();
                            }
                        }
                        else
                        {
                            urlText = valVar.toString();
                        }

                        if (requestId <= 0)
                            requestId = ++updateRequestCounter;
                        else
                            updateRequestCounter.store (requestId);

                        const auto trimmed = urlText.trim();
                        const juce::URL url (trimmed);
                        const auto scheme = url.getScheme().toLowerCase();

                        if (trimmed.isEmpty() || ! (scheme == "https" || scheme == "http"))
                        {
                            juce::var result (new juce::DynamicObject());
                            result.getDynamicObject()->setProperty ("ok", false);
                            result.getDynamicObject()->setProperty ("requestId", requestId);
                            result.getDynamicObject()->setProperty ("statusCode", 0);
                            result.getDynamicObject()->setProperty ("error", trimmed.isEmpty() ? "Empty URL" : "Invalid scheme");
                            sendWebEvent ("updateCheckResult", result);
                        }
                        else
                        {
                            requestUpdateCheck (trimmed, requestId);
                        }
                    }

                     completion(juce::var());
                 });

#if JUCE_WINDOWS
    if (! juce::WebBrowserComponent::areOptionsSupported (options))
    {
        useResourceProvider = false;
        localWebAssetsDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile (audioProcessor.getName())
                                .getChildFile ("web");

        localWebAssetsDir.createDirectory();

        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            int dataSize = 0;
            const auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize);

            if (data == nullptr || dataSize <= 0)
                continue;

            const auto original = juce::String (BinaryData::originalFilenames[i]).replaceCharacter ('\\', '/');
            const auto marker = original.lastIndexOf ("/web/");
            const auto rel = marker >= 0 ? original.substring (marker + 5)
                                         : juce::File (original).getFileName();

            auto dest = localWebAssetsDir.getChildFile (rel);
            dest.getParentDirectory().createDirectory();
            dest.replaceWithData (data, dataSize);
        }
    }
#endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (*webView);
#endif  // JUCE_WEB_BROWSER
    
    {
        const float rawPct = *audioProcessor.apvts.getRawParameterValue ("ui_scale");
        lastUiScalePct = juce::jlimit (50.0f, 200.0f, rawPct);
        const float s = lastUiScalePct / 100.0f;
        setSize ((int) std::lround ((float) baseUiWidth * s),
                 (int) std::lround ((float) baseUiHeight * s));
    }
    setResizable(false, false);

    // Initialize parameter cache
    auto& params = audioProcessor.getParameters();
    for (auto* param : params)
    {
        if (auto* pParam = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
        {
            float val = *audioProcessor.apvts.getRawParameterValue(pParam->paramID);
            lastParamValues[pParam->paramID] = val;
        }
    }

    // Start timer for metering and parameter updates (60fps)
    startTimerHz (60);
    
    setupWebBrowser();
}

ClipperLimiterAudioProcessorEditor::~ClipperLimiterAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void ClipperLimiterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void ClipperLimiterAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

//==============================================================================
std::optional<juce::WebBrowserComponent::Resource> ClipperLimiterAudioProcessorEditor::getResource(const juce::String& url)
{
    DBG("getResource: " << url);

    auto cleaned = url.upToFirstOccurrenceOf ("?", false, false)
                       .upToFirstOccurrenceOf ("#", false, false)
                       .trim();

    auto path = cleaned;
    if (path.contains ("://"))
    {
        const auto schemePos = path.indexOf ("://");
        const auto firstSlash = path.indexOfChar (schemePos + 3, '/');
        path = firstSlash >= 0 ? path.substring (firstSlash) : "/";
    }

    path = path.trim();
    if (path.isEmpty() || path == "/")
        path = "/index.html";

    while (path.startsWithChar ('/'))
        path = path.substring (1);

    if (path.isEmpty())
        path = "index.html";

    if (path.contains ("..") || path.containsChar (':') || path.containsChar ('\\'))
        return std::nullopt;

    const auto contentTypeForPath = [] (const juce::String& p) -> juce::String
    {
        const auto ext = p.fromLastOccurrenceOf (".", false, false).toLowerCase();
        if (ext == "html" || ext == "htm") return "text/html";
        if (ext == "js") return "application/javascript";
        if (ext == "css") return "text/css";
        if (ext == "json") return "application/json";
        if (ext == "svg") return "image/svg+xml";
        if (ext == "png") return "image/png";
        if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
        if (ext == "gif") return "image/gif";
        if (ext == "webp") return "image/webp";
        if (ext == "woff2") return "font/woff2";
        if (ext == "woff") return "font/woff";
        if (ext == "ttf") return "font/ttf";
        if (ext == "otf") return "font/otf";
        if (ext == "wasm") return "application/wasm";
        return "application/octet-stream";
    };

    const auto normalisedRequest = path.replaceCharacter ('\\', '/');
    const auto requestBasename = juce::File (normalisedRequest).getFileName();

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const auto original = juce::String (BinaryData::originalFilenames[i]).replaceCharacter ('\\', '/');

        if (! original.endsWith ("/web/" + normalisedRequest)
            && ! original.endsWith ("/web/" + requestBasename)
            && ! original.endsWith ("/" + normalisedRequest)
            && ! original.endsWith ("/" + requestBasename)
            && ! original.endsWith (normalisedRequest)
            && ! original.endsWith (requestBasename))
        {
            continue;
        }

        int dataSize = 0;
        const auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize);

        if (data == nullptr || dataSize <= 0)
            return std::nullopt;

        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (reinterpret_cast<const std::byte*> (data),
                                    reinterpret_cast<const std::byte*> (data + dataSize)),
            contentTypeForPath (requestBasename)
        };
    }

    return std::nullopt;
}

void ClipperLimiterAudioProcessorEditor::setupWebBrowser()
{
    if (webView == nullptr)
        return;

    if (! useResourceProvider)
    {
        const auto indexFile = localWebAssetsDir.getChildFile ("index.html");
        webView->goToURL (juce::URL (indexFile).toString (true));
        return;
    }

    auto url = juce::WebBrowserComponent::getResourceProviderRoot();
    if (url.endsWithChar ('/'))
        url = url.dropLastCharacters (1);

    webView->goToURL (url);
}

void ClipperLimiterAudioProcessorEditor::sendWebEvent(const juce::String& type, const juce::var& data)
{
    if (webView == nullptr)
        return;

    juce::var message(new juce::DynamicObject());
    message.getDynamicObject()->setProperty("type", type);
    message.getDynamicObject()->setProperty("data", data);
    
    // Modern JUCE 8 way: Emit event that JS can listen to
    // This replaces manual evaluateJavascript
    webView->emitEventIfBrowserIsVisible("juce_backend_event", message);
}

void ClipperLimiterAudioProcessorEditor::requestUpdateCheck (const juce::String& urlText, int requestId)
{
    struct UpdateCheckJob final : public juce::ThreadPoolJob
    {
        UpdateCheckJob (juce::Component::SafePointer<ClipperLimiterAudioProcessorEditor> editorIn,
                        juce::String urlIn,
                        int requestIdIn)
            : juce::ThreadPoolJob ("UpdateCheckJob"),
              editor (std::move (editorIn)),
              urlText (std::move (urlIn)),
              requestId (requestIdIn)
        {
        }

        JobStatus runJob() override
        {
            if (editor == nullptr)
                return jobHasFinished;

            if (requestId != editor->updateRequestCounter.load())
                return jobHasFinished;

            juce::StringPairArray responseHeaders;
            int statusCode = 0;
            juce::String error;
            juce::var payload;

            const juce::URL url (urlText);

            auto stream = url.createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                                     .withConnectionTimeoutMs (10000)
                                                     .withNumRedirectsToFollow (3)
                                                     .withResponseHeaders (&responseHeaders)
                                                     .withStatusCode (&statusCode));

            if (stream == nullptr)
            {
                error = "Network error";
            }
            else if (statusCode >= 400 || statusCode == 0)
            {
                error = "HTTP " + juce::String (statusCode);
            }
            else
            {
                const auto body = stream->readEntireStreamAsString();
                payload = juce::JSON::parse (body);
                if (payload.isVoid() || payload.isUndefined())
                    error = "Invalid JSON";
            }

            const bool ok = error.isEmpty();
            auto editorCopy = editor;
            const auto urlCopy = urlText;
            const auto payloadCopy = payload;
            const auto errorCopy = error;
            const auto statusCodeCopy = statusCode;
            const auto requestIdCopy = requestId;
            juce::MessageManager::callAsync ([editorCopy, ok, payloadCopy, errorCopy, statusCodeCopy, requestIdCopy, urlCopy]() mutable {
                if (editorCopy == nullptr)
                    return;

                if (requestIdCopy != editorCopy->updateRequestCounter.load())
                    return;

                juce::var result (new juce::DynamicObject());
                result.getDynamicObject()->setProperty ("ok", ok);
                result.getDynamicObject()->setProperty ("requestId", requestIdCopy);
                result.getDynamicObject()->setProperty ("statusCode", statusCodeCopy);
                result.getDynamicObject()->setProperty ("url", urlCopy);
                result.getDynamicObject()->setProperty ("error", errorCopy);
                result.getDynamicObject()->setProperty ("payload", payloadCopy);
                editorCopy->sendWebEvent ("updateCheckResult", result);
            });

            return jobHasFinished;
        }

        juce::Component::SafePointer<ClipperLimiterAudioProcessorEditor> editor;
        juce::String urlText;
        int requestId = 0;
    };

    updateThreadPool.addJob (new UpdateCheckJob ({ this }, urlText, requestId), true);
}

void ClipperLimiterAudioProcessorEditor::handleWebEvent(const juce::var& eventData)
{
    if (!eventData.isObject()) return;
    
    auto type = eventData["type"].toString();
    
    // 1. Parameter Change
    if (type == "setParameter")
    {
        auto paramId = eventData["paramId"].toString();
        float value = 0.0f;
        
        if (eventData["value"].isDouble() || eventData["value"].isInt())
            value = (float) eventData["value"];
        else if (eventData["value"].isBool())
            value = eventData["value"] ? 1.0f : 0.0f;
        else if (eventData["value"].isString())
            value = eventData["value"].toString().getFloatValue();

        if (auto* param = audioProcessor.apvts.getParameter(paramId))
        {
            auto range = param->getNormalisableRange();
            auto normValue = range.convertTo0to1(value);
            param->setValueNotifyingHost(normValue);
        }
    }
    // 2. Gesture Start
    else if (type == "beginGesture")
    {
        auto paramId = eventData["paramId"].toString();
        if (auto* param = audioProcessor.apvts.getParameter(paramId))
            param->beginChangeGesture();
    }
    // 3. Gesture End
    else if (type == "endGesture")
    {
        auto paramId = eventData["paramId"].toString();
        if (auto* param = audioProcessor.apvts.getParameter(paramId))
            param->endChangeGesture();
    }
    // 4. Init Request
    else if (type == "uiReady")
    {
        juce::var allParams(new juce::DynamicObject());
        auto& params = audioProcessor.getParameters();
        
        for (auto* p : params)
        {
            if (auto* param = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            {
                float val = *audioProcessor.apvts.getRawParameterValue(param->paramID);
                allParams.getDynamicObject()->setProperty(param->paramID, val);
            }
        }
        sendWebEvent("init", allParams);
    }
    // 5. Reset Loudness
    else if (type == "resetLoudness")
    {
        audioProcessor.lufsMeter.reset();
    }
}

void ClipperLimiterAudioProcessorEditor::timerCallback()
{
    {
        const float rawPct = *audioProcessor.apvts.getRawParameterValue ("ui_scale");
        const float pct = juce::jlimit (50.0f, 200.0f, rawPct);
        if (std::abs (pct - lastUiScalePct) > 0.5f)
        {
            lastUiScalePct = pct;
            const float s = pct / 100.0f;
            setSize ((int) std::lround ((float) baseUiWidth * s),
                     (int) std::lround ((float) baseUiHeight * s));
        }
    }

    // 1. Metering Updates
    // Atomic exchange to get the max reduction (minimum value) since last read
    // and reset the atomic to 0.0f for the next accumulation cycle.
    float rawLimitGR = audioProcessor.limiterGR.exchange(0.0f);
    float rawClipGR = audioProcessor.clipperGR.exchange(0.0f);
    float rawLimitGRL = audioProcessor.limiterGRLeft.exchange(0.0f);
    float rawLimitGRR = audioProcessor.limiterGRRight.exchange(0.0f);
    float rawClipGRL = audioProcessor.clipperGRLeft.exchange(0.0f);
    float rawClipGRR = audioProcessor.clipperGRRight.exchange(0.0f);
    float rawOutL = audioProcessor.outputMeterLeft.exchange(0.0f);
    float rawOutR = audioProcessor.outputMeterRight.exchange(0.0f);
    
    // Smooth Limit GR
    if (rawLimitGR < smoothLimitGR)
        smoothLimitGR = rawLimitGR;
    else
        smoothLimitGR = std::min(smoothLimitGR + 0.5f, 0.0f);

    if (rawLimitGRL < smoothLimitGRL)
        smoothLimitGRL = rawLimitGRL;
    else
        smoothLimitGRL = std::min(smoothLimitGRL + 0.5f, 0.0f);

    if (rawLimitGRR < smoothLimitGRR)
        smoothLimitGRR = rawLimitGRR;
    else
        smoothLimitGRR = std::min(smoothLimitGRR + 0.5f, 0.0f);
        
    // Smooth Clip GR
    if (rawClipGR < smoothClipGR)
        smoothClipGR = rawClipGR;
    else
        smoothClipGR = std::min(smoothClipGR + 0.5f, 0.0f);

    if (rawClipGRL < smoothClipGRL)
        smoothClipGRL = rawClipGRL;
    else
        smoothClipGRL = std::min(smoothClipGRL + 0.5f, 0.0f);

    if (rawClipGRR < smoothClipGRR)
        smoothClipGRR = rawClipGRR;
    else
        smoothClipGRR = std::min(smoothClipGRR + 0.5f, 0.0f);

    if (smoothLimitGRL < holdLimitGRL)
        holdLimitGRL = smoothLimitGRL;
    else
        holdLimitGRL = std::min(holdLimitGRL + 0.2f, 0.0f);

    if (smoothLimitGRR < holdLimitGRR)
        holdLimitGRR = smoothLimitGRR;
    else
        holdLimitGRR = std::min(holdLimitGRR + 0.2f, 0.0f);

    if (smoothClipGRL < holdClipGRL)
        holdClipGRL = smoothClipGRL;
    else
        holdClipGRL = std::min(holdClipGRL + 0.2f, 0.0f);

    if (smoothClipGRR < holdClipGRR)
        holdClipGRR = smoothClipGRR;
    else
        holdClipGRR = std::min(holdClipGRR + 0.2f, 0.0f);

    auto linToDb = [](float x) {
        if (x <= 1.0e-6f) return -100.0f;
        const float db = 20.0f * std::log10(x);
        return std::max(-100.0f, std::min(db, 0.0f));
    };

    const float rawOutDbL = linToDb(rawOutL);
    const float rawOutDbR = linToDb(rawOutR);

    if (rawOutDbL > smoothOutL)
        smoothOutL = rawOutDbL;
    else
        smoothOutL = std::max(smoothOutL - 1.5f, rawOutDbL);

    if (rawOutDbR > smoothOutR)
        smoothOutR = rawOutDbR;
    else
        smoothOutR = std::max(smoothOutR - 1.5f, rawOutDbR);
    
    // Get LUFS Values
    float lufsInt = audioProcessor.lufsMeter.getIntegratedLoudness();
    float lufsShort = audioProcessor.lufsMeter.getShortTermLoudness();
    float lufsMom = audioProcessor.lufsMeter.getMomentaryLoudness();

    {
        constexpr float lufsFloor = -300.0f;
        const auto nowMs = juce::Time::getMillisecondCounter();
        const auto lastMs = (juce::uint32) audioProcessor.lastProcessTime.load();
        const bool hasAudioCallback = lastMs != 0;
        const bool isIdle = hasAudioCallback && (nowMs - lastMs) > 250;

        if (isIdle)
        {
            if (lastIdleLufsUpdateMs == 0)
            {
                lastIdleLufsUpdateMs = nowMs;
                idleStartLufsShort = lufsShort;
                idleStartLufsMom = lufsMom;
            }

            const float tSec = std::max(0.0f, (nowMs - lastIdleLufsUpdateMs) * 0.001f);

            auto windowReplaceWithSilence = [lufsFloor, tSec](float lufs0, float windowSec) {
                if (lufs0 <= lufsFloor) return lufsFloor;
                const float window = std::max(1.0e-3f, windowSec);
                const float frac = std::max(0.0f, 1.0f - (tSec / window));
                const float p0 = std::pow(10.0f, lufs0 / 10.0f);
                const float p = p0 * frac;
                if (!(p > 0.0f)) return lufsFloor;
                return std::max(lufsFloor, 10.0f * std::log10(p));
            };

            lufsShort = windowReplaceWithSilence(idleStartLufsShort, 3.0f);
            lufsMom = windowReplaceWithSilence(idleStartLufsMom, 0.4f);
        }
        else
        {
            lastIdleLufsUpdateMs = 0;
        }
    }
    
    // Only send updates if values changed significantly (reduce IPC load)
    bool grChanged = std::abs(smoothLimitGR - lastSentLimitGR) > 0.01f ||
                     std::abs(smoothClipGR - lastSentClipGR) > 0.01f ||
                     std::abs(smoothLimitGRL - lastSentLimitGRL) > 0.01f ||
                     std::abs(smoothLimitGRR - lastSentLimitGRR) > 0.01f ||
                     std::abs(smoothClipGRL - lastSentClipGRL) > 0.01f ||
                     std::abs(smoothClipGRR - lastSentClipGRR) > 0.01f;
                     
    bool lufsChanged = std::abs(lufsInt - lastSentLufsInt) > 0.01f ||
                       std::abs(lufsShort - lastSentLufsShort) > 0.01f ||
                       std::abs(lufsMom - lastSentLufsMom) > 0.01f;

    bool outChanged = std::abs(smoothOutL - lastSentOutL) > 0.1f ||
                      std::abs(smoothOutR - lastSentOutR) > 0.1f;

    bool holdChanged = std::abs(holdLimitGRL - lastSentHoldLimitGRL) > 0.01f ||
                       std::abs(holdLimitGRR - lastSentHoldLimitGRR) > 0.01f ||
                       std::abs(holdClipGRL - lastSentHoldClipGRL) > 0.01f ||
                       std::abs(holdClipGRR - lastSentHoldClipGRR) > 0.01f;

    juce::Array<float> vizInDb;
    juce::Array<float> vizOutDb;
    const int vizRead = audioProcessor.readVizPoints(vizInDb, vizOutDb, 256);
    const bool vizChanged = vizRead > 0 && vizInDb.size() == vizOutDb.size();
    
    if (grChanged || lufsChanged || outChanged || holdChanged || vizChanged)
    {
        juce::var meterData(new juce::DynamicObject());
        const float limitGRLinked = std::min(smoothLimitGRL, smoothLimitGRR);
        const float clipGRLinked = std::min(smoothClipGRL, smoothClipGRR);
        meterData.getDynamicObject()->setProperty("limitGR", limitGRLinked);
        meterData.getDynamicObject()->setProperty("clipGR", clipGRLinked);
        meterData.getDynamicObject()->setProperty("limitGRL", smoothLimitGRL);
        meterData.getDynamicObject()->setProperty("limitGRR", smoothLimitGRR);
        meterData.getDynamicObject()->setProperty("clipGRL", smoothClipGRL);
        meterData.getDynamicObject()->setProperty("clipGRR", smoothClipGRR);

        meterData.getDynamicObject()->setProperty("limitHoldL", holdLimitGRL);
        meterData.getDynamicObject()->setProperty("limitHoldR", holdLimitGRR);
        meterData.getDynamicObject()->setProperty("clipHoldL", holdClipGRL);
        meterData.getDynamicObject()->setProperty("clipHoldR", holdClipGRR);
        
        meterData.getDynamicObject()->setProperty("lufsInt", lufsInt);
        meterData.getDynamicObject()->setProperty("lufsShort", lufsShort);
        meterData.getDynamicObject()->setProperty("lufsMom", lufsMom);

        meterData.getDynamicObject()->setProperty("outL", smoothOutL);
        meterData.getDynamicObject()->setProperty("outR", smoothOutR);

        if (vizChanged)
        {
            juce::Array<juce::var> inVar;
            juce::Array<juce::var> outVar;
            inVar.ensureStorageAllocated(vizRead);
            outVar.ensureStorageAllocated(vizRead);

            for (int i = 0; i < vizRead; ++i)
            {
                inVar.add(vizInDb.getReference(i));
                outVar.add(vizOutDb.getReference(i));
            }

            meterData.getDynamicObject()->setProperty("vizInDb", juce::var(inVar));
            meterData.getDynamicObject()->setProperty("vizOutDb", juce::var(outVar));
            meterData.getDynamicObject()->setProperty("vizHopSamples", audioProcessor.getVizHopSamples());
            meterData.getDynamicObject()->setProperty("vizSampleRate", audioProcessor.getSampleRate());
        }
        
        sendWebEvent("updateMeters", meterData);
        
        lastSentLimitGR = limitGRLinked;
        lastSentClipGR = clipGRLinked;
        lastSentLimitGRL = smoothLimitGRL;
        lastSentLimitGRR = smoothLimitGRR;
        lastSentClipGRL = smoothClipGRL;
        lastSentClipGRR = smoothClipGRR;
        lastSentLufsInt = lufsInt;
        lastSentLufsShort = lufsShort;
        lastSentLufsMom = lufsMom;

        lastSentOutL = smoothOutL;
        lastSentOutR = smoothOutR;

        lastSentHoldLimitGRL = holdLimitGRL;
        lastSentHoldLimitGRR = holdLimitGRR;
        lastSentHoldClipGRL = holdClipGRL;
        lastSentHoldClipGRR = holdClipGRR;
    }
    
    // 2. Parameter Updates (Polling)
    juce::var paramUpdates(new juce::DynamicObject());
    bool hasUpdates = false;
    
    auto& params = audioProcessor.getParameters();
    for (auto* param : params)
    {
        if (auto* pParam = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
        {
             float currentVal = *audioProcessor.apvts.getRawParameterValue(pParam->paramID);
             float lastVal = lastParamValues[pParam->paramID];
             
             // Check for change
             if (std::abs(currentVal - lastVal) > 0.0001f)
             {
                 paramUpdates.getDynamicObject()->setProperty(pParam->paramID, currentVal);
                 lastParamValues[pParam->paramID] = currentVal;
                 hasUpdates = true;
             }
        }
    }
    
    if (hasUpdates)
    {
        sendWebEvent("updateParams", paramUpdates);
    }
}
