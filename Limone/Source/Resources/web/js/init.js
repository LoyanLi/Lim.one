// Initialize Controls when DOM is ready
const initApp = () => {
    console.log("initApp called");
    if (window.AudioPlugin && window.AudioPlugin.Controls) {
        window.AudioPlugin.Controls.init();
        console.log("Controls initialized");
    } else {
        console.error("AudioPlugin.Controls not found");
    }
    
    // Emit uiReady event to request initial state
    if (window.AudioPlugin && window.AudioPlugin.IPC) {
        window.AudioPlugin.IPC.emit('uiReady', {});
        console.log("uiReady emitted");
    }

    if (window.AudioPlugin && window.AudioPlugin.Visualizer && window.AudioPlugin.Visualizer.init) {
        window.AudioPlugin.Visualizer.init();
    }

    if (window.AudioPlugin && window.AudioPlugin.State) {
        window.AudioPlugin.State.activationAutoOpened = true;
    }
    if (window.AudioPlugin && window.AudioPlugin.IPC) {
        window.AudioPlugin.IPC.emit('getSettingsInfo', 0);
    }
};

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initApp);
} else {
    initApp();
}

// Global functions for C++ to call (defined immediately)
window.updateParams = function(jsonStr) {
    if (!window.AudioPlugin || !window.AudioPlugin.State || !window.AudioPlugin.UI) return;

    try {
        const data = typeof jsonStr === 'string' ? JSON.parse(jsonStr) : jsonStr;
        const State = window.AudioPlugin.State;
        const UI = window.AudioPlugin.UI;
        
        for (const [key, value] of Object.entries(data)) {
            // Update internal state
            State.setParam(key, value);
            
            // Update UI if not currently being dragged by user
            if (State.activeDragParam !== key) {
                UI.updateParamUI(key, value);
            }
        }
    } catch (e) {
        console.error("updateParams error:", e);
    }
};

window.updateMeters = function(data) {
    if (window.AudioPlugin && window.AudioPlugin.UI) {
        window.AudioPlugin.UI.updateMeters(data);
    }
};
