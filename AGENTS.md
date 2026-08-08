# AGENTS.md - Zephyr RTOS Project Guidelines & Context

This file provides workspace context, build instructions, and development guidelines for AI coding agents working on the **zephyr-wifi-led-mcp** Zephyr RTOS project.

---

## 1. Project Overview

- **Project Name:** `zephyr-wifi-led-mcp`
- **Path:** `/home/user/zephyrproject/projects/zephyr-wifi-led-mcp`
- **RTOS:** Zephyr RTOS (v4.4.99+, located at `/home/user/zephyrproject/zephyr`)
- **Primary Purpose:** Test Wi-Fi driver functionality, Wi-Fi shell commands (`wifi scan`, `wifi connect`, `wifi disconnect`), network stack management (`net_shell`, `net_if`), PSRAM allocation/relocation, auto-connection with stored Wi-Fi credentials upon boot using Net Connection Manager, HTTP Model Context Protocol (MCP) server tool execution, and Zbus hardware control integration.
- **Target Boards / Architectures:**
  - `esp32c5_devkitc/esp32c5/hpcore` (Default ESP32-C5 DevKitC RISC-V HP Core)
  - `xiao_esp32c5/esp32c5/hpcore` (Seeed Studio XIAO ESP32-C5)
  - `xiao_esp32s3/esp32s3/procpu` (Seeed Studio XIAO ESP32-S3 with Octal SPI RAM)
  - Nordic `nrf7002dk/nrf5340/cpuapp` or other supported Wi-Fi boards.

---

## 2. Development Environment & Setup

### Python Virtual Environment
The Zephyr environment relies on a dedicated Python virtual environment:
- **Location:** `/home/user/zephyrproject/.venv`
- **Activation Command:** `source /home/user/zephyrproject/.venv/bin/activate` (or `zvenv` alias)
- **SDK:** Zephyr SDK 1.0.1 installed at `/home/user/zephyr-sdk-1.0.1`

> **Note for Agents:** Always ensure commands interacting with `west` or Python tools run with `zvenv` activated or within the virtual environment PATH (`/home/user/zephyrproject/.venv/bin`).

---

## 3. Project Structure & Features

```
zephyr-wifi-led-mcp/
├── .gitignore                  # Git ignore rules for build artifacts and temporary files
├── CMakeLists.txt              # CMake build script (includes net common, sets default snippets, and CONFIG_BUILD_OUTPUT_META=y)
├── Kconfig                     # Application Kconfig options (LED brightness, update delay)
├── VERSION                     # Project version file
├── app.overlay                 # Devicetree overlay for WS2812 LED strip (I2S/DMA)
├── prj.conf                    # Main Kconfig application configuration (128KB heap, 4KB system workqueue, Net Conn Mgr)
├── overlay-debug.conf          # Debug Kconfig overlay configuration
├── README.rst                  # Documentation, board guide, snippet testing, SPDX generation, and sample logs
├── tests.yaml                  # Twister test configuration for automated test runs
├── boards/
│   ├── xiao_esp32c5_hpcore.overlay # Devicetree overlay for Seeed XIAO ESP32-C5 (I2S LED strip on GPIO8)
│   ├── xiao_esp32s3_procpu.conf    # Kconfig overlay for Seeed XIAO ESP32-S3 (Octal SPIRAM configuration)
│   └── xiao_esp32s3_procpu.overlay # Devicetree overlay for Seeed XIAO ESP32-S3 (I2S LED strip layout)
├── esp32/
│   └── overlay_enterprise.conf # Kconfig overlay for ESP32 WPA Enterprise features
├── include/
│   ├── led_zbus.h              # Zbus channel and message structures for LED control
│   ├── mcp_server.h            # Header file declaring MCP HTTP server interface
│   ├── params.h                # Global configuration structures and thread parameter definitions
│   └── thd_led.h               # Header file declaring LED strip thread functions
├── src/
│   ├── _start_threads.c        # Thread initialization boilerplate (defines thd_led thread)
│   ├── main.c                  # Main application entry point, Wi-Fi auto-connect, wait_for_network(), and MCP startup
│   ├── mcp_server.c            # MCP HTTP server initialization, tool callbacks, and Zbus publishing
│   └── thd_led.c               # LED strip HSV thread function with 5-second periodic logging
└── build/                      # Generated build artifacts (managed by west)
```

### Application Features & Threads
- **LED Strip Thread (`src/thd_led.c`):** Runs the WS2812 RGB LED strip driven over I2S/DMA via `app.overlay` or board overlays. Animates HSV rainbow colors and logs status every 5 seconds.
- **Thread Management (`src/_start_threads.c`):** Boilerplate defining system threads (e.g. `thd_led`) using `K_THREAD_DEFINE`.
- **Main Application Module (`src/main.c`):** Triggers `auto_connect()` via NVS credentials, waits for IP assignment via `wait_for_network()`, and launches the MCP server, HTTP server, and mDNS responder.
- **MCP HTTP Server & Zbus (`src/mcp_server.c`, `include/mcp_server.h`, `include/led_zbus.h`):** Runs an HTTP MCP server listening on port 8080 (`/mcp` endpoint) under hostname `mcp-led`. Registers MCP tools:
  - `delayed_response`: Asynchronous tool for SSE ping keep-alive and cancellation testing.
  - `led_control`: Remote LED command tool publishing to Zbus channel `led_chan` (`on`, `off`, `toggle`, `red`, `green`, `blue`).
