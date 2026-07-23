# AGENTS.md - Zephyr RTOS Project Guidelines & Context

This file provides workspace context, build instructions, and development guidelines for AI coding agents working on the **wifi-shell** Zephyr RTOS project.

---

## 1. Project Overview

- **Project Name:** `wifi-shell`
- **Path:** `/home/camilo/zephyrproject/projects/wifi-shell`
- **RTOS:** Zephyr RTOS (v4.4.99+, located at `/home/camilo/zephyrproject/zephyr`)
- **Primary Purpose:** Test Wi-Fi driver functionality, Wi-Fi shell commands (`wifi scan`, `wifi connect`, `wifi disconnect`), network stack management (`net_shell`, `net_if`), PSRAM allocation/relocation, auto-connection with stored Wi-Fi credentials upon boot, and HTTP Model Context Protocol (MCP) server tool execution with Zbus hardware control integration.
- **Target Board / Architecture:** Default configured board is `esp32c5_devkitc/esp32c5/hpcore` (ESP32-C5 RISC-V High-Performance Core). Can also target Nordic `nrf7002dk/nrf5340/cpuapp` or other supported Wi-Fi boards.

---

## 2. Development Environment & Setup

### Python Virtual Environment
The Zephyr environment relies on a dedicated Python virtual environment:
- **Location:** `/home/camilo/zephyrproject/.venv`
- **Activation Command:** `source /home/camilo/zephyrproject/.venv/bin/activate` (or `zvenv` alias)
- **SDK:** Zephyr SDK 1.0.1 installed at `/home/camilo/zephyr-sdk-1.0.1`

> **Note for Agents:** Always ensure commands interacting with `west` or Python tools run with `zvenv` activated or within the virtual environment PATH (`/home/camilo/zephyrproject/.venv/bin`).

---

## 3. Project Structure & Features

```
zephyr-wifi/
├── .gitignore                  # Git ignore rules for build artifacts and temporary files
├── CMakeLists.txt              # CMake build script (appends default snippets and includes include/)
├── Kconfig                     # Application Kconfig options (LED brightness, update delay)
├── VERSION                     # Project version file
├── app.overlay                 # Devicetree overlay for WS2812 LED strip (I2S/DMA)
├── prj.conf                    # Main Kconfig application configuration
├── overlay-debug.conf          # Debug Kconfig overlay configuration
├── README.rst                  # Documentation, snippet testing guide, and sample logs
├── tests.yaml                  # Twister test configuration for automated test runs
├── esp32/
│   └── overlay_enterprise.conf # Kconfig overlay for ESP32 WPA Enterprise features
├── include/
│   ├── led_zbus.h              # Zbus channel and message structures for LED control
│   ├── mcp_server.h            # Header file declaring MCP HTTP server interface
│   ├── params.h                # Global configuration structures and thread parameter definitions
│   └── thd_led.h               # Header file declaring LED strip thread functions
├── src/
│   ├── _start_threads.c        # Thread initialization boilerplate (defines thd_led thread)
│   ├── mcp_server.c            # MCP HTTP server initialization, tool callbacks, and Zbus publishing
│   ├── thd_led.c               # LED strip HSV thread function with 5-second periodic logging
│   └── wifi_test.c             # Wi-Fi test module, auto-connect, event management, and MCP initialization
└── build/                      # Generated build artifacts (managed by west)
```

### Application Features & Threads
- **LED Strip Thread (`src/thd_led.c`):** Runs the WS2812 RGB LED strip driven over I2S/DMA via `app.overlay`. Animates HSV rainbow colors and logs status every 5 seconds.
- **Thread Management (`src/_start_threads.c`):** Boilerplate defining system threads (e.g. `thd_led`) using `K_THREAD_DEFINE`.
- **Wi-Fi Module (`src/wifi_test.c`):** Handles Wi-Fi status callbacks, DHCP event notifications, auto-connection using saved credentials in NVS, and launches the MCP server.
- **MCP HTTP Server & Zbus (`src/mcp_server.c`, `include/mcp_server.h`, `include/led_zbus.h`):** Runs an HTTP MCP server listening on port 8080 (`/mcp` endpoint) under hostname `mcp-hello-world`. Registers MCP tools:
  - `delayed_response`: Asynchronous tool for SSE ping keep-alive and cancellation testing.
  - `led_control`: Remote LED command tool publishing to Zbus channel `led_chan` (`on`, `off`, `toggle`, `red`, `green`, `blue`).
