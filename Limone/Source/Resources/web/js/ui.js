window.AudioPlugin = window.AudioPlugin || {};

AudioPlugin.UI = (function() {
    const State = AudioPlugin.State;
    let didAutoOpenForActivation = false;

    const KNOBS = {
        drive: {
            min: 0,
            max: 18,
            circumference: 326.73,
            arcId: 'arc-drive',
            valueId: 'val-drive',
            format: (v) => `+${v.toFixed(2)} dB`
        },
        knee: {
            min: 0,
            max: 1,
            circumference: 326.73,
            arcId: 'arc-knee',
            valueId: 'val-knee',
            format: (v) => `${(v * 100).toFixed(0)}%`
        },
        limiter_drive: {
            min: 0,
            max: 18,
            circumference: 326.73,
            arcId: 'arc-limiter_drive',
            valueId: 'val-limiter_drive',
            format: (v) => `+${v.toFixed(2)} dB`
        },
        in_gain: {
            min: -24,
            max: 24,
            circumference: 326.73,
            arcId: 'arc-in_gain',
            valueId: 'val-in_gain',
            format: (v) => `${v >= 0 ? '+' : ''}${v.toFixed(2)} dB`
        },
        out_gain: {
            min: -24,
            max: 1,
            circumference: 326.73,
            arcId: 'arc-out_gain',
            valueId: 'val-out_gain',
            format: (v) => `${v >= 0 ? '+' : ''}${v.toFixed(2)} dB`
        },
        character: {
            min: 0,
            max: 1,
            circumference: 326.73,
            arcId: 'arc-character',
            valueId: 'val-character',
            format: (v) => `${(v * 100).toFixed(0)}%`
        },
        ceiling: {
            min: -1,
            max: 0,
            circumference: 326.73,
            arcId: 'arc-ceiling',
            valueId: 'val-ceiling',
            format: (v) => `${v.toFixed(2)} dB`
        },
        lookahead: {
            min: 0,
            max: 5,
            circumference: 326.73,
            arcId: 'arc-lookahead',
            valueId: 'val-lookahead',
            format: (v) => `${v.toFixed(2)} ms`
        },
        modern_hold_ms: {
            min: 0,
            max: 10,
            circumference: 326.73,
            arcId: 'arc-modern_hold_ms',
            valueId: 'val-modern_hold_ms',
            format: (v) => `${v.toFixed(2)} ms`
        },
        modern_hold_release_ms: {
            min: 0.05,
            max: 10,
            circumference: 326.73,
            arcId: 'arc-modern_hold_release_ms',
            valueId: 'val-modern_hold_release_ms',
            format: (v) => `${v.toFixed(2)} ms`
        },
        modern_attack_tau_div: {
            min: 0.25,
            max: 8,
            circumference: 326.73,
            arcId: 'arc-modern_attack_tau_div',
            valueId: 'val-modern_attack_tau_div',
            format: (v) => `${v.toFixed(2)}`
        },
        modern_release_smooth_base_ms: {
            min: 0,
            max: 20,
            circumference: 326.73,
            arcId: 'arc-modern_release_smooth_base_ms',
            valueId: 'val-modern_release_smooth_base_ms',
            format: (v) => `${v.toFixed(2)} ms`
        },
        modern_release_smooth_range_ms: {
            min: 0,
            max: 50,
            circumference: 326.73,
            arcId: 'arc-modern_release_smooth_range_ms',
            valueId: 'val-modern_release_smooth_range_ms',
            format: (v) => `${v.toFixed(2)} ms`
        },
        modern_adapt_fast_strength: {
            min: 0,
            max: 8,
            circumference: 326.73,
            arcId: 'arc-modern_adapt_fast_strength',
            valueId: 'val-modern_adapt_fast_strength',
            format: (v) => `${v.toFixed(2)}`
        },
        modern_adapt_slow_strength: {
            min: 0,
            max: 16,
            circumference: 326.73,
            arcId: 'arc-modern_adapt_slow_strength',
            valueId: 'val-modern_adapt_slow_strength',
            format: (v) => `${v.toFixed(2)}`
        },
        modern_sc_hpf_hz: {
            min: 20,
            max: 400,
            circumference: 326.73,
            arcId: 'arc-modern_sc_hpf_hz',
            valueId: 'val-modern_sc_hpf_hz',
            format: (v) => `${v.toFixed(0)} Hz`
        },
        modern_ratio_base: {
            min: 0.1,
            max: 3,
            circumference: 326.73,
            arcId: 'arc-modern_ratio_base',
            valueId: 'val-modern_ratio_base',
            format: (v) => `${v.toFixed(2)}`
        },
        modern_ratio_slope: {
            min: 0,
            max: 1.5,
            circumference: 326.73,
            arcId: 'arc-modern_ratio_slope',
            valueId: 'val-modern_ratio_slope',
            format: (v) => `${v.toFixed(2)}`
        },
        modern_transient_mix: {
            min: 0,
            max: 1,
            circumference: 326.73,
            arcId: 'arc-modern_transient_mix',
            valueId: 'val-modern_transient_mix',
            format: (v) => `${v.toFixed(3)}`
        },
        modern_soft_safety_strength: {
            min: 0,
            max: 20,
            circumference: 326.73,
            arcId: 'arc-modern_soft_safety_strength',
            valueId: 'val-modern_soft_safety_strength',
            format: (v) => `${v.toFixed(2)}`
        },
        modern_release_fast_ms: {
            min: 0.05,
            max: 20,
            circumference: 326.73,
            arcId: 'arc-modern_release_fast_ms',
            valueId: 'val-modern_release_fast_ms',
            format: (v) => `${v.toFixed(2)} ms`
        },
        modern_release_slow_ms: {
            min: 0.1,
            max: 200,
            circumference: 326.73,
            arcId: 'arc-modern_release_slow_ms',
            valueId: 'val-modern_release_slow_ms',
            format: (v) => `${v.toFixed(1)} ms`
        },
        modern_link_transients: {
            min: 0,
            max: 1,
            circumference: 326.73,
            arcId: 'arc-modern_link_transients',
            valueId: 'val-modern_link_transients',
            format: (v) => `${v.toFixed(3)}`
        },
        modern_link_release: {
            min: 0,
            max: 1,
            circumference: 326.73,
            arcId: 'arc-modern_link_release',
            valueId: 'val-modern_link_release',
            format: (v) => `${v.toFixed(3)}`
        }
    };

    let pendingMeterUpdate = null;

    function updateModeUI() {
        const val = Math.round(State.params.limiter_mode);
        const sel = document.getElementById('select-limiter-mode');
        if (sel) sel.value = String(val === 1 ? 1 : 0);

        // Dynamic visibility for Modern parameters
        const isModern = (val === 1);
        const sliders = document.querySelectorAll('input[type="range"][id^="slider-modern_"]');
        sliders.forEach(slider => {
            const container = slider.parentElement;
            if (container) {
                if (isModern) container.classList.remove('hidden');
                else container.classList.add('hidden');
            }
        });
    }

    function updateClipperModeUI() {
        const val = Math.round(State.params.clipper_mode);
        const sel = document.getElementById('select-clipper-mode');
        if (!sel) return;
        sel.value = String(val === 1 ? 1 : 0);
    }

    function updateTruePeakUI() {
        const indicator = document.getElementById('tp-indicator');
        const isEnabled = !!State.params.true_peak;
        if (isEnabled) {
            indicator.classList.remove('bg-white', 'border', 'border-gray-300', 'translate-x-1');
            indicator.classList.add('bg-primary', 'translate-x-4');
        } else {
            indicator.classList.remove('bg-primary', 'translate-x-4');
            indicator.classList.add('bg-white', 'border', 'border-gray-300', 'translate-x-1');
        }

    }

    function updateDeltaUI() {
        const btn = document.getElementById('btn-delta');
        if (!btn) return;
        const isEnabled = !!State.params.delta;
        btn.textContent = isEnabled ? 'On' : 'Off';
        if (isEnabled) {
            btn.classList.add('text-primary', 'bg-primary/10', 'border-primary/30');
            btn.classList.remove('text-gray-500', 'bg-gray-100', 'border-gray-200');
        } else {
            btn.classList.remove('text-primary', 'bg-primary/10', 'border-primary/30');
            btn.classList.add('text-gray-500', 'bg-gray-100', 'border-gray-200');
        }
    }

    function updateUiScaleUI() {
        const raw = State.params.ui_scale;
        const pct = Math.round((raw === undefined || raw === null) ? 100 : Number(raw));
        const sel = document.getElementById('select-ui-scale');
        if (sel) {
            const steps = [75, 100, 125, 150, 175, 200];
            let best = steps[0];
            let bestDist = Math.abs(pct - best);
            for (let i = 1; i < steps.length; ++i) {
                const d = Math.abs(pct - steps[i]);
                if (d < bestDist) {
                    bestDist = d;
                    best = steps[i];
                }
            }
            sel.value = String(best);
        }

        const s = Math.max(0.5, Math.min(2.0, pct / 100));
        if (document.body) {
            document.body.style.transformOrigin = '';
            document.body.style.transform = '';
            document.body.style.width = '';
            document.body.style.height = '';
        }

        const inner = document.getElementById('scale-inner');
        if (inner) {
            const w = Math.max(1, Math.round(window.innerWidth / s));
            const h = Math.max(1, Math.round(window.innerHeight / s));
            inner.style.width = `${w}px`;
            inner.style.height = `${h}px`;
            inner.style.transform = `translate(-50%, -50%) scale(${s})`;
        }
    }

    function setAdvancedOpen(isOpen) {
        const panel = document.getElementById('advanced-panel');
        const backdrop = document.getElementById('advanced-backdrop');
        if (panel) panel.classList.toggle('hidden', !isOpen);
        if (backdrop) backdrop.classList.toggle('hidden', !isOpen);
        State.drawerOpen = isOpen;

        const btnAdvMenu = document.getElementById('btn-adv-menu');
        if (btnAdvMenu) {
            btnAdvMenu.classList.toggle('text-primary', isOpen);
            btnAdvMenu.classList.toggle('text-gray-400', !isOpen);
        }
    }

    function setSettingsOpen(isOpen) {
        const panel = document.getElementById('settings-panel');
        const backdrop = document.getElementById('settings-backdrop');
        const wasOpen = panel ? !panel.classList.contains('hidden') : false;
        if (panel) panel.classList.toggle('hidden', !isOpen);
        if (backdrop) backdrop.classList.toggle('hidden', !isOpen);

        if (isOpen && !wasOpen) {
            setAdvancedOpen(false);
            if (window.AudioPlugin && window.AudioPlugin.IPC) {
                window.AudioPlugin.IPC.emit('getSettingsInfo', 0);
            }
        }
    }

    function updateSettingsInfo(info) {
        const data = info || State.settingsInfo || {};

        const setText = (id, v) => {
            const el = document.getElementById(id);
            if (!el) return;
            const text = (v === undefined || v === null || v === '') ? '-' : String(v);
            el.textContent = text;
        };

        setText('settings-version', data.version);

        const updateStatus = document.getElementById('settings-update-status');
        if (updateStatus && (!updateStatus.textContent || updateStatus.textContent.trim() === '')) {
            updateStatus.textContent = '';
        }
        
        // Removed licensing UI logic
    }

    function setAlertOpen(isOpen, title, message, kind) {
        const backdrop = document.getElementById('alert-backdrop');
        if (!backdrop) return;

        const titleEl = document.getElementById('alert-title');
        const msgEl = document.getElementById('alert-message');
        const subEl = document.getElementById('alert-sub');

        if (titleEl) titleEl.textContent = title || 'Alert';
        if (msgEl) msgEl.textContent = message || '';
        if (subEl) subEl.textContent = (kind === 'error') ? 'Error' : 'Message';

        backdrop.classList.toggle('hidden', !isOpen);
    }

    function showAlert(title, message, kind) {
        setAlertOpen(true, title, message, kind);
    }

    function handleLicenseResult(data) {
        const resultEl = document.getElementById('license-result');
        if (!resultEl) return;

        const ok = !!(data && data.ok);
        const action = (data && data.action) ? String(data.action) : 'activate';
        const actionLabel = (action === 'deactivate') ? 'Deactivation' : 'Activation';
        const defaultFail = `${actionLabel} failed`;
        const defaultOk = `${actionLabel} successful`;
        const msg = ok ? defaultOk : (data && data.error ? String(data.error) : defaultFail);
        resultEl.textContent = msg;
        resultEl.classList.toggle('text-green-600', ok);
        resultEl.classList.toggle('text-red-600', !ok);
        resultEl.classList.remove('hidden');

        if (!ok) {
            setSettingsOpen(true);
            State.licenseLocked = true;
            showAlert(defaultFail, msg, 'error');
            return;
        }

        showAlert(defaultOk, msg, 'message');
    }

    function handleOpenExternalUrlResult(data) {
        const ok = !!(data && data.ok);
        if (ok) return;
        const url = (data && data.url) ? String(data.url) : '';
        const err = (data && data.error) ? String(data.error) : 'Launch failed';
        showAlert('Open link failed', url ? `${err}\n${url}` : err, 'error');
    }

    function updateKnobUI(paramId, value) {
        const cfg = KNOBS[paramId];
        if (!cfg) return;
        const range = cfg.max - cfg.min;
        const pct = range === 0 ? 0 : (value - cfg.min) / range;
        const clampedPct = Math.max(0, Math.min(1, pct));
        const arc = document.getElementById(cfg.arcId);
        if (arc) {
            const arcLength = cfg.circumference * 0.75;
            const activeLength = arcLength * clampedPct;
            arc.style.strokeDasharray = `${activeLength} ${cfg.circumference}`;
            arc.style.strokeDashoffset = '0';
        }
        const needle = document.getElementById(`needle-${paramId}`);
        if (needle) {
            const deg = (clampedPct * 270) - 135;
            needle.style.transform = `rotate(${deg}deg)`;
        }

        const slider = document.getElementById(`slider-${paramId}`);
        if (slider) slider.value = String(value);

        const valEl = document.getElementById(cfg.valueId);
        if (valEl) valEl.textContent = cfg.format(value);
    }

    function updateMeters(data) {
        if (pendingMeterUpdate) return;
        pendingMeterUpdate = requestAnimationFrame(() => {
            // GR Updates
            const limitGR = (data.limitGR !== undefined) ? data.limitGR : data.limit;
            const clipGR = (data.clipGR !== undefined) ? data.clipGR : data.clip;
            const limitGRL = (data.limitGRL !== undefined) ? data.limitGRL : limitGR;
            const limitGRR = (data.limitGRR !== undefined) ? data.limitGRR : limitGR;
            const clipGRL = (data.clipGRL !== undefined) ? data.clipGRL : clipGR;
            const clipGRR = (data.clipGRR !== undefined) ? data.clipGRR : clipGR;
            
            const getRangeDb = () => {
                const viz = window.AudioPlugin && window.AudioPlugin.Visualizer;
                if (viz && typeof viz.getRangeDb === 'function') {
                    const r = viz.getRangeDb();
                    if (Number.isFinite(r) && r > 0) return r;
                }
                return 18;
            };
            const rangeDb = getRangeDb();
            const getPadTopPct = () => {
                const viz = window.AudioPlugin && window.AudioPlugin.Visualizer;
                if (viz && typeof viz.getPadTopPct === 'function') {
                    const v = viz.getPadTopPct();
                    if (Number.isFinite(v) && v >= 0) return v;
                }
                return 0;
            };
            const getPadBottomPct = () => {
                const viz = window.AudioPlugin && window.AudioPlugin.Visualizer;
                if (viz && typeof viz.getPadBottomPct === 'function') {
                    const v = viz.getPadBottomPct();
                    if (Number.isFinite(v) && v >= 0) return v;
                }
                return 6;
            };
            const padTopPct = getPadTopPct();
            const padBottomPct = getPadBottomPct();
            const trackPct = Math.max(1, 100 - padTopPct - padBottomPct);

            const mapGR = (gr) => {
                if (gr > 0) gr = 0;
                const x = Math.abs(gr);
                return Math.min(100, (Math.min(x, rangeDb) / rangeDb) * 100);
            };
            const wLimit = mapGR(limitGR);
            const wClip = mapGR(clipGR);
            const wLimitL = mapGR(limitGRL);
            const wLimitR = mapGR(limitGRR);
            const wClipL = mapGR(clipGRL);
            const wClipR = mapGR(clipGRR);

            const clipHoldL = (data.clipHoldL !== undefined) ? data.clipHoldL : clipGRL;
            const clipHoldR = (data.clipHoldR !== undefined) ? data.clipHoldR : clipGRR;
            const limitHoldL = (data.limitHoldL !== undefined) ? data.limitHoldL : limitGRL;
            const limitHoldR = (data.limitHoldR !== undefined) ? data.limitHoldR : limitGRR;

            const wClipHoldL = mapGR(clipHoldL);
            const wClipHoldR = mapGR(clipHoldR);
            const wLimitHoldL = mapGR(limitHoldL);
            const wLimitHoldR = mapGR(limitHoldR);

            const meterLimit = document.getElementById('meter-limit-bar');
            if (meterLimit) meterLimit.style.width = `${wLimit}%`;
            const meterClip = document.getElementById('meter-clip-bar');
            if (meterClip) meterClip.style.width = `${wClip}%`;

            const meterLimitL = document.getElementById('meter-limit-bar-l');
            const meterLimitR = document.getElementById('meter-limit-bar-r');
            const meterClipL = document.getElementById('meter-clip-bar-l');
            const meterClipR = document.getElementById('meter-clip-bar-r');
            
            const setBar = (el, w) => {
                if (!el) return;
                const clamped = Math.min(100, Math.max(0, w));
                el.style.top = `${padTopPct}%`;
                el.style.height = `${(clamped / 100) * trackPct}%`;
            };

            setBar(meterLimitL, wLimitL);
            setBar(meterLimitR, wLimitR);
            setBar(meterClipL, wClipL);
            setBar(meterClipR, wClipR);

            const setHold = (id, w) => {
                const el = document.getElementById(id);
                if (!el) return;
                const clamped = Math.min(100, Math.max(0, w));
                el.style.top = `${padTopPct + (clamped / 100) * trackPct}%`;
            };

            setHold('meter-clip-hold-l', wClipHoldL);
            setHold('meter-clip-hold-r', wClipHoldR);
            setHold('meter-limit-hold-l', wLimitHoldL);
            setHold('meter-limit-hold-r', wLimitHoldR);

            const mapLevelDb = (db) => {
                if (db > 0) db = 0;
                if (db < -30) db = -30;
                return ((db + 30) / 30) * 100;
            };

            if (data.outL !== undefined) {
                const el = document.getElementById('meter-out-bar-l');
                if (el) el.style.width = `${mapLevelDb(data.outL)}%`;
            }
            if (data.outR !== undefined) {
                const el = document.getElementById('meter-out-bar-r');
                if (el) el.style.width = `${mapLevelDb(data.outR)}%`;
            }
            
            const formatLufs = (v) => {
                if (!Number.isFinite(v) || v < -127) return '-∞';
                return v.toFixed(1);
            };

            if (data.lufsInt !== undefined) {
                const el = document.getElementById('val-lufs-int');
                if (el) el.textContent = formatLufs(data.lufsInt);
            }
            if (data.lufsShort !== undefined) {
                const el = document.getElementById('val-lufs-st');
                if (el) el.textContent = formatLufs(data.lufsShort);
            }
            if (data.lufsMom !== undefined) {
                const el = document.getElementById('val-lufs-mom');
                if (el) el.textContent = formatLufs(data.lufsMom);
            }

            if (window.AudioPlugin && window.AudioPlugin.Visualizer && window.AudioPlugin.Visualizer.update) {
                window.AudioPlugin.Visualizer.update(data);
            }
            
            pendingMeterUpdate = null;
        });
    }

    function updateParamUI(paramId, value) {
        if (KNOBS[paramId]) {
            updateKnobUI(paramId, value);
            return;
        }

        if (paramId.endsWith('_link')) {
            const btn = document.querySelector(`button[data-link-param="${paramId}"]`);
            if (btn) {
                const isLinked = value > 0.5;
                if (isLinked) {
                    btn.classList.add('text-primary', 'border-primary/30', 'bg-primary/10');
                    btn.classList.remove('text-gray-300', 'border-gray-200');
                } else {
                    btn.classList.remove('text-primary', 'border-primary/30', 'bg-primary/10');
                    btn.classList.add('text-gray-300', 'border-gray-200');
                }
            }
            return;
        }

        const slider = document.getElementById(`slider-${paramId}`);
        if (slider) {
            slider.value = String(value);
        }

        const valEl = document.getElementById(`val-${paramId}`);
        if (valEl) {
            if (paramId.endsWith('_ms')) valEl.textContent = `${value.toFixed(2)} ms`;
            else if (paramId === 'modern_sc_hpf_hz') valEl.textContent = `${value.toFixed(0)} Hz`;
            else if (paramId === 'modern_transient_mix' || paramId === 'modern_link_transients' || paramId === 'modern_link_release') valEl.textContent = `${value.toFixed(3)}`;
            else valEl.textContent = `${value.toFixed(2)}`;
        }

        if (paramId === 'clipper_mode') {
            updateClipperModeUI();
        } else if (paramId === 'limiter_mode') {
            updateModeUI();
        } else if (paramId === 'true_peak') {
            updateTruePeakUI();
        } else if (paramId === 'delta') {
            updateDeltaUI();
        } else if (paramId === 'ui_scale') {
            updateUiScaleUI();
        } else if (paramId === 'oversampling') {
            const sel = document.getElementById('select-oversampling');
            if (sel) sel.value = String(Math.round(value) % 5);
        }
    }

    function fullUpdate() {
        Object.keys(State.params).forEach(key => {
            updateParamUI(key, State.params[key]);
        });
        if (window.AudioPlugin && window.AudioPlugin.Visualizer && window.AudioPlugin.Visualizer.syncControls) {
            window.AudioPlugin.Visualizer.syncControls();
        }
    }

    window.addEventListener('resize', () => {
        updateUiScaleUI();
    });

    return {
        updateClipperModeUI,
        updateModeUI,
        updateTruePeakUI,
        updateDeltaUI,
        updateKnobUI,
        updateParamUI,
        updateMeters,
        setAdvancedOpen,
        setSettingsOpen,
        updateSettingsInfo,
        handleLicenseResult,
        handleOpenExternalUrlResult,
        showAlert,
        setAlertOpen,
        fullUpdate
    };
})();