- **PSRAM Size Output:** Queries and prints detected PSRAM size on startup using `esp_psram_get_size()` or Devicetree properties.

### Default Snippets (`CMakeLists.txt`)
`CMakeLists.txt` appends default snippets to `SNIPPET` prior to `find_package(Zephyr)`:
- `wifi-credentials`
- `espressif-psram-8M`
- `espressif-psram-reloc`
- `espressif-psram-wifi`

---

## 4. Common Build & Flashing Commands

All commands should be executed from the project root (`/home/user/zephyrproject/projects/zephyr-wifi-led-mcp`):

### Set Target Board
```bash
# Seeed Studio XIAO ESP32-C5
west config build.board xiao_esp32c5/esp32c5/hpcore

# Seeed Studio XIAO ESP32-S3
west config build.board xiao_esp32s3/esp32s3/procpu

# Default ESP32-C5 DevKitC
west config build.board esp32c5_devkitc/esp32c5/hpcore
```

### Build Application
```bash
# Standard build (uses default snippets configured in CMakeLists.txt)
west build

# Pristine (clean) build for Seeed XIAO ESP32-C5
west build -p always -b xiao_esp32c5/esp32c5/hpcore --build-dir build/xiao_esp32c5

# Pristine (clean) build for Seeed XIAO ESP32-S3
west build -p always -b xiao_esp32s3/esp32s3/procpu --build-dir build/xiao_esp32s3

# Build with custom/additional snippets
west build -p -S wifi-credentials -S espressif-psram-8M -S espressif-psram-reloc -S espressif-psram-wifi
```

### SPDX Bill of Materials (SBOM) Generation
```bash
# Initialize and generate SPDX 2.3 documents
west spdx --init -d build
west build -d build -- -DCONFIG_BUILD_OUTPUT_META=y
west spdx -d build
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
west espressif monitor -p /dev/ttyACM0
```

For Xiao boards:

```bash
west espressif monitor -p /dev/ttyUSB0
```

### Automated Testing (Twister)
```bash
west twister -T .
```

---

## 5. Coding & Development Guidelines

### C Coding Style & SPDX Metadata (Zephyr Standard)
- Follow standard Zephyr / Linux Kernel C style guidelines:
  - **License Headers:** Include standard SPDX tag (`SPDX-License-Identifier: MIT` or `Apache-2.0`), Doxygen `@file`, `@author`, `@version`, and `@date` tags in all `.c` and `.h` files.
  - **Tabs:** 8-character tab indentations.
  - **Naming:** `snake_case` for function and variable names; `UPPER_CASE` for macros and Kconfig options.
  - **Types:** Use exact-width types (`uint8_t`, `uint32_t`, `ssize_t`, `bool`) from `<zephyr/types.h>` or standard C headers.

### Kconfig Conventions (`prj.conf`)
- All network and Wi-Fi features are driven by Kconfig symbols starting with `CONFIG_`.
- Key configurations in this project:
  - `CONFIG_NETWORKING=y` / `CONFIG_WIFI=y` - Enable IP networking and Wi-Fi subsystems.
  - `CONFIG_NET_CONNECTION_MANAGER=y` - Network Connection Manager support for automatic connection management.
  - `CONFIG_HEAP_MEM_POOL_SIZE=81920` - 80KB heap pool for MCP server, HTTP server, mDNS, and Wi-Fi buffer allocations.
  - `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=4096` - 4KB stack size for system workqueue to handle network management events safely.
  - `CONFIG_NET_RX_STACK_SIZE=4096` / `CONFIG_NET_TX_STACK_SIZE=4096` - 4KB network stack sizes.
  - `CONFIG_STACK_SENTINEL=y` - Stack overflow protection sentinel.
  - `CONFIG_MCP_SERVER=y` - Enable Model Context Protocol (MCP) server library.
  - `CONFIG_MCP_HTTP_PORT=8080` / `CONFIG_MCP_HTTP_ENDPOINT="/mcp"` - MCP HTTP server port and endpoint.
  - `CONFIG_MDNS_RESPONDER=y` / `CONFIG_NET_HOSTNAME="mcp-led"` - Zero-conf mDNS hostname resolution.

### Devicetree (`.dts` / `.overlay`)
- Hardware pin assignments or peripheral nodes are customized in `app.overlay` or board overlays in `boards/<board_name>.overlay` (e.g. `boards/xiao_esp32c5_hpcore.overlay` and `boards/xiao_esp32s3_procpu.overlay`).

### Verification Requirement
- Before completing any task, always verify that the project compiles cleanly using `west build` inside `zvenv`.
