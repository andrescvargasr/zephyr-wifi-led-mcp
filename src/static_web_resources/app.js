/**
 * ESP32 16x16 LED Matrix Webpage Controller - Client Logic
 * Manages 256 addressable LED pixel states, grid interactions, paint mode,
 * and communicates with the ESP32 Zephyr HTTP Server API.
 */

const MATRIX_SIZE = 16;
const TOTAL_PIXELS = MATRIX_SIZE * MATRIX_SIZE;

const CONFIG = {
    apiEndpoint: '/api/led',
    apiMode: 'json',
    apiMethod: 'POST',
    debounceDelay: 100,
    autoSimulationFallback: true
};

// UI Element Cache
const els = {
    connectionStatus: document.getElementById('connection-status'),
    statusDot: document.querySelector('.status-dot'),
    statusText: document.querySelector('.status-text'),
    matrixGrid: document.getElementById('matrix-grid'),
    matrixTooltip: document.getElementById('matrix-tooltip'),
    pixelBadge: document.getElementById('pixel-badge'),
    selectedPixelLabel: document.getElementById('selected-pixel-label'),
    selectedLedTitle: document.getElementById('selected-led-title'),
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
    toolPaint: document.getElementById('tool-paint'),
    toolFillAll: document.getElementById('tool-fill-all'),
    toolClearAll: document.getElementById('tool-clear-all'),
    consoleLogs: document.getElementById('console-logs'),
    consoleToggle: document.getElementById('console-toggle'),
    consoleCard: document.querySelector('.console-card')
};

// State Variables
let state = {
    matrix: Array.from({ length: TOTAL_PIXELS }, () => ({ r: 0, g: 0, b: 0 })),
    selectedIndex: 0,
    paintMode: false,
    isMouseDown: false,
    rainbow: false,
    isSimulating: true,
    lastSentTimestamp: 0,
    pendingUpdateTimeout: null
};

// Initialize Application
document.addEventListener('DOMContentLoaded', () => {
    buildMatrixGrid();
    setupEventListeners();
    checkInitialConnection();
    selectPixel(0);
});

// Build 16x16 Grid Elements
function buildMatrixGrid() {
    els.matrixGrid.innerHTML = '';
    for (let i = 0; i < TOTAL_PIXELS; i++) {
        const pixel = document.createElement('div');
        pixel.className = 'matrix-pixel';
        pixel.dataset.index = i;
        const row = Math.floor(i / MATRIX_SIZE);
        const col = i % MATRIX_SIZE;
        pixel.dataset.row = row;
        pixel.dataset.col = col;
        els.matrixGrid.appendChild(pixel);
    }
}

