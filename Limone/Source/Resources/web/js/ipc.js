window.AudioPlugin = window.AudioPlugin || {};

AudioPlugin.IPC = (function() {
    let lastEmitTime = 0;
    const EMIT_INTERVAL = 16; // ~60fps
    let didRequestSettingsInfo = false;
    
    function log(msg, type = 'info') {
        console.log(`[IPC] ${msg}`);
    }

    // ============================================================================
    // JUCE 8 Native Integration (Robust Hybrid)
    // ============================================================================
    
    // Bridge Discovery & Polling
    let bridgeMode = null; // 'standard' | 'webkit' | null

    function checkBridge() {
        if (typeof window.hostEmit === 'function') {
            bridgeMode = 'standard';
            log("Bridge: Standard JUCE 8 (hostEmit) Connected", 'info');
            return true;
        } 
        else if (window.__JUCE__ && window.__JUCE__.backend) {
             bridgeMode = 'juce8_backend';
             log("Bridge: JUCE 8 __JUCE__.backend Connected", 'info');
             
             // Polyfill hostEmit for compatibility
             window.hostEmit = function(key, value) {
                 // Use the internal emitEvent that PluginEditor.cpp expects
                 // Note: PluginEditor.cpp uses .withNativeFunction("hostEmit", ...)
                 // This corresponds to __juce__invoke with name="hostEmit"
                 window.__JUCE__.backend.emitEvent("__juce__invoke", { 
                     name: "hostEmit", 
                     params: [key, value], 
                     resultId: 0 
                 });
             };
             return true;
        }
        else if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.hostEmit) {
            bridgeMode = 'webkit';
            log("Bridge: macOS WebKit Native Connected", 'info');
            return true;
        }
        return false;
    }

    // Also listen via __JUCE__ backend if available
    function setupJuceEventListener() {
        if (window.__JUCE__ && window.__JUCE__.backend && !window.__juceListenerAttached) {
             window.__JUCE__.backend.addEventListener("juce_backend_event", (data) => {
                 // data is the payload object
                 // Dispatch as CustomEvent to keep compatibility with existing listener
                 const event = new CustomEvent("juce_backend_event", { detail: data });
                 window.dispatchEvent(event);
             });
             window.__juceListenerAttached = true;
             log("Attached JUCE 8 Backend Listener", 'info');
        }
    }

    // Start polling for bridge
    let pollCount = 0;
    const bridgePoller = setInterval(() => {
        pollCount++;
        setupJuceEventListener(); // Try to attach listener early
        
        if (checkBridge()) {
            clearInterval(bridgePoller);
            emit('uiReady', 0);
        }
    }, 100);

    // 1. Listen for events from C++
    window.addEventListener("juce_backend_event", (event) => {
        const message = event.detail;
        if (message) {
            if (message.type !== 'meters' && message.type !== 'updateMeters') {
                 log(`RX: ${message.type}`, 'info');
            }
            handleMessage(message);
        }
    });

    // Helper for safe emitting based on bridge mode
    function safeNativeEmit(key, value) {
        if (typeof window.hostEmit === 'function') {
            window.hostEmit(key, value);
            if (key !== 'uiReady') log(`TX: ${key} = ${JSON.stringify(value)}`, 'info');
        }
        else if (bridgeMode === 'juce8_backend' && window.__JUCE__ && window.__JUCE__.backend) {
            // Should be covered by hostEmit polyfill, but safe fallback
            window.__JUCE__.backend.emitEvent("__juce__invoke", { 
                name: "hostEmit", 
                params: [key, value], 
                resultId: 0 
            });
            if (key !== 'uiReady') log(`TX(J8): ${key} = ${JSON.stringify(value)}`, 'info');
        } 
        else if (bridgeMode === 'webkit' && window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.hostEmit) {
            // WebKit message handlers take a single body.
            // JUCE C++ expects an array: [key, value]
            window.webkit.messageHandlers.hostEmit.postMessage([key, value]);
            if (key !== 'uiReady') log(`TX(Native): ${key} = ${JSON.stringify(value)}`, 'info');
        } 
        else {
            log(`TX FAILED: ${key} (No Bridge)`, 'error');
        }
    }

    function emit(type, data) {
        // Map abstract events to C++ (key, value) pairs
        if (type === 'setParameter') {
             safeNativeEmit(data.paramId, data.value);
        } else if (type === 'beginGesture') {
             // For gestures, value is the paramId
             safeNativeEmit('beginGesture', data.paramId);
        } else if (type === 'endGesture') {
             // For gestures, value is the paramId
             safeNativeEmit('endGesture', data.paramId);
        } else if (type === 'uiReady') {
             safeNativeEmit('uiReady', 0);
        } else if (type === 'resetLoudness') {
             safeNativeEmit('resetLoudness', 0);
        } else if (type === 'getSettingsInfo') {
             safeNativeEmit('getSettingsInfo', 0);
        } else if (type === 'activateLicense') {
             safeNativeEmit('activateLicense', data);
        } else if (type === 'deactivateLicense') {
             safeNativeEmit('deactivateLicense', 0);
        } else if (type === 'openExternalUrl') {
             safeNativeEmit('openExternalUrl', data);
        } else if (type === 'checkForUpdates') {
             safeNativeEmit('checkForUpdates', data);
        }
    }

    function emitParamUpdate(paramId, value) {
        emit('setParameter', { paramId: paramId, value: value });
    }

    function throttledEmitParamUpdate(paramId, value) {
        const now = Date.now();
        if (now - lastEmitTime > EMIT_INTERVAL) {
            emitParamUpdate(paramId, value);
            lastEmitTime = now;
        }
    }

    function handleMessage(message) {
        // message is { type: "...", data: ... } passed from C++
        if (!message || !message.type) return;
        
        const { type, data } = message;

        if (type === 'init') {
             if (AudioPlugin.State.init) {
                 AudioPlugin.State.init(data);
             }
             if (AudioPlugin.UI.fullUpdate) {
                 AudioPlugin.UI.fullUpdate();
             }
             if (!didRequestSettingsInfo) {
                 didRequestSettingsInfo = true;
                 emit('getSettingsInfo', 0);
             }
        }
        else if (type === 'updateParams') {
            // Bulk update (from timer)
            if (window.updateParams) {
                window.updateParams(data);
            }
        }
        else if (type === 'meters' || type === 'updateMeters') {
             if (window.updateMeters) {
                 window.updateMeters(data);
             }
        }
        else if (type === 'settingsInfo') {
            if (AudioPlugin.State && AudioPlugin.State.setSettingsInfo) {
                AudioPlugin.State.setSettingsInfo(data);
            }
            if (AudioPlugin.UI && AudioPlugin.UI.updateSettingsInfo) {
                AudioPlugin.UI.updateSettingsInfo(data);
            }
        }
        else if (type === 'licenseResult') {
            if (AudioPlugin.UI && AudioPlugin.UI.handleLicenseResult) {
                AudioPlugin.UI.handleLicenseResult(data);
            }
        }
        else if (type === 'openExternalUrlResult') {
            if (AudioPlugin.UI && AudioPlugin.UI.handleOpenExternalUrlResult) {
                AudioPlugin.UI.handleOpenExternalUrlResult(data);
            }
        }
        else if (type === 'updateCheckResult') {
            if (AudioPlugin.Controls && AudioPlugin.Controls.handleUpdateCheckResult) {
                AudioPlugin.Controls.handleUpdateCheckResult(data);
            }
        }
    }

    return {
        emit,
        emitParamUpdate,
        throttledEmitParamUpdate,
        handleMessage // Exposed for testing, though triggered by event listener now
    };
})();
