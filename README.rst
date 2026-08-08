.. zephyr:code:: wifi-led-mcp
   :name: Wi-Fi LED MCP Controller
   :relevant-api: net_mgmt, wifi_mgmt, mcp, zbus

   This code sample demonstrates how to control a WS2812 RGB LED strip via a native Model Context Protocol (MCP) HTTP server and embedded web dashboard on Zephyr RTOS.

Overview
********

An advanced, edge-AI-compatible firmware built on **Zephyr RTOS**. This project bridges local hardware with modern AI agents by hosting a native **Model Context Protocol (MCP)** server alongside a traditional web dashboard.

With this setup, you can control a physical RGB LED through a standard browser UI *or* let an LLM agent dynamically adjust the hardware using standardized tool-calling over a local network.

It includes:

- Multi-threading support via ``_start_threads.c``, featuring a dedicated WS2812 LED strip RGB/HSV thread (``thd_led.c``) using I2S/DMA.
- Integration of a Model Context Protocol (MCP) HTTP server (``src/mcp_server.c``) listening on port 8080 (endpoint ``/mcp``) to expose remote AI tools over HTTP and interact with system hardware via Zbus message channels (``include/led_zbus.h``).

Key Features
============

* **⚡ Real-Time Control (Zephyr RTOS):** Multithreaded execution ensuring low-latency hardware control and network handling.
* **🤖 Model Context Protocol (MCP) Server:** Exposes a JSON-RPC over HTTP/SSE interface, allowing AI agents (like Claude Desktop) to discover and call the ``set_rgb_color`` tool natively.
* **📡 Wi-Fi & Zero-Conf (mDNS):** Automatically connects to your local network and broadcasts itself as ``mcp-led.local``—no hunting for dynamic IP addresses.
* **🌐 Embedded Web Server:** Hosts a lightweight, responsive HTML/JS color-picker dashboard to control the LED from any web browser.
* **💡 Hardware Abstraction:** Leverages Zephyr's Devicetree to seamlessly map to RGB LEDs across supported development boards (STM32, ESP32, Nordic, etc.).

System Architecture
===================

.. code-block:: text

   ┌────────────────────────┐
   │    AI Agent / Client   │
   └───────────┬────────────┘
               │ (MCP Tool Call / SSE)
               ▼
   ┌─────────────────┐   ┌────────────────────────┐
   │  User Browser   ├──►│ Web Server (Port 80)   │
   └─────────────────┘   ├────────────────────────┤
                         │  MCP Server (Port 8080)│
                         └───────────┬────────────┘
                                     │
                                     ▼
                             ┌────────────────┐
                             │ Zephyr RTOS    │
                             │ - MCP Service  │
                             │ - Web Server   │
                             │ - Wi-Fi        │
                             │ - LED Driver   │
                             └────────────────┘
                                     │
                                     ▼
                           [ GPIO/PWM Driver Control ]
                                    │
                                    ▼
                              🔴 🟢 🔵 RGB LED

MCP Server Interface
====================

The application features an HTTP-based MCP server running on port 8080 (endpoint ``/mcp``) with hostname ``mcp-led``. The server exposes remote tools that can be invoked by MCP clients or AI assistants:

- **``delayed_response``**: A test tool demonstrating asynchronous execution, SSE ping keep-alives, and cancellation support.
- **``led_control``**: Enables remote control of the WS2812 RGB LED strip over Zbus (``led_chan``). Accepts actions: ``on``, ``off``, ``toggle``, ``red``, ``green``, and ``blue``.

Project Structure
=================

- ``app.overlay``: Default devicetree overlay configuring WS2812 LED strip over I2S/DMA.
- ``boards/xiao_esp32c5_hpcore.overlay``: Devicetree overlay for Seeed Studio XIAO ESP32-C5 (I2S LED strip on GPIO8).
- ``boards/xiao_esp32s3_procpu.conf``: Kconfig configuration overlay for Seeed Studio XIAO ESP32-S3 (Octal SPIRAM).
- ``boards/xiao_esp32s3_procpu.overlay``: Devicetree overlay for Seeed Studio XIAO ESP32-S3 (I2S LED strip layout).
- ``Kconfig``: Sample configuration options for LED brightness and update delay.
- ``prj.conf``: Main Kconfig application configuration (80 KB heap pool, 4KB system workqueue stack, Net Conn Mgr).
- ``VERSION``: Application version metadata file.
- ``include/led_zbus.h``: Zbus channel and message declarations for LED control.
- ``include/mcp_server.h``: Function prototypes and interface for the MCP HTTP server.
- ``include/params.h``: Shared application parameters and data structures.
- ``include/thd_led.h``: Function prototypes for the LED thread module.
- ``src/_start_threads.c``: Thread management boilerplate defining and launching system threads (e.g. ``thd_led``).
- ``src/mcp_server.c``: MCP HTTP server implementation, registering tools (``delayed_response``, ``led_control``) and publishing Zbus messages.
- ``src/thd_led.c``: WS2812 LED strip thread function implementing smooth HSV rainbow cycling and periodic logging (every 5 seconds).
- ``src/main.c``: Main application entry point, Wi-Fi auto-connect, ``wait_for_network()`` initialization, and MCP server startup.

