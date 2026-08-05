/**
 * ESP32-C5 LED Webpage Controller - Client Logic
 * Handles user interactions, updates the dynamic virtual LED,
 * and communicates with the ESP32 Zephyr HTTP Server API.
 */

// Configuration
const CONFIG = {
    // API settings
    apiEndpoint: '/api/led',
    apiMode: 'json', // Options: 'query' (e.g. ?r=255&g=0&b=0&rainbow=0) or 'json' (e.g. {"r":255,"g":0,"b":0,"rainbow":false})
    apiMethod: 'POST',

    // Safety limit: wait at least this long (ms) between slider requests to avoid overwhelming the ESP32 socket
    debounceDelay: 120,

    // Automatically fallback to mock mode if requests fail (saves developer test time locally)
    autoSimulationFallback: true
};

// UI Elements
const els = {
    connectionStatus: document.getElementById('connection-status'),
    statusDot: document.querySelector('.status-dot'),
    statusText: document.querySelector('.status-text'),
    ledHalo: document.getElementById('led-halo'),
    ledLens: document.getElementById('led-lens'),
    rgbText: document.getElementById('rgb-text'),
    hexText: document.getElementById('hex-text'),
    rainbowToggle: document.getElementById('rainbow-toggle'),
    slidersPanel: document.getElementById('sliders-panel'),
    sliderR: document.getElementById('slider-r'),
    sliderG: document.getElementById('slider-g'),
    sliderB: document.getElementById('slider-b'),
    valR: document.getElementById('val-r'),
    valG: document.getElementById('val-g'),
    valB: document.getElementById('val-b'),
    colorPicker: document.getElementById('color-picker'),
    presetBtns: document.querySelectorAll('.preset-btn'),
    consoleLogs: document.getElementById('console-logs'),
    consoleToggle: document.getElementById('console-toggle'),
    consoleCard: document.querySelector('.console-card')
};

// State Variables
let state = {
    r: 0,
    g: 0,
    b: 0,
    rainbow: false,
    isSimulating: true, // starts in simulating unless we verify a real connection
    lastSentTimestamp: 0,
    pendingUpdateTimeout: null
};

// Initialize Application
document.addEventListener('DOMContentLoaded', () => {
    setupEventListeners();
    checkInitialConnection();
    updateUI();
});

// Setup Listeners
function setupEventListeners() {
    // Sliders
    const handleSliderInput = () => {
        state.r = parseInt(els.sliderR.value);
        state.g = parseInt(els.sliderG.value);
        state.b = parseInt(els.sliderB.value);

        // Turn off rainbow if user adjusts sliders
        if (state.rainbow) {
            state.rainbow = false;
            els.rainbowToggle.classList.remove('active');
            document.getElementById('mode-badge').textContent = 'Static';
        }

        updateUIValuesOnly();
        debouncedSendUpdate();
    };

    els.sliderR.addEventListener('input', handleSliderInput);
    els.sliderG.addEventListener('input', handleSliderInput);
    els.sliderB.addEventListener('input', handleSliderInput);

    // Rainbow Toggle
    els.rainbowToggle.addEventListener('click', () => {
        state.rainbow = !state.rainbow;
        if (state.rainbow) {
            els.rainbowToggle.classList.add('active');
            els.slidersPanel.classList.add('disabled');
            document.getElementById('mode-badge').textContent = 'Rainbow';
            logConsole("[SYS] Rainbow mode enabled (automatic cycling)", "system-log");
        } else {
            els.rainbowToggle.classList.remove('active');
            els.slidersPanel.classList.remove('disabled');
            document.getElementById('mode-badge').textContent = 'Static';
            logConsole("[SYS] Static color mode restored", "system-log");
        }
        updateUI();
        sendUpdateNow();
    });

    // Preset Buttons
    els.presetBtns.forEach(btn => {
        btn.addEventListener('click', (e) => {
            // Determine clicked element's target color
            const targetBtn = e.target.closest('.preset-btn');
            const hex = targetBtn.getAttribute('data-color');
            const rgb = hexToRgb(hex);

            if (rgb) {
                state.r = rgb.r;
                state.g = rgb.g;
                state.b = rgb.b;
                state.rainbow = false;

                // Update slider values
                els.sliderR.value = rgb.r;
                els.sliderG.value = rgb.g;
                els.sliderB.value = rgb.b;

                els.rainbowToggle.classList.remove('active');
                els.slidersPanel.classList.remove('disabled');
                document.getElementById('mode-badge').textContent = 'Static';

                logConsole(`[SYS] Preset selected: ${targetBtn.textContent.trim()} (${hex})`, "system-log");

                updateUI();
                sendUpdateNow();
            }
        });
    });

    // Color Picker Input
    els.colorPicker.addEventListener('input', (e) => {
        const hex = e.target.value;
        const rgb = hexToRgb(hex);
        if (rgb) {
            state.r = rgb.r;
            state.g = rgb.g;
            state.b = rgb.b;
            state.rainbow = false;

            els.sliderR.value = rgb.r;
            els.sliderG.value = rgb.g;
            els.sliderB.value = rgb.b;

            els.rainbowToggle.classList.remove('active');
            els.slidersPanel.classList.remove('disabled');
            document.getElementById('mode-badge').textContent = 'Static';

            updateUIValuesOnly();
            debouncedSendUpdate();
        }
    });

    els.colorPicker.addEventListener('change', (e) => {
        logConsole(`[SYS] Custom color chosen: ${e.target.value}`, "system-log");
        sendUpdateNow();
    });

    // Console Toggle
    els.consoleToggle.addEventListener('click', () => {
        els.consoleCard.classList.toggle('collapsed');
    });
}