// Setup Event Listeners
function setupEventListeners() {
    // Matrix Pixel Clicks & Drag Painting
    document.addEventListener('mousedown', () => { state.isMouseDown = true; });
    document.addEventListener('mouseup', () => { state.isMouseDown = false; });

    els.matrixGrid.addEventListener('click', (e) => {
        const pixel = e.target.closest('.matrix-pixel');
        if (pixel) {
            const idx = parseInt(pixel.dataset.index);
            selectPixel(idx);
        }
    });

    els.matrixGrid.addEventListener('mouseover', (e) => {
        const pixel = e.target.closest('.matrix-pixel');
        if (pixel) {
            const idx = parseInt(pixel.dataset.index);
            const row = pixel.dataset.row;
            const col = pixel.dataset.col;

            // Show Tooltip
            els.matrixTooltip.textContent = `LED #${idx} (R${row}, C${col})`;
            els.matrixTooltip.classList.add('visible');

            // Position tooltip relative to matrix container
            const gridRect = els.matrixGrid.getBoundingClientRect();
            const pixelRect = pixel.getBoundingClientRect();
            const left = pixelRect.left - gridRect.left + (pixelRect.width / 2) - 35;
            const top = pixelRect.top - gridRect.top - 28;
            els.matrixTooltip.style.left = `${left}px`;
            els.matrixTooltip.style.top = `${top}px`;

            // If mouse is down & Paint mode enabled, paint this pixel
            if (state.isMouseDown && state.paintMode) {
                paintPixel(idx, getActiveColor());
            }
        }
    });

    els.matrixGrid.addEventListener('mouseleave', () => {
        els.matrixTooltip.classList.remove('visible');
    });

    // Tool Buttons
    els.toolPaint.addEventListener('click', () => {
        state.paintMode = !state.paintMode;
        if (state.paintMode) {
            els.toolPaint.classList.add('active');
            logConsole("[SYS] Paint Mode ON: Click & drag to draw on matrix", "system-log");
        } else {
            els.toolPaint.classList.remove('active');
            logConsole("[SYS] Paint Mode OFF", "system-log");
        }
    });

    els.toolFillAll.addEventListener('click', () => {
        const color = getActiveColor();
        for (let i = 0; i < TOTAL_PIXELS; i++) {
            state.matrix[i] = { ...color };
            updatePixelDOM(i);
        }
        logConsole(`[SYS] Filled all 256 LEDs with rgb(${color.r},${color.g},${color.b})`, "system-log");
        state.selectedIndex = TOTAL_PIXELS;
        updatePreviewLens(color.r, color.g, color.b);
        sendUpdateNow();
    });

    els.toolClearAll.addEventListener('click', () => {
        for (let i = 0; i < TOTAL_PIXELS; i++) {
            state.matrix[i] = { r: 0, g: 0, b: 0 };
            updatePixelDOM(i);
        }
        logConsole("[SYS] Cleared matrix grid (All LEDs OFF)", "system-log");
        state.selectedIndex = TOTAL_PIXELS;
        updatePreviewLens(0, 0, 0);
        sendUpdateNow();
    });

    // Sliders Input
    const handleSliderInput = () => {
        const r = parseInt(els.sliderR.value);
        const g = parseInt(els.sliderG.value);
        const b = parseInt(els.sliderB.value);

        if (state.rainbow) {
            state.rainbow = false;
            els.rainbowToggle.classList.remove('active');
            document.getElementById('mode-badge').textContent = 'Single LED';
        }

        // Apply to current pixel
        state.matrix[state.selectedIndex] = { r, g, b };
        updatePixelDOM(state.selectedIndex);
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
            logConsole("[SYS] Rainbow mode enabled", "system-log");
        } else {
            els.rainbowToggle.classList.remove('active');
            els.slidersPanel.classList.remove('disabled');
            document.getElementById('mode-badge').textContent = 'Single LED';
            logConsole("[SYS] Single LED color mode restored", "system-log");
        }
        updateUI();
        sendUpdateNow();
    });

    // Color Presets
    els.presetBtns.forEach(btn => {
        btn.addEventListener('click', (e) => {
            const targetBtn = e.target.closest('.preset-btn');
            const hex = targetBtn.getAttribute('data-color');
            const rgb = hexToRgb(hex);

            if (rgb) {
                state.matrix[state.selectedIndex] = { ...rgb };
                state.rainbow = false;

                els.sliderR.value = rgb.r;
                els.sliderG.value = rgb.g;
                els.sliderB.value = rgb.b;

                els.rainbowToggle.classList.remove('active');
                els.slidersPanel.classList.remove('disabled');
                document.getElementById('mode-badge').textContent = 'Single LED';

                updatePixelDOM(state.selectedIndex);
                updateUI();
                logConsole(`[SYS] Preset applied to LED #${state.selectedIndex}: ${targetBtn.textContent.trim()} (${hex})`, "system-log");
                sendUpdateNow();
            }
        });
    });

    // Color Picker
    els.colorPicker.addEventListener('input', (e) => {
        const hex = e.target.value;
        const rgb = hexToRgb(hex);
        if (rgb) {
            state.matrix[state.selectedIndex] = { ...rgb };
            state.rainbow = false;

            els.sliderR.value = rgb.r;
            els.sliderG.value = rgb.g;
            els.sliderB.value = rgb.b;

            els.rainbowToggle.classList.remove('active');
            els.slidersPanel.classList.remove('disabled');

            updatePixelDOM(state.selectedIndex);
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

// Select a Pixel in the Matrix
function selectPixel(index) {
    state.selectedIndex = index;
    const pixels = els.matrixGrid.children;

    for (let i = 0; i < pixels.length; i++) {
        if (i === index) {
            pixels[i].classList.add('selected');
        } else {
            pixels[i].classList.remove('selected');
        }
    }

    const row = Math.floor(index / MATRIX_SIZE);
    const col = index % MATRIX_SIZE;
    els.selectedPixelLabel.textContent = `LED #${index} (R${row}, C${col})`;
    els.selectedLedTitle.textContent = `Active LED #${index}`;

    // Load active pixel's color into controls
    const curColor = state.matrix[index];
    els.sliderR.value = curColor.r;
    els.sliderG.value = curColor.g;
    els.sliderB.value = curColor.b;

    updateUI();
}

// Paint Pixel with Color
function paintPixel(index, color) {
    state.matrix[index] = { ...color };
    updatePixelDOM(index);
    if (index === state.selectedIndex) {
        updateUI();
    }
    debouncedSendUpdate();
}

// Update Single Pixel DOM styling
function updatePixelDOM(index) {
    const pixel = els.matrixGrid.children[index];
    if (!pixel) return;

    const { r, g, b } = state.matrix[index];
    const colorStr = `rgb(${r}, ${g}, ${b})`;

    if (r === 0 && g === 0 && b === 0) {
        pixel.style.removeProperty('--pixel-bg');
        pixel.style.removeProperty('--pixel-color');
        pixel.classList.remove('pixel-on');
    } else {
        pixel.style.setProperty('--pixel-bg', colorStr);
        pixel.style.setProperty('--pixel-color', colorStr);
        pixel.classList.add('pixel-on');
    }
}

// Active Color helper
function getActiveColor() {
    return {
        r: parseInt(els.sliderR.value),
        g: parseInt(els.sliderG.value),
        b: parseInt(els.sliderB.value)
    };
}

// Network Connection Check
function checkInitialConnection() {
    if (window.location.protocol === 'file:') {
        setSimulationMode(true, "Local File (Simulation)");
        return;
    }

    fetch('/api/led')
        .then(res => {
            if (res.ok) {
                setSimulationMode(false, "Connected");
                logConsole("[SYS] Connected to ESP32 HTTP Server!", "api-success");
            } else {
                throw new Error("HTTP connection check failed");
            }
        })
        .catch(() => {
            if (CONFIG.autoSimulationFallback) {
                setSimulationMode(true, "Offline (Simulation)");
                logConsole("[SYS] ESP32 not reachable. Fallback to Simulation Mode.", "system-log");
            } else {
                setSimulationMode(false, "Disconnected");
                logConsole("[ERR] Failed to reach ESP32 endpoint", "api-error");
            }
        });
}

function setSimulationMode(isSimulating, statusText) {
    state.isSimulating = isSimulating;
    els.statusText.textContent = statusText;

    if (isSimulating) {
        els.statusDot.style.backgroundColor = '#eab308';
        els.statusDot.style.boxShadow = '0 0 8px #eab308';
    } else {
        els.statusDot.style.backgroundColor = '#2ad074';
        els.statusDot.style.boxShadow = '0 0 8px #2ad074';
    }
}

// UI Rendering Utilities
function updateUI() {
    const curColor = state.matrix[state.selectedIndex];
    els.valR.textContent = curColor.r;
    els.valG.textContent = curColor.g;
    els.valB.textContent = curColor.b;

    els.colorPicker.value = rgbToHex(curColor.r, curColor.g, curColor.b);
    els.rgbText.textContent = `rgb(${curColor.r}, ${curColor.g}, ${curColor.b})`;
    els.hexText.textContent = rgbToHex(curColor.r, curColor.g, curColor.b).toUpperCase();

    updatePreviewLens(curColor.r, curColor.g, curColor.b);
}

function updateUIValuesOnly() {
    const curColor = state.matrix[state.selectedIndex];
    els.valR.textContent = curColor.r;
    els.valG.textContent = curColor.g;
    els.valB.textContent = curColor.b;

    els.colorPicker.value = rgbToHex(curColor.r, curColor.g, curColor.b);
    els.rgbText.textContent = `rgb(${curColor.r}, ${curColor.g}, ${curColor.b})`;
    els.hexText.textContent = rgbToHex(curColor.r, curColor.g, curColor.b).toUpperCase();

    updatePreviewLens(curColor.r, curColor.g, curColor.b);
}

function updatePreviewLens(r, g, b) {
    if (state.rainbow) {
        els.ledLens.className = 'led-lens led-rainbow';
        els.ledHalo.className = 'led-halo led-rainbow';
    } else {
        const colorStr = `rgb(${r}, ${g}, ${b})`;
        if (r === 0 && g === 0 && b === 0) {
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

// Debounced Network Updates
function debouncedSendUpdate() {
    const now = Date.now();
    const elapsed = now - state.lastSentTimestamp;

    if (state.pendingUpdateTimeout) {
        clearTimeout(state.pendingUpdateTimeout);
    }

    if (elapsed >= CONFIG.debounceDelay) {
        sendUpdateNow();
    } else {
        state.pendingUpdateTimeout = setTimeout(() => {
            sendUpdateNow();
        }, CONFIG.debounceDelay - elapsed);
    }
}

function sendUpdateNow() {
    state.lastSentTimestamp = Date.now();
    const curColor = state.matrix[state.selectedIndex];

    const bodyData = {
        r: curColor.r,
        g: curColor.g,
        b: curColor.b,
        index: state.selectedIndex,
        rainbow: state.rainbow
    };

    const requestOptions = {
        method: CONFIG.apiMethod,
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(bodyData)
    };

    logConsole(`[API] POST ${CONFIG.apiEndpoint} ${JSON.stringify(bodyData)}`, "api-request");

    if (state.isSimulating) {
        setTimeout(() => {
            logConsole(`[SYS] MOCK RESPONSE: LED #${state.selectedIndex} updated.`, "api-success");
        }, 20);
    } else {
        fetch(CONFIG.apiEndpoint, requestOptions)
            .then(res => {
                if (res.ok) {
                    logConsole(`[API] 200 OK - LED #${state.selectedIndex} updated.`, "api-success");
                } else {
                    logConsole(`[ERR] ${res.status} ${res.statusText} - LED update failed.`, "api-error");
                }
            })
            .catch(err => {
                logConsole(`[ERR] Connection error: ${err.message}`, "api-error");
            });
    }
}

// Log utility
function logConsole(msg, typeClass = '') {
    const logLine = document.createElement('div');
    logLine.className = `log-line ${typeClass}`;

    const d = new Date();
    const timeStr = `${d.getHours().toString().padStart(2, '0')}:${d.getMinutes().toString().padStart(2, '0')}:${d.getSeconds().toString().padStart(2, '0')}.${(d.getMilliseconds() / 10).toFixed(0).padStart(2, '0')}`;

    logLine.textContent = `[${timeStr}] ${msg}`;
    els.consoleLogs.appendChild(logLine);
    els.consoleLogs.scrollTop = els.consoleLogs.scrollHeight;

    if (els.consoleLogs.childNodes.length > 40) {
        els.consoleLogs.removeChild(els.consoleLogs.firstChild);
    }
}

// Color Conversion Helpers
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