Requirements
************

Hardware Requirements
=====================

* **Supported Target Boards:**
  - **Seeed Studio XIAO ESP32-C5:** ``xiao_esp32c5/esp32c5/hpcore``
  - **Seeed Studio XIAO ESP32-S3:** ``xiao_esp32s3/esp32s3/procpu`` (with Octal SPIRAM)
  - **ESP32-C5 DevKitC:** ``esp32c5_devkitc/esp32c5/hpcore`` (ESP32-C5 RISC-V High-Performance Core)
  - **Nordic nRF7002:** ``nrf7002dk/nrf5340/cpuapp``
* **Peripherals:** WS2812 RGB LED strip driven over I2S/DMA.
* **Network:** Wi-Fi Access Point with DHCP enabled.

Software & Host Environment
===========================

* **Zephyr SDK:** Version 1.0.1+ installed.
* **Python Environment:** Python 3.x virtual environment with ``west`` installed (activated via ``zvenv`` or ``source /home/user/zephyrproject/.venv/bin/activate``).
* **Network Services:** Host OS supporting mDNS resolution for ``http://mcp-led.local:8080/mcp``.

Wiring
******

Connect the WS2812 RGB LED strip to your development board according to the Devicetree assignments specified in ``app.overlay``:

* **Data Line (DIN):** Connected to the I2S/DMA data output pin defined in ``app.overlay``.
* **Power (VCC):** +5V or +3.3V (depending on LED strip specification).
* **Ground (GND):** Common ground with the development board.

Building and Running
********************

Environment Setup
=================

Activate the Python virtual environment before executing ``west`` commands:

.. code-block:: console

   zvenv
   # or: source /home/user/zephyrproject/.venv/bin/activate

Set Target Board Configuration
==============================

.. code-block:: console

   # Seeed Studio XIAO ESP32-C5
   west config build.board xiao_esp32c5/esp32c5/hpcore

   # Seeed Studio XIAO ESP32-S3
   west config build.board xiao_esp32s3/esp32s3/procpu

   # ESP32-C5 DevKitC
   west config build.board esp32c5_devkitc/esp32c5/hpcore

Build Commands
==============

Standard incremental build using configured board:

.. code-block:: console

   west build

Pristine (clean) build for Seeed XIAO ESP32-C5:

.. code-block:: console

   west build -p always -b xiao_esp32c5/esp32c5/hpcore --build-dir build/xiao_esp32c5

Pristine (clean) build for Seeed XIAO ESP32-S3:

.. code-block:: console

   west build -p always -b xiao_esp32s3/esp32s3/procpu --build-dir build/xiao_esp32s3

Build with Kconfig overlay (e.g. debug overlay):

.. code-block:: console

   west build -b esp32c5_devkitc/esp32c5/hpcore -- -DEXTRA_CONF_FILE="overlay-debug.conf"

Build with Snippets
===================

You can enable and test various snippets incrementally:

1. **Add ``wifi-credentials`` and test it:**

   .. code-block:: console

      west build -p -S wifi-credentials

2. **Add ``espressif-psram-8M`` and test it:**

   .. code-block:: console

      west build -p -S wifi-credentials -S espressif-psram-8M

3. **Add ``espressif-psram-wifi`` and test it:**

   .. code-block:: console

      west build -p -S wifi-credentials -S espressif-psram-8M -S espressif-psram-wifi

Generating SPDX Bill of Materials (SBOM)
========================================

To generate Software Bill of Materials (SPDX 2.3) documents for your build (`DCONFIG_BUILD_OUTPUT_META` in enable by defualt):

.. code-block:: console

   west spdx --init -d build
   west build -d build -- -DCONFIG_BUILD_OUTPUT_META=y
   west spdx -d build

Flashing to Hardware
====================

Default flash command:

.. code-block:: console

   west flash

Flashing specifying serial/USB port:

.. code-block:: console

   # Windows serial port
   west flash --esp-device COM17

   # Linux USB native port (ESP32-C5)
   west flash --esp-device /dev/ttyACM0

   # Linux Serial-to-USB port (ESP32-C5)
   west flash --esp-device /dev/ttyUSB0

Serial Monitor
==============

Launch the serial monitor to view system logs and interact with the Zephyr shell:

.. code-block:: console

   # USB native port on Linux (ESP32-C5)
   west espressif monitor -p /dev/ttyACM0

   # Serial-to-USB port on Linux (ESP32-C5)
   west espressif monitor -p /dev/ttyUSB0

Flash and Open Serial Monitor
=============================

Flash firmware and immediately launch the serial monitor in a single command:

.. code-block:: console

   # Serial-to-USB port on Linux (ESP32-C5)
   west flash --esp-device /dev/ttyUSB0 && west espressif monitor -p /dev/ttyUSB0

   # USB native port on Linux (ESP32-C5)
   west flash --esp-device /dev/ttyACM0 && west espressif monitor -p /dev/ttyACM0

Discovering Device IP via mDNS (Avahi)
======================================