// Check if running on ESP32 or locally
function checkInitialConnection() {
    // If hosted on file:// protocol, automatically run in Simulating/Mock Mode
    if (window.location.protocol === 'file:') {
        setSimulationMode(true, "Local File (Simulation)");
        return;
    }

    // Try a ping check or handshake to check server availability
    fetch('/api/led')
        .then(res => {
            if (res.ok) {
                setSimulationMode(false, "Connected");
                logConsole("[SYS] Connected to ESP32 HTTP Server!", "api-success");
            } else {
                throw new Error("HTTP connection check failed");
            }
        })
        .catch(err => {
            if (CONFIG.autoSimulationFallback) {
                setSimulationMode(true, "Offline (Simulation)");
                logConsole("[SYS] ESP32 not reachable. Fallback to Simulation Mode.", "system-log");
            } else {
                setSimulationMode(false, "Error / Disconnected");
                logConsole("[ERR] Failed to reach ESP32 endpoint", "api-error");
            }
        });
}

function setSimulationMode(isSimulating, statusText) {
    state.isSimulating = isSimulating;
    els.statusText.textContent = statusText;

    if (isSimulating) {
        els.statusDot.style.backgroundColor = '#eab308'; // Yellow
        els.statusDot.style.boxShadow = '0 0 10px #eab308';
        els.connectionStatus.title = "Operating in local simulation mode. API requests are mocked.";
    } else {
        els.statusDot.style.backgroundColor = '#2ad074'; // Green
        els.statusDot.style.boxShadow = '0 0 10px #2ad074';
        els.connectionStatus.title = "Connected to ESP32 device.";
    }
}

// UI Rendering Utilities
function updateUI() {
    // Sync slider texts
    els.valR.textContent = state.r;
    els.valG.textContent = state.g;
    els.valB.textContent = state.b;

    // Sync picker input value
    els.colorPicker.value = rgbToHex(state.r, state.g, state.b);

    // Sync labels
    els.rgbText.textContent = `rgb(${state.r}, ${state.g}, ${state.b})`;
    els.hexText.textContent = rgbToHex(state.r, state.g, state.b).toUpperCase();

    // Render Preview LED state
    if (state.rainbow) {
        els.ledLens.className = 'led-lens led-rainbow';
        els.ledHalo.className = 'led-halo led-rainbow';
        els.ledHalo.style.removeProperty('--led-color');
    } else {
        els.ledLens.className = 'led-lens led-active';
        els.ledHalo.className = 'led-halo led-active';

        const colorStr = `rgb(${state.r}, ${state.g}, ${state.b})`;
        // If LED is off, don't glow
        if (state.r === 0 && state.g === 0 && state.b === 0) {
            els.ledLens.classList.remove('led-active');
            els.ledHalo.classList.remove('led-active');
        } else {
            els.ledLens.style.setProperty('--led-color', colorStr);
            els.ledHalo.style.setProperty('--led-color', colorStr);
        }
    }
}