AudioPlugin.Visualizer = (function() {
    const METER_PAD_TOP_PCT = 0;
    const METER_PAD_BOTTOM_PCT = 6;

    const TICK_MODES = [
        { label: 'Off', step: 0 },
        { label: '6 dB', step: 6 },
        { label: '3 dB', step: 3 }
    ];

    const RANGE_MODES = [
        { label: '12 dB', range: 12 },
        { label: '18 dB', range: 18 },
        { label: '30 dB', range: 30 }
    ];

    const SPEED_MODES = [
        { label: 'Fast', seconds: 6.0 },
        { label: 'Med', seconds: 10.0 },
        { label: 'Slow', seconds: 16.0 }
    ];

    const state = {
        container: null,
        canvas: null,
        ctx: null,
        ro: null,
        dpr: 1,
        cssW: 0,
        cssH: 0,
        cols: 0,
        filled: 0,
        write: 0,
        outDb: null,
        clipDb: null,
        limitDb: null,
        lufsSt: null,
        frac: 0,
        hopSamples: 256,
        sampleRate: 48000,
        windowSeconds: 10.0,
        lastDataTs: 0,
        lastSynthTs: 0,
        synthFrac: 0,
        lastMeterUpdateTs: 0,
        lastOutDb: -100,
        lastClipGR: 0,
        lastLimitGR: 0,
        lastLufsSt: -127,
        showWaveform: true,
        tickMode: 2,
        rangeMode: 1,
        speedMode: 1,
        running: false,
        lastDrawTs: 0,
        dirty: true
    };

    const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));

    const STORAGE_KEY = 'limone.viz.settings.v1';

    function saveSettings() {
        try {
            const payload = {
                tickStep: TICK_MODES[state.tickMode].step,
                rangeDb: RANGE_MODES[state.rangeMode].range,
                speedSeconds: SPEED_MODES[state.speedMode].seconds
            };
            localStorage.setItem(STORAGE_KEY, JSON.stringify(payload));
        } catch (_) {}
    }

    function loadSettings() {
        let raw = null;
        try {
            raw = localStorage.getItem(STORAGE_KEY);
        } catch (_) {
            return;
        }
        if (!raw) return;

        let payload = null;
        try {
            payload = JSON.parse(raw);
        } catch (_) {
            return;
        }
        if (!payload || typeof payload !== 'object') return;

        if (Number.isFinite(payload.tickStep)) {
            const idx = TICK_MODES.findIndex(m => m.step === payload.tickStep);
            if (idx >= 0) state.tickMode = idx;
        }

        if (Number.isFinite(payload.rangeDb)) {
            let bestIdx = 0;
            let bestDist = Number.POSITIVE_INFINITY;
            for (let i = 0; i < RANGE_MODES.length; ++i) {
                const d = Math.abs(RANGE_MODES[i].range - payload.rangeDb);
                if (d < bestDist) {
                    bestDist = d;
                    bestIdx = i;
                }
            }
            state.rangeMode = bestIdx;
        }

        if (Number.isFinite(payload.speedSeconds)) {
            const idx = SPEED_MODES.findIndex(m => m.seconds === payload.speedSeconds);
            if (idx >= 0) {
                state.speedMode = idx;
                state.windowSeconds = SPEED_MODES[state.speedMode].seconds;
            }
        }
    }

    function rebuildGrMeterScale(containerId) {
        const el = document.getElementById(containerId);
        if (!el) return;

        el.innerHTML = '';

        const step = TICK_MODES[state.tickMode].step;
        if (!(step > 0)) return;

        const rangeDb = RANGE_MODES[state.rangeMode].range;
        const padTopPct = METER_PAD_TOP_PCT;
        const padBottomPct = METER_PAD_BOTTOM_PCT;
        const trackPct = Math.max(1, 100 - padTopPct - padBottomPct);

        const majorStep = 6;
        const showLabels = containerId === 'meter-limit-scale';

        for (let db = 0; db >= -rangeDb; db -= step) {
            const pct = padTopPct + (Math.abs(db) / rangeDb) * trackPct;

            const line = document.createElement('div');
            const isMajor = (db % majorStep) === 0 || db === -rangeDb;
            line.className = 'absolute left-0 right-0 h-px';
            line.style.top = `${pct}%`;
            line.style.backgroundColor = isMajor ? 'rgba(0, 0, 0, 0.18)' : 'rgba(0, 0, 0, 0.10)';
            el.appendChild(line);

            if (isMajor && showLabels) {
                const label = document.createElement('div');
                label.className = 'absolute text-[9px] font-mono';
                label.style.top = `${pct}%`;
                label.style.right = '-18px';
                label.style.transform = 'translateY(-50%)';
                label.style.color = 'rgba(107, 114, 128, 1)';
                label.textContent = String(db);
                el.appendChild(label);
            }
        }
    }

    function rebuildGrMeterScales() {
        rebuildGrMeterScale('meter-clip-scale');
        rebuildGrMeterScale('meter-limit-scale');
    }

    function updateControlLabels() {
        const ticksBtn = document.getElementById('btn-viz-ticks');
        if (ticksBtn) ticksBtn.textContent = TICK_MODES[state.tickMode].label;

        const rangeBtn = document.getElementById('btn-viz-range');
        if (rangeBtn) rangeBtn.textContent = RANGE_MODES[state.rangeMode].label;

        const speedBtn = document.getElementById('btn-viz-speed');
        if (speedBtn) speedBtn.textContent = SPEED_MODES[state.speedMode].label;

        rebuildGrMeterScales();
    }

    function ensure() {
        if (state.running) return true;
        const container = document.getElementById('viz-area');
        if (!container) return false;

        state.container = container;
        state.dpr = Math.max(1, Math.min(2, window.devicePixelRatio || 1));

        const canvas = document.createElement('canvas');
        canvas.className = 'absolute inset-0 w-full h-full';
        canvas.setAttribute('aria-hidden', 'true');
        container.appendChild(canvas);

        const ctx = canvas.getContext('2d', { alpha: true, desynchronized: true });
        if (!ctx) return false;

        state.canvas = canvas;
        state.ctx = ctx;

        state.ro = new ResizeObserver(() => resize());
        state.ro.observe(container);

        resize();
        state.running = true;
        loadSettings();
        updateControlLabels();
        requestAnimationFrame(draw);
        return true;
    }

    function resize() {
        if (!state.container || !state.canvas) return;
        const rect = state.container.getBoundingClientRect();
        const cssW = Math.max(1, Math.floor(rect.width));
        const cssH = Math.max(1, Math.floor(rect.height));
        const w = Math.max(1, Math.floor(cssW * state.dpr));
        const h = Math.max(1, Math.floor(cssH * state.dpr));

        if (state.canvas.width !== w) state.canvas.width = w;
        if (state.canvas.height !== h) state.canvas.height = h;
        state.canvas.style.width = `${cssW}px`;
        state.canvas.style.height = `${cssH}px`;

        state.cssW = cssW;
        state.cssH = cssH;

        const nextCols = Math.max(64, cssW);
        state.cols = nextCols;
        state.outDb = new Float32Array(nextCols);
        state.outDb.fill(-100);
        state.clipDb = new Float32Array(nextCols);
        state.limitDb = new Float32Array(nextCols);
        state.clipDb.fill(0);
        state.limitDb.fill(0);
        state.lufsSt = new Float32Array(nextCols);
        state.lufsSt.fill(-127);
        state.filled = 0;
        state.write = 0;
        state.frac = 0;

        state.dirty = true;
    }

    function push(outDb, clipDb, limitDb, lufsSt) {
        if (!state.outDb || !state.clipDb || !state.limitDb || !state.lufsSt || state.cols <= 0) return;
        const i = state.write;
        state.outDb[i] = outDb;
        state.clipDb[i] = clipDb;
        state.limitDb[i] = limitDb;
        state.lufsSt[i] = lufsSt;
        state.write = (i + 1) % state.cols;
        state.filled = Math.min(state.cols, state.filled + 1);
    }

    function update(meterData) {
        if (!ensure()) return;
        if (!meterData) return;

        const nowTs = performance.now();

        if (Number.isFinite(meterData.vizHopSamples)) state.hopSamples = Math.max(1, Math.round(meterData.vizHopSamples));
        if (Number.isFinite(meterData.vizSampleRate)) state.sampleRate = Math.max(1, Number(meterData.vizSampleRate));

        const pps = state.cssW / Math.max(0.25, state.windowSeconds);

        const rawClipGR = Number(meterData.clipGR);
        const rawLimitGR = Number(meterData.limitGR);
        const clipGR = clamp(Number.isFinite(rawClipGR) ? rawClipGR : 0, -18, 0);
        const limitGR = clamp(Number.isFinite(rawLimitGR) ? rawLimitGR : 0, -18, 0);
        const rawLufsSt = Number(meterData.lufsShort);
        if (Number.isFinite(rawLufsSt)) state.lastLufsSt = rawLufsSt;
        state.lastClipGR = clipGR;
        state.lastLimitGR = limitGR;
        state.lastDataTs = nowTs;
        state.lastSynthTs = nowTs;

        if (Array.isArray(meterData.vizOutDb)) {
            const dt = state.hopSamples / state.sampleRate;
            const dx = dt * pps;
            const n = meterData.vizOutDb.length;
            let lastOutDb = state.lastOutDb;
            for (let i = 0; i < n; ++i) {
                const raw = Number(meterData.vizOutDb[i]);
                const outDb = clamp(Number.isFinite(raw) ? raw : -100, -100, 0);
                lastOutDb = outDb;
                state.frac += dx;
                while (state.frac >= 1.0) {
                    state.frac -= 1.0;
                    push(outDb, clipGR, limitGR, state.lastLufsSt);
                }
            }
            state.lastOutDb = lastOutDb;
            state.lastMeterUpdateTs = nowTs;
            state.dirty = true;
            return;
        }

        const outL = Number.isFinite(meterData.outL) ? meterData.outL : -100;
        const outR = Number.isFinite(meterData.outR) ? meterData.outR : outL;
        const outDb = clamp(Math.max(outL, outR), -100, 0);
        state.lastOutDb = outDb;

        const elapsedSecRaw = state.lastMeterUpdateTs > 0 ? (nowTs - state.lastMeterUpdateTs) / 1000 : 0;
        const elapsedSec = clamp(elapsedSecRaw, 0, 0.25);
        const dx = elapsedSec * pps;
        state.lastMeterUpdateTs = nowTs;

        state.frac += dx;
        while (state.frac >= 1.0) {
            state.frac -= 1.0;
            push(outDb, clipGR, limitGR, state.lastLufsSt);
        }
        state.dirty = true;
    }

    function synthIfStale(ts) {
        if (!state.outDb || !state.clipDb || !state.limitDb || !state.lufsSt) return;
        if (!Number.isFinite(state.lastDataTs) || state.lastDataTs <= 0) return;
        if (ts - state.lastDataTs < 180) return;

        const pps = state.cssW / Math.max(0.25, state.windowSeconds);
        if (!(pps > 0)) return;

        if (!Number.isFinite(state.lastSynthTs) || state.lastSynthTs <= 0) {
            state.lastSynthTs = ts;
            state.synthFrac = 0;
            return;
        }

        const dtMs = Math.min(250, Math.max(0, ts - state.lastSynthTs));
        state.lastSynthTs = ts;

        const inc = (dtMs / 1000) * pps;
        state.synthFrac += inc;

        const maxPushes = 64;
        const pushes = Math.min(maxPushes, Math.floor(state.synthFrac));
        if (pushes <= 0) return;

        state.synthFrac -= pushes;
        for (let i = 0; i < pushes; ++i) push(state.lastOutDb, state.lastClipGR, state.lastLimitGR, state.lastLufsSt);
        state.dirty = true;
    }

    function draw(ts) {
        if (!state.running || !state.ctx) return;
        synthIfStale(ts);
        const stale = Number.isFinite(state.lastDataTs) && (ts - state.lastDataTs) >= 180;

        state.lastDrawTs = ts;
        state.dirty = false;

        const ctx = state.ctx;
        const w = state.cssW;
        const h = state.cssH;

        ctx.setTransform(state.dpr, 0, 0, state.dpr, 0, 0);
        ctx.clearRect(0, 0, w, h);

        const topDb = 0;
        const rangeDb = RANGE_MODES[state.rangeMode].range;
        const botDb = -rangeDb;
        const padTop = Math.round(h * (METER_PAD_TOP_PCT / 100));
        const padBottom = Math.round(h * (METER_PAD_BOTTOM_PCT / 100));
        const plotH = Math.max(1, h - padTop - padBottom);
        const grToY = (db) => {
            const c = clamp(db, botDb, topDb);
            const t = (topDb - c) / (topDb - botDb);
            return padTop + (t * plotH);
        };
        const waveToY = (db) => {
            if (db >= botDb) return grToY(db);
            const extraDbMax = (padBottom / Math.max(1.0e-6, plotH)) * (topDb - botDb);
            const minDb = botDb - extraDbMax;
            const c = clamp(db, minDb, botDb);
            const t = (botDb - c) / (topDb - botDb);
            return grToY(botDb) + t * plotH;
        };

        if (state.filled <= 1 || !state.outDb || !state.clipDb || !state.limitDb) {
            requestAnimationFrame(draw);
            return;
        }

        const n = state.cols;
        const filled = state.filled;
        const x0 = Math.max(0, n - filled);
        const xStep = 1;

        ctx.fillStyle = 'rgba(0, 0, 0, 0.06)';
        ctx.fillRect(0, 0, w, h);

        const tickStep = TICK_MODES[state.tickMode].step;
        if (tickStep > 0) {
            ctx.lineWidth = 1;
            ctx.strokeStyle = 'rgba(255, 255, 255, 0.06)';
            ctx.beginPath();
            for (let db = 0; db >= botDb; db -= tickStep) {
                const y = Math.round(grToY(db)) + 0.5;
                ctx.moveTo(0, y);
                ctx.lineTo(w, y);
            }
            ctx.stroke();

            ctx.fillStyle = 'rgb(160, 160, 160)';
            ctx.font = '10px JetBrains Mono, ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace';
            ctx.textAlign = 'right';
            ctx.textBaseline = 'middle';
            const majorStep = 6;
            for (let db = 0; db >= botDb; db -= majorStep) {
                const y = grToY(db);
                ctx.fillText(`${db}`, w - 6, y);
            }
        }

        if (state.showWaveform) {
            ctx.fillStyle = 'rgba(14, 165, 233, 0.18)';
            ctx.beginPath();
            ctx.moveTo(x0, h);
            for (let x = x0; x < n; x += xStep) {
                const j = x - x0;
                const idx = (state.write - filled + j + n) % n;
                ctx.lineTo(x, waveToY(state.outDb[idx]));
            }
            ctx.lineTo(n - 1, h);
            ctx.closePath();
            ctx.fill();

            ctx.lineWidth = 1.25;
            ctx.lineCap = 'round';
            ctx.lineJoin = 'round';
            ctx.strokeStyle = 'rgba(2, 132, 199, 0.85)';
            ctx.beginPath();
            for (let x = x0; x < n; x += xStep) {
                const j = x - x0;
                const idx = (state.write - filled + j + n) % n;
                const y = waveToY(state.outDb[idx]);
                if (x === x0) ctx.moveTo(x, y);
                else ctx.lineTo(x, y);
            }
            ctx.stroke();
        }

        if (state.lufsSt) {
            ctx.lineWidth = 1;
            ctx.lineCap = 'butt';
            ctx.lineJoin = 'round';
            ctx.strokeStyle = 'rgba(59, 130, 246, 0.55)';
            ctx.beginPath();
            let started = false;
            for (let x = x0; x < n; x += xStep) {
                const j = x - x0;
                const idx = (state.write - filled + j + n) % n;
                const v = state.lufsSt[idx];
                if (!Number.isFinite(v) || v <= -127) {
                    started = false;
                    continue;
                }
                const y = grToY(v);
                if (!started) {
                    ctx.moveTo(x, y);
                    started = true;
                } else {
                    ctx.lineTo(x, y);
                }
            }
            ctx.stroke();
        }

        const idxStart = (state.write - filled + n) % n;
        const clipStart = clamp(Math.min(0, state.clipDb[idxStart]), -rangeDb, 0);

        ctx.fillStyle = 'rgb(239, 68, 68)';
        ctx.beginPath();
        ctx.moveTo(x0, grToY(0));
        for (let x = x0; x < n; x += xStep) {
            const j = x - x0;
            const idx = (state.write - filled + j + n) % n;
            const clipDb = clamp(Math.min(0, state.clipDb[idx]), -rangeDb, 0);
            ctx.lineTo(x, grToY(clipDb));
        }
        ctx.lineTo(n - 1, grToY(0));
        ctx.closePath();
        ctx.fill();

        ctx.fillStyle = 'rgb(251, 191, 36)';
        ctx.beginPath();
        ctx.moveTo(x0, grToY(clipStart));
        for (let x = x0; x < n; x += xStep) {
            const j = x - x0;
            const idx = (state.write - filled + j + n) % n;
            const clipDb = clamp(Math.min(0, state.clipDb[idx]), -rangeDb, 0);
            const limitDb = clamp(Math.min(0, state.limitDb[idx]), -rangeDb, 0);
            const totalDb = clamp(clipDb + limitDb, -rangeDb, 0);
            ctx.lineTo(x, grToY(totalDb));
        }
        for (let x = n - 1; x >= x0; x -= xStep) {
            const j = x - x0;
            const idx = (state.write - filled + j + n) % n;
            const clipDb = clamp(Math.min(0, state.clipDb[idx]), -rangeDb, 0);
            ctx.lineTo(x, grToY(clipDb));
        }
        ctx.closePath();
        ctx.fill();

        requestAnimationFrame(draw);
    }

    function cycleTicks() {
        state.tickMode = (state.tickMode + 1) % TICK_MODES.length;
        updateControlLabels();
        saveSettings();
        state.dirty = true;
    }

    function cycleRange() {
        state.rangeMode = (state.rangeMode + 1) % RANGE_MODES.length;
        updateControlLabels();
        saveSettings();
        state.dirty = true;
    }

    function cycleSpeed() {
        state.speedMode = (state.speedMode + 1) % SPEED_MODES.length;
        state.windowSeconds = SPEED_MODES[state.speedMode].seconds;
        updateControlLabels();
        saveSettings();
        state.dirty = true;
    }

    return {
        init: ensure,
        update,
        cycleTicks,
        cycleRange,
        cycleSpeed,
        syncControls: updateControlLabels,
        getRangeDb: () => RANGE_MODES[state.rangeMode].range,
        getTickStep: () => TICK_MODES[state.tickMode].step,
        getPadTopPct: () => METER_PAD_TOP_PCT,
        getPadBottomPct: () => METER_PAD_BOTTOM_PCT
    };
})();
