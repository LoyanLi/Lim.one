window.AudioPlugin = window.AudioPlugin || {};

AudioPlugin.Controls = (function() {
    const State = AudioPlugin.State;
    const IPC = AudioPlugin.IPC;
    const UI = AudioPlugin.UI;

    const DEFAULTS = {
        in_gain: 0.0,
        drive: 0.0,
        knee: 0.0,
        character: 0.5,
        limiter_drive: 0.0,
        ceiling: -0.1,
        out_gain: -0.1,
        clipper_mode: 0,
        delta: false,
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

    const PARAMS = {
        in_gain: { min: -24, max: 24, step: 0.01 },
        drive: { min: 0, max: 18, step: 0.01 },
        knee: { min: 0, max: 1, step: 0.001 },
        limiter_drive: { min: 0, max: 18, step: 0.01 },
        character: { min: 0, max: 1, step: 0.001 },
        ceiling: { min: -1, max: 0, step: 0.001 },
        out_gain: { min: -24, max: 1, step: 0.01, pixelsPerRange: 360 },
        clipper_mode: { min: 0, max: 1, step: 1 },

        lookahead: { min: 0, max: 5, step: 0.01 },

        modern_hold_ms: { min: 0, max: 10, step: 0.01 },
        modern_hold_release_ms: { min: 0.05, max: 10, step: 0.01 },
        modern_attack_tau_div: { min: 0.25, max: 8, step: 0.01 },
        modern_release_smooth_base_ms: { min: 0, max: 20, step: 0.01 },
        modern_release_smooth_range_ms: { min: 0, max: 50, step: 0.01 },
        modern_adapt_fast_strength: { min: 0, max: 8, step: 0.01 },
        modern_adapt_slow_strength: { min: 0, max: 16, step: 0.01 },
        modern_sc_hpf_hz: { min: 20, max: 400, step: 1 },
        modern_ratio_base: { min: 0.1, max: 3, step: 0.01 },
        modern_ratio_slope: { min: 0, max: 1.5, step: 0.01 },
        modern_transient_mix: { min: 0, max: 1, step: 0.001 },
        modern_soft_safety_strength: { min: 0, max: 20, step: 0.01 },
        modern_release_fast_ms: { min: 0.05, max: 20, step: 0.01 },
        modern_release_slow_ms: { min: 0.1, max: 200, step: 0.1 },
        modern_link_transients: { min: 0, max: 1, step: 0.001 },
        modern_link_release: { min: 0, max: 1, step: 0.001 }
    };

    const PIXELS_PER_RANGE = 180;
    let active = null;
    let updateCheckPending = null;
    let updateCheckTimeoutHandle = 0;
    let updateCheckSeq = 0;

    function clamp(value, min, max) {
        return Math.max(min, Math.min(max, value));
    }

    function lerp(a, b, t) {
        return a + (b - a) * t;
    }

    function expLerp(a, b, t) {
        if (a <= 0 || b <= 0) return lerp(a, b, t);
        return a * Math.pow(b / a, t);
    }

    function quantize(value, step) {
        if (!step) return value;
        return Math.round(value / step) * step;
    }

    function rangeFor(paramId) {
        const p = PARAMS[paramId];
        return p ? (p.max - p.min) : 1;
    }

    function deltaToValue(deltaY, paramId, fine) {
        const range = rangeFor(paramId);
        const p = PARAMS[paramId];
        const ppr = (p && Number.isFinite(p.pixelsPerRange) && p.pixelsPerRange > 0) ? p.pixelsPerRange : PIXELS_PER_RANGE;
        const scale = range / ppr;
        const modifier = fine ? 0.1 : 1;
        return -deltaY * scale * modifier;
    }

    function setParam(paramId, value) {
        State.params[paramId] = value;
        IPC.emit('beginGesture', { paramId });
        IPC.emitParamUpdate(paramId, value);
        IPC.emit('endGesture', { paramId });
        UI.updateParamUI(paramId, value);
    }


    function wheelAdjust(e, paramId) {
        const p = PARAMS[paramId];
        if (!p) return;
        e.preventDefault();
        if (e.altKey === true) {
            const def = DEFAULTS[paramId];
            if (def !== undefined) setParam(paramId, def);
            return;
        }
        const direction = e.deltaY > 0 ? -1 : 1;
        const fine = e.shiftKey === true;
        const step = fine ? Math.max(p.step * 0.1, 1.0e-6) : p.step;
        let newVal = State.params[paramId] + direction * step;
        newVal = clamp(newVal, p.min, p.max);
        newVal = quantize(newVal, step);
        if (Math.abs(newVal - State.params[paramId]) < step * 0.5) return;
        State.params[paramId] = newVal;
        UI.updateKnobUI(paramId, newVal);
        IPC.emitParamUpdate(paramId, newVal);
    }

    function beginDrag(e, paramId) {
        if (e.button !== 0) return;
        const p = PARAMS[paramId];
        if (!p) return;
        e.preventDefault();
        if (e.altKey === true) {
            const def = DEFAULTS[paramId];
            if (def !== undefined) setParam(paramId, def);
            return;
        }
        State.activeDragParam = paramId;
        active = {
            paramId,
            pointerId: e.pointerId,
            startY: e.clientY,
            startVal: State.params[paramId]
        };
        if (e.currentTarget && e.currentTarget.setPointerCapture) {
            e.currentTarget.setPointerCapture(e.pointerId);
        }
        if (document.body) document.body.style.cursor = 'ns-resize';
        IPC.emit('beginGesture', { paramId });
    }

    function updateDrag(e) {
        if (!active || e.pointerId !== active.pointerId) return;
        const paramId = active.paramId;
        const p = PARAMS[paramId];
        if (!p) return;
        const fine = e.shiftKey === true;
        const step = fine ? Math.max(p.step * 0.1, 1.0e-6) : p.step;
        const deltaY = e.clientY - active.startY;
        let newVal = active.startVal + deltaToValue(deltaY, paramId, fine);
        newVal = clamp(newVal, p.min, p.max);
        newVal = quantize(newVal, step);
        if (Math.abs(newVal - State.params[paramId]) < step * 0.5) return;
        State.params[paramId] = newVal;
        UI.updateKnobUI(paramId, newVal);
        IPC.throttledEmitParamUpdate(paramId, newVal);
    }

    function endDrag(e) {
        if (!active || e.pointerId !== active.pointerId) return;
        const paramId = active.paramId;
        if (document.body) document.body.style.cursor = '';
        State.activeDragParam = null;
        IPC.emitParamUpdate(paramId, State.params[paramId]);
        IPC.emit('endGesture', { paramId });
        active = null;
    }

    function toggleTruePeak() {
        setParam('true_peak', !State.params.true_peak);
    }

    function toggleDelta() {
        setParam('delta', !State.params.delta);
    }

    function bindButton(id, handler) {
        const el = document.getElementById(id);
        if (!el) return;
        el.addEventListener('click', handler);
    }

    function parseSemver(v) {
        const s = (v || '').toString().trim();
        const m = s.match(/^(\d+)\.(\d+)\.(\d+)/);
        if (!m) return null;
        return [parseInt(m[1], 10), parseInt(m[2], 10), parseInt(m[3], 10)];
    }

    function compareSemver(a, b) {
        const pa = parseSemver(a);
        const pb = parseSemver(b);
        if (!pa && !pb) return 0;
        if (!pa) return -1;
        if (!pb) return 1;
        for (let i = 0; i < 3; i++) {
            if (pa[i] !== pb[i]) return pa[i] > pb[i] ? 1 : -1;
        }
        return 0;
    }

    function setUpdateUiStatus(text, kind) {
        const el = document.getElementById('settings-update-status');
        if (!el) return;
        el.textContent = text || '';
        el.classList.toggle('text-red-600', kind === 'error');
        el.classList.toggle('text-gray-400', kind !== 'error');
    }

    function setUpdateCheckButtonBusy(isBusy) {
        const btn = document.getElementById('btn-check-update');
        if (btn) {
            btn.disabled = !!isBusy;
            btn.classList.toggle('opacity-60', !!isBusy);
            btn.classList.toggle('cursor-not-allowed', !!isBusy);
        }
    }

    function checkForUpdates() {
        setUpdateCheckButtonBusy(true);

        try {
            setUpdateUiStatus('Checking…', 'message');
            const currentVersion = (State.settingsInfo && State.settingsInfo.version) ? String(State.settingsInfo.version) : '';
            updateCheckSeq = (updateCheckSeq + 1) | 0;
            const requestId = updateCheckSeq <= 0 ? 1 : updateCheckSeq;
            const url = `https://lumlayers.com/apps/limone/CHANGELOG.json?t=${Date.now()}`;

            updateCheckPending = { requestId, currentVersion };

            if (updateCheckTimeoutHandle) {
                clearTimeout(updateCheckTimeoutHandle);
                updateCheckTimeoutHandle = 0;
            }

            updateCheckTimeoutHandle = setTimeout(() => {
                if (!updateCheckPending || updateCheckPending.requestId !== requestId) return;
                updateCheckPending = null;
                updateCheckTimeoutHandle = 0;
                setUpdateUiStatus('Updates load failed: Timeout', 'error');
                if (UI && UI.showAlert) UI.showAlert('Check updates failed', 'Timeout', 'error');
                setUpdateCheckButtonBusy(false);
            }, 12000);

            if (IPC && IPC.emit) {
                IPC.emit('checkForUpdates', { url, requestId });
            } else {
                throw new Error('IPC unavailable');
            }
        } catch (e) {
            const msg = (e && e.message) ? String(e.message) : String(e);
            setUpdateUiStatus(`Updates load failed: ${msg}`, 'error');
            if (UI && UI.showAlert) {
                UI.showAlert('Check updates failed', msg, 'error');
            }
            setUpdateCheckButtonBusy(false);
        }
    }

    function handleUpdateCheckResult(data) {
        const requestId = data && data.requestId ? parseInt(data.requestId, 10) : 0;
        if (!updateCheckPending || updateCheckPending.requestId !== requestId) return;

        const currentVersion = updateCheckPending.currentVersion || '';
        updateCheckPending = null;

        if (updateCheckTimeoutHandle) {
            clearTimeout(updateCheckTimeoutHandle);
            updateCheckTimeoutHandle = 0;
        }

        setUpdateCheckButtonBusy(false);

        const ok = !!(data && data.ok);
        if (!ok) {
            const msg = (data && data.error) ? String(data.error) : 'Unknown error';
            setUpdateUiStatus(`Updates load failed: ${msg}`, 'error');
            if (UI && UI.showAlert) UI.showAlert('Check updates failed', msg, 'error');
            return;
        }

        const payload = data && data.payload ? data.payload : null;
        const latest = payload && (payload.latest || (payload.releases && payload.releases[0] && payload.releases[0].version))
            ? String(payload.latest || payload.releases[0].version)
            : '';
        if (!latest) {
            setUpdateUiStatus('Updates load failed: Invalid CHANGELOG.json', 'error');
            if (UI && UI.showAlert) UI.showAlert('Check updates failed', 'Invalid CHANGELOG.json', 'error');
            return;
        }

        const cmp = compareSemver(latest, currentVersion);
        if (cmp > 0) {
            const rel = payload && payload.releases && payload.releases.find ? payload.releases.find(r => String(r.version) === latest) : null;
            const notes = rel && Array.isArray(rel.notes) ? rel.notes.slice(0, 6).join('\n') : '';
            setUpdateUiStatus(`Update available: ${latest}`, 'message');
            if (UI && UI.showAlert) {
                UI.showAlert('Update available', `Current: ${currentVersion || '-'}\nLatest: ${latest}${notes ? `\n\n${notes}` : ''}`, 'message');
            }
        } else {
            setUpdateUiStatus(`Up to date: ${latest}`, 'message');
            if (UI && UI.showAlert) {
                UI.showAlert('Up to date', `Current: ${currentVersion || '-'}\nLatest: ${latest}`, 'message');
            }
        }
    }

    function init() {
        const gestureActive = new Set();

        const knobs = document.querySelectorAll('[data-drag-param]');
        knobs.forEach((el) => {
            const paramId = el.getAttribute('data-drag-param');
            if (!paramId) return;
            if (!PARAMS[paramId]) return;
            el.style.touchAction = 'none';
            el.addEventListener('pointerdown', (e) => beginDrag(e, paramId));
            el.addEventListener('wheel', (e) => wheelAdjust(e, paramId), { passive: false });
        });

        window.addEventListener('pointermove', updateDrag);
        window.addEventListener('pointerup', endDrag);
        window.addEventListener('pointercancel', endDrag);

        const sliders = document.querySelectorAll('input[type="range"][data-param]');
        sliders.forEach((el) => {
            const paramId = el.getAttribute('data-param');
            if (!paramId) return;
            if (!PARAMS[paramId]) return;

            el.addEventListener('pointerdown', (e) => {
                if (e.altKey === true) {
                    e.preventDefault();
                    const def = DEFAULTS[paramId];
                    if (def !== undefined) setParam(paramId, def);
                    el.value = String(State.params[paramId]);
                    return;
                }
                gestureActive.add(paramId);
                IPC.emit('beginGesture', { paramId });
            });

            el.addEventListener('input', () => {
                const p = PARAMS[paramId];
                let v = parseFloat(el.value);
                if (!Number.isFinite(v)) return;
                v = clamp(v, p.min, p.max);
                v = quantize(v, p.step);
                State.params[paramId] = v;
                UI.updateKnobUI(paramId, v);
                IPC.throttledEmitParamUpdate(paramId, v);
            });

            el.addEventListener('pointerup', () => {
                if (!gestureActive.has(paramId)) return;
                gestureActive.delete(paramId);
                IPC.emitParamUpdate(paramId, State.params[paramId]);
                IPC.emit('endGesture', { paramId });
            });

            el.addEventListener('pointercancel', () => {
                if (!gestureActive.has(paramId)) return;
                gestureActive.delete(paramId);
                IPC.emitParamUpdate(paramId, State.params[paramId]);
                IPC.emit('endGesture', { paramId });
            });

            // Prevent scroll wheel value change, but allow page scrolling
        el.addEventListener('wheel', (e) => {
            e.preventDefault();
            e.stopPropagation();
            
            // Manually scroll the container
            const scrollable = el.closest('.overflow-y-auto');
            if (scrollable) {
                scrollable.scrollTop += e.deltaY;
            }
        }, { passive: false });
        });

        bindButton('btn-true-peak', toggleTruePeak);
        bindButton('btn-delta', toggleDelta);
        bindButton('btn-reset-loudness', () => {
             IPC.emit('resetLoudness', 0);
        });
        bindButton('btn-reset-links', () => {
             IPC.emit('resetLinks', 0);
        });

        const linkButtons = document.querySelectorAll('button[data-link-param]');
        linkButtons.forEach(btn => {
            btn.addEventListener('click', (e) => {
                e.stopPropagation();
                const paramId = btn.getAttribute('data-link-param');
                const currentVal = (State.params[paramId] || 0) > 0.5 ? 1 : 0;
                setParam(paramId, currentVal === 1 ? 0 : 1);
            });
        });

        bindButton('btn-adv-menu', () => {
            const newOpen = !State.drawerOpen;
            if (AudioPlugin && AudioPlugin.UI && AudioPlugin.UI.setAdvancedOpen) {
                AudioPlugin.UI.setAdvancedOpen(newOpen);
            }
            if (newOpen && AudioPlugin && AudioPlugin.UI && AudioPlugin.UI.fullUpdate) {
                AudioPlugin.UI.fullUpdate();
            }
        });

        bindButton('btn-advanced-close', () => {
            if (AudioPlugin && AudioPlugin.UI && AudioPlugin.UI.setAdvancedOpen) {
                AudioPlugin.UI.setAdvancedOpen(false);
            }
        });
        bindButton('advanced-backdrop', () => {
            if (AudioPlugin && AudioPlugin.UI && AudioPlugin.UI.setAdvancedOpen) {
                AudioPlugin.UI.setAdvancedOpen(false);
            }
        });

        bindButton('btn-settings', () => {
            const panel = document.getElementById('settings-panel');
            const isOpen = panel ? !panel.classList.contains('hidden') : false;
            if (State.licenseLocked && isOpen) return;
            if (AudioPlugin && AudioPlugin.UI && AudioPlugin.UI.setSettingsOpen) {
                AudioPlugin.UI.setSettingsOpen(!isOpen);
            }
        });
        bindButton('btn-settings-close', () => {
            if (State.licenseLocked) return;
            if (AudioPlugin && AudioPlugin.UI && AudioPlugin.UI.setSettingsOpen) {
                AudioPlugin.UI.setSettingsOpen(false);
            }
        });
        bindButton('settings-backdrop', () => {
            if (State.licenseLocked) return;
            if (AudioPlugin && AudioPlugin.UI && AudioPlugin.UI.setSettingsOpen) {
                AudioPlugin.UI.setSettingsOpen(false);
            }
        });

        bindButton('btn-viz-ticks', () => {
            if (AudioPlugin && AudioPlugin.Visualizer && AudioPlugin.Visualizer.cycleTicks) {
                AudioPlugin.Visualizer.cycleTicks();
            }
        });
        bindButton('btn-viz-range', () => {
            if (AudioPlugin && AudioPlugin.Visualizer && AudioPlugin.Visualizer.cycleRange) {
                AudioPlugin.Visualizer.cycleRange();
            }
        });
        bindButton('btn-viz-speed', () => {
            if (AudioPlugin && AudioPlugin.Visualizer && AudioPlugin.Visualizer.cycleSpeed) {
                AudioPlugin.Visualizer.cycleSpeed();
            }
        });
        
        const uiScaleSelect = document.getElementById('select-ui-scale');
        if (uiScaleSelect) {
            uiScaleSelect.addEventListener('change', () => {
                const v = parseFloat(uiScaleSelect.value);
                if (!Number.isFinite(v)) return;
                setParam('ui_scale', v);
            });
        }

        const clipperSelect = document.getElementById('select-clipper-mode');
        if (clipperSelect) {
            clipperSelect.addEventListener('pointerdown', (e) => e.stopPropagation());
            clipperSelect.addEventListener('change', () => {
                const v = parseInt(clipperSelect.value, 10);
                if (!Number.isFinite(v)) return;
                setParam('clipper_mode', v);
            });
        }

        const limiterSelect = document.getElementById('select-limiter-mode');
        if (limiterSelect) {
            limiterSelect.addEventListener('pointerdown', (e) => e.stopPropagation());
            limiterSelect.addEventListener('change', () => {
                const v = parseInt(limiterSelect.value, 10);
                if (!Number.isFinite(v)) return;
                setParam('limiter_mode', v);
            });
        }

        const oversamplingSelect = document.getElementById('select-oversampling');
        if (oversamplingSelect) {
            oversamplingSelect.addEventListener('change', () => {
                const v = parseInt(oversamplingSelect.value, 10);
                if (!Number.isFinite(v)) return;
                setParam('oversampling', v);
            });
        }

        bindButton('btn-check-update', checkForUpdates);

        const closeAlert = () => {
            if (AudioPlugin && AudioPlugin.UI && AudioPlugin.UI.setAlertOpen) {
                AudioPlugin.UI.setAlertOpen(false);
            }
        };
        bindButton('btn-alert-close', closeAlert);
        bindButton('btn-alert-ok', closeAlert);

        bindButton('alert-backdrop', (evt) => {
            if (evt && evt.target && evt.target.id === 'alert-backdrop') closeAlert();
        });
    }

    return {
        init,
        toggleTruePeak,
        handleUpdateCheckResult
    };
})();