// Update UI values without full redraw of heavy CSS bindings
function updateUIValuesOnly() {
    els.valR.textContent = state.r;
    els.valG.textContent = state.g;
    els.valB.textContent = state.b;
    els.colorPicker.value = rgbToHex(state.r, state.g, state.b);
    els.rgbText.textContent = `rgb(${state.r}, ${state.g}, ${state.b})`;
    els.hexText.textContent = rgbToHex(state.r, state.g, state.b).toUpperCase();

    if (!state.rainbow) {
        const colorStr = `rgb(${state.r}, ${state.g}, ${state.b})`;
        if (state.r === 0 && state.g === 0 && state.b === 0) {
            els.ledLens.className = 'led-lens';
            els.ledHalo.className = 'led-halo';
        } else {
            els.ledLens.className = 'led-lens led-active';
            els.ledHalo.className = 'led-halo led-active';
            els.ledLens.style.setProperty('--led-color', colorStr);
            els.ledHalo.style.setProperty('--led-color', colorStr);
        }
    }
}

// Debounce controller to prevent network bottleneck
function debouncedSendUpdate() {
    const now = Date.now();
    const elapsed = now - state.lastSentTimestamp;

    if (state.pendingUpdateTimeout) {
        clearTimeout(state.pendingUpdateTimeout);
    }

    if (elapsed >= CONFIG.debounceDelay) {
        sendUpdateNow();
    } else {
        // Schedule final update when time is up
        state.pendingUpdateTimeout = setTimeout(() => {
            sendUpdateNow();
        }, CONFIG.debounceDelay - elapsed);
    }
}

// Send actual network command
function sendUpdateNow() {
    state.lastSentTimestamp = Date.now();

    // Prepare API URL and parameters
    let url = CONFIG.apiEndpoint;
    let requestOptions = {
        method: CONFIG.apiMethod,
        headers: {}
    };

    const isRainbowVal = state.rainbow ? 1 : 0;

    if (CONFIG.apiMode === 'query') {
        url += `?r=${state.r}&g=${state.g}&b=${state.b}&rainbow=${isRainbowVal}`;
    } else {
        requestOptions.headers['Content-Type'] = 'application/json';
        requestOptions.body = JSON.stringify({
            r: state.r,
            g: state.g,
            b: state.b,
            rainbow: state.rainbow
        });
    }

    const logParamStr = CONFIG.apiMode === 'query'
        ? `?r=${state.r}&g=${state.g}&b=${state.b}&rainbow=${isRainbowVal}`
        : JSON.stringify({ r: state.r, g: state.g, b: state.b, rainbow: state.rainbow });

    logConsole(`[API] ${CONFIG.apiMethod} ${CONFIG.apiEndpoint}${CONFIG.apiMode === 'query' ? logParamStr : ''}`, "api-request");

    // Execution
    if (state.isSimulating) {
        // Local simulation delay
        setTimeout(() => {
            logConsole(`[SYS] MOCK RESPONSE: LED successfully updated.`, "api-success");
        }, 30);
    } else {
        fetch(url, requestOptions)
            .then(res => {
                if (res.ok) {
                    logConsole(`[API] 200 OK - LED Updated.`, "api-success");
                } else {
                    logConsole(`[ERR] ${res.status} ${res.statusText} - LED update failed.`, "api-error");
                }
            })
            .catch(err => {
                logConsole(`[ERR] Connection error: ${err.message}`, "api-error");
                // If real connection fails, fallback to simulation so page stays interactive
                if (CONFIG.autoSimulationFallback && !state.isSimulating) {
                    setSimulationMode(true, "Fallback (Simulating)");
                    logConsole("[SYS] Disconnection detected. Switched to Simulation.", "system-log");
                }
            });
    }
}

// Log utility
function logConsole(msg, typeClass = '') {
    const logLine = document.createElement('div');
    logLine.className = `log-line ${typeClass}`;

    // Add Timestamp
    const d = new Date();
    const timeStr = `${d.getHours().toString().padStart(2, '0')}:${d.getMinutes().toString().padStart(2, '0')}:${d.getSeconds().toString().padStart(2, '0')}.${(d.getMilliseconds() / 10).toFixed(0).padStart(2, '0')}`;

    logLine.textContent = `[${timeStr}] ${msg}`;
    els.consoleLogs.appendChild(logLine);

    // Auto-scroll
    els.consoleLogs.scrollTop = els.consoleLogs.scrollHeight;

    // Keep max 40 log entries to save device DOM memory
    if (els.consoleLogs.childNodes.length > 40) {
        els.consoleLogs.removeChild(els.consoleLogs.firstChild);
    }
}

// Color conversion helpers
function hexToRgb(hex) {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : null;
}

function rgbToHex(r, g, b) {
    const componentToHex = (c) => {
        const hex = c.toString(16);
        return hex.length === 1 ? "0" + hex : hex;
    };
    return "#" + componentToHex(r) + componentToHex(g) + componentToHex(b);
}