- **PSRAM Size Output:** Queries and prints detected PSRAM size on startup using `esp_psram_get_size()` or Devicetree properties.
- **Net Management Callbacks:** Subscribes to `NET_EVENT_WIFI_CONNECT_RESULT`, `NET_EVENT_WIFI_DISCONNECT_RESULT`, and `NET_EVENT_IPV4_DHCP_BOUND`. Note: Callback handlers must use `uint64_t mgmt_event` parameter type per Zephyr 4.4+ signature specifications.

### Default Snippets (`CMakeLists.txt`)
`CMakeLists.txt` appends default snippets to `SNIPPET` prior to `find_package(Zephyr)`:
- `wifi-credentials`
- `espressif-psram-8M`
- `espressif-psram-wifi`

---

## 4. Common Build & Flashing Commands

All commands should be executed from the project root (`/home/camilo/zephyrproject/projects/wifi-shell`):

### Set Target Board
```bash
west config build.board esp32c5_devkitc/esp32c5/hpcore
```

### Build Application
```bash
# Standard build (uses default snippets configured in CMakeLists.txt)
west build

# Pristine (clean) build for ESP32-C5
west build -p always -b esp32c5_devkitc/esp32c5/hpcore

# Build with custom/additional snippets
west build -p -S wifi-credentials -S espressif-psram-8M -S espressif-psram-wifi

# Build with specific Kconfig overlay (e.g. debug or ESP32 enterprise)
west build -b esp32c5_devkitc/esp32c5/hpcore -- -DEXTRA_CONF_FILE="overlay-debug.conf"
```

### Flash to Hardware
```bash
# Default flash command
west flash

# Flash specifying serial/USB port on Windows
west flash --esp-device COM17

# USB native port on ESP32C5 (Linux)
west flash --esp-device /dev/ttyACM0

# Serial-to-USB port on ESP32C5 (Linux)
west flash --esp-device /dev/ttyUSB0
```

### Serial Monitor
```bash
west espressif monitor -p /dev/ttyUSB0
```

### Automated Testing (Twister)
```bash
west twister -T .
```

---

## 5. Coding & Development Guidelines

### C Coding Style (Zephyr Standard)
- Follow standard Zephyr / Linux Kernel C style guidelines:
  - **Tabs:** 8-character tab indentations.
  - **Naming:** `snake_case` for function and variable names; `UPPER_CASE` for macros and Kconfig options.
  - **Types:** Use exact-width types (`uint8_t`, `uint32_t`, `ssize_t`, `bool`) from `<zephyr/types.h>` or standard C headers.
  - **Net Mgmt Callbacks:** Handler functions must match `net_mgmt_event_handler_t` (`void (*)(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)`).

### Kconfig Conventions (`prj.conf`)
- All network and Wi-Fi features are driven by Kconfig symbols starting with `CONFIG_`.
- Key configurations in this project:
  - `CONFIG_NETWORKING=y` - Enable IP networking stack.
  - `CONFIG_WIFI=y` - Enable Wi-Fi management subsystem.
  - `CONFIG_NET_L2_WIFI_SHELL=y` - Enable Wi-Fi shell commands.
  - `CONFIG_NET_SHELL=y` - Enable network interface shell commands.
  - `CONFIG_NET_MGMT_EVENT_QUEUE_SIZE` / `CONFIG_NET_MGMT_EVENT_QUEUE_TIMEOUT` - Tuned for Wi-Fi scan results processing without queue overflows.
  - `CONFIG_MCP_SERVER=y` - Enable Model Context Protocol (MCP) server library.
  - `CONFIG_MCP_HTTP_PORT=8080` / `CONFIG_MCP_HTTP_ENDPOINT="/mcp"` - MCP HTTP server port and endpoint.
  - `CONFIG_MCP_MAX_TOOLS=4` / `CONFIG_MCP_REQUEST_WORKERS=2` - MCP tool capacity and worker thread allocations.

### Devicetree (`.dts` / `.overlay`)
- If hardware pin assignments or peripheral nodes need customization, add a board overlay file in `boards/<board_name>.overlay`.

### Verification Requirement
- Before completing any task, always verify that the project compiles cleanly using `west build` inside `zvenv`.
