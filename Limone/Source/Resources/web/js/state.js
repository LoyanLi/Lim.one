// Source/Resources/web/js/state.js
window.AudioPlugin = window.AudioPlugin || {};

AudioPlugin.State = (function() {
    const params = {
        in_gain: 0.0,
        drive: 0.0,
        knee: 0.0,
        character: 0.5,
        limiter_drive: 0.0,
        ceiling: -0.1,
        out_gain: -0.1,
        clipper_mode: 0,
        limiter_mode: 1,
        ui_mode: 0,
        true_peak: true,
        delta: false,
        oversampling: 2,
        ui_scale: 100.0,
        lookahead: 2.0,

        modern_hold_ms: 4.0,
        modern_hold_release_ms: 1.5,
        modern_attack_tau_div: 2.5,
        modern_release_smooth_base_ms: 1.5,
        modern_release_smooth_range_ms: 6.0,
        modern_adapt_fast_strength: 1.5,
        modern_adapt_slow_strength: 3.0,
        modern_sc_hpf_hz: 120.0,
        modern_ratio_base: 1.5,
        modern_ratio_slope: 0.9,
        modern_transient_mix: 0.3,
        modern_soft_safety_strength: 6.0,
        modern_release_fast_ms: 2.0,
        modern_release_slow_ms: 30.0,
        modern_link_transients: 0.5,
        modern_link_release: 0.95
    };

    let drawerOpen = false;
    let activeDragParam = null;
    let settingsInfo = {};
    let licenseLocked = false;
    let activationAutoOpened = false;

    function setParam(key, value) {
        params[key] = value;
    }
    
    function getParam(key) {
        return params[key];
    }

    function init(initialParams) {
        Object.assign(params, initialParams);
    }

    return {
        params, // Direct access if needed, but setters preferred
        setParam,
        getParam,
        init,
        get settingsInfo() { return settingsInfo; },
        setSettingsInfo(v) { settingsInfo = v || {}; },
        get licenseLocked() { return licenseLocked; },
        set licenseLocked(v) { licenseLocked = !!v; },
        get activationAutoOpened() { return activationAutoOpened; },
        set activationAutoOpened(v) { activationAutoOpened = !!v; },
        get drawerOpen() { return drawerOpen; },
        set drawerOpen(v) { drawerOpen = v; },
        get activeDragParam() { return activeDragParam; },
        set activeDragParam(v) { activeDragParam = v; }
    };
})();