Once the board connects to Wi-Fi, it advertises its hostname as ``mcp-led.local``. You can resolve its assigned IPv4 address or browse mDNS services on Linux/macOS using Avahi tools:

1. **Resolve IPv4 address by hostname:**

   .. code-block:: console

      avahi-resolve-host-name -4 mcp-led.local
      # or:
      avahi-resolve -4 -n mcp-led.local

   *Output example:*

   .. code-block:: text

      mcp-led.local    192.168.1.46

2. **Resolve services using DNS Service Discovery:**

   .. code-block:: console

      avahi-browse -t -r _mcp-led._tcp

3. **Ping device by mDNS hostname:**

   .. code-block:: console

      ping mcp-led.local

4. **Query via `dns-sd` (macOS / Linux):**

   .. code-block:: console

      dns-sd -G v4 mcp-led.local

Sample Console Interaction
==========================

.. code-block:: console

   shell> wifi scan
   Scan requested
   shell>
   Num  | SSID                             (len) | Chan | RSSI | Sec
   1    | kapoueh!                         8     | 1    | -93  | WPA/WPA2
   2    | mooooooh                         8     | 6    | -89  | WPA/WPA2
   3    | Ap-foo blob..                    13    | 11   | -73  | WPA/WPA2
   4    | gksu                             4     | 1    | -26  | WPA/WPA2
   ----------
   Scan request done

   shell> wifi connect "gksu" 4 SecretStuff
   Connection requested
   shell>
   Connected
   shell>

Testing MCP Client with Python
==============================

A Python test script is available in `test_mcp_python_code <https://github.com/andrescvargasr/test_mcp_python_code>`_ to test the connection and issue remote commands to the MCP HTTP server.

Connection Requirements
-----------------------

1. **Host Environment:**
   * Python 3.x (uses built-in ``urllib.request`` and ``json`` libraries).
   * The host machine running the script must be on the same Wi-Fi / LAN network as the target board.
   * Host OS must support mDNS to resolve ``http://mcp-led.local:8080/mcp``. If mDNS is disabled, replace the hostname with the board's IPv4 address.

2. **Target Device Configuration:**
   * Active Wi-Fi network connection with an assigned IPv4 address via DHCP.
   * mDNS Responder enabled (``CONFIG_MDNS_RESPONDER=y``, ``CONFIG_NET_HOSTNAME="mcp-led"``).
   * MCP HTTP Server configured on port 8080 at endpoint ``/mcp`` (``CONFIG_MCP_SERVER=y``, ``CONFIG_MCP_HTTP_PORT=8080``, ``CONFIG_MCP_HTTP_ENDPOINT="/mcp"``).
   * Active Zbus subscriber channel ``led_chan`` and LED strip thread (``thd_led.c``).

3. **MCP JSON-RPC 2.0 Handshake Flow:**

   To establish a successful connection, the server must support the 3-step MCP sequence (JSON-RPC 2.0, version 2025-11-25) executed by ``control_led.py``:

   * **Step 1: Session Initialization (initialize)**

     Request:

     .. code-block:: json

        {
          "jsonrpc": "2.0",
          "id": 0,
          "method": "initialize",
          "params": {
            "protocolVersion": "2025-11-25",
            "capabilities": {},
            "clientInfo": { "name": "antigravity-client", "version": "1.0.0" }
          }
        }

     *Server Requirement:* Return HTTP 200 OK with header ``Mcp-Session-Id`` and JSON response confirming protocol version and server capabilities.

   * **Step 2: Initialization Notification (notifications/initialized)**

     Request:

     .. code-block:: json

        {
          "jsonrpc": "2.0",
          "method": "notifications/initialized"
        }

     *Server Requirement:* Accept notification without error (HTTP 200 or 204).

   * **Step 3: Tool Execution (tools/call)**

     Request:

     .. code-block:: json

        {
          "jsonrpc": "2.0",
          "id": 1,
          "method": "tools/call",
          "params": {
            "name": "led_control",
            "arguments": {
              "action": "on|off|toggle|red|green|blue"
            }
          }
        }

     *Server Requirement:* Execute the hardware action on the RGB LED and return:

     .. code-block:: json

        {
          "jsonrpc": "2.0",
          "id": 1,
          "result": {
            "content": [
              {
                "type": "text",
                "text": "LED turned ON via Zbus"
              }
            ]
          }
        }

Running the Script
------------------

.. code-block:: console

   cd /home/camilo/zephyrproject/projects/test_mcp_python_code

   # Turn LED on / off / toggle
   python control_led.py on
   python control_led.py off
   python control_led.py toggle

   # Set LED color
   python control_led.py red
   python control_led.py green
   python control_led.py blue

References
**********

* `Zephyr RTOS Documentation <https://docs.zephyrproject.org/>`_
* `Zephyr Wi-Fi Management Subsystem <https://docs.zephyrproject.org/latest/connectivity/networking/api/wifi.html>`_
* `Model Context Protocol (MCP) Specification <https://modelcontextprotocol.io/>`_
* `Zephyr Zbus IPC Subsystem <https://docs.zephyrproject.org/latest/services/zbus/index.html>`_
