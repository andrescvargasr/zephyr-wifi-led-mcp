.. zephyr:code-sample:: wifi-shell
   :name: Wi-Fi shell
   :relevant-api: net_stats

   Test Wi-Fi functionality using the Wi-Fi shell module.

Overview
********

This sample allows testing Wi-Fi drivers for various boards by
enabling the Wi-Fi shell module that provides a set of commands:
scan, connect, and disconnect. It also enables the net_shell module
to verify net_if settings.

Additionally, it includes multi-threading support via ``_start_threads.c``,
featuring a dedicated WS2812 LED strip RGB/HSV thread (``thd_led.c``) using I2S/DMA.

It also integrates a Model Context Protocol (MCP) HTTP server (``src/mcp_server.c``)
listening on port 8080 (endpoint ``/mcp``) to expose remote AI tools over HTTP and interact
with system hardware via Zbus message channels (``include/led_zbus.h``).

Target Boards
=============

Supported target boards include:

- **ESP32-C5:** ``esp32c5_devkitc/esp32c5/hpcore`` (ESP32-C5 RISC-V High-Performance Core)
- **Nordic nRF7002:** ``nrf7002dk/nrf5340/cpuapp``

Project Structure
=================

- ``app.overlay``: Devicetree overlay configuring WS2812 LED strip over I2S/DMA.
- ``Kconfig``: Sample configuration options for LED brightness and update delay.
- ``VERSION``: Application version metadata file.
- ``include/led_zbus.h``: Zbus channel and message declarations for LED control.
- ``include/mcp_server.h``: Function prototypes and interface for the MCP HTTP server.
- ``include/params.h``: Shared application parameters and data structures.
- ``include/thd_led.h``: Function prototypes for the LED thread module.
- ``src/_start_threads.c``: Thread management boilerplate defining and launching system threads (e.g. ``thd_led``).
- ``src/mcp_server.c``: MCP HTTP server implementation, registering tools (``delayed_response``, ``led_control``) and publishing Zbus messages.
- ``src/thd_led.c``: WS2812 LED strip thread function implementing smooth HSV rainbow cycling and periodic logging (every 5 seconds).
- ``src/wifi_test.c``: Wi-Fi driver, auto-connect, event management, and MCP server startup.

Model Context Protocol (MCP) Server
===================================

The application features an HTTP-based MCP server running on port 8080 (endpoint ``/mcp``) with hostname ``mcp-hello-world``. The server exposes remote tools that can be invoked by MCP clients or AI assistants:

- **``delayed_response``**: A test tool demonstrating asynchronous execution, SSE ping keep-alives, and cancellation support.
- **``led_control``**: Enables remote control of the WS2812 RGB LED strip over Zbus (``led_chan``). Accepts actions: ``on``, ``off``, ``toggle``, ``red``, ``green``, and ``blue``.

Environment Setup
=================

Activate the Python virtual environment before executing ``west`` commands:

.. code-block:: console

   zvenv
   # or: source /home/camilo/zephyrproject/.venv/bin/activate

Building and Running
********************

Set Target Board Configuration
==============================

.. code-block:: console

   west config build.board esp32c5_devkitc/esp32c5/hpcore

Build Commands
==============

Incremental build using configured board:

.. code-block:: console

   west build

Pristine (clean) build for ESP32-C5:

.. code-block:: console

   west build -p always -b esp32c5_devkitc/esp32c5/hpcore

Build with Kconfig overlay (e.g. debug overlay):

.. code-block:: console

   west build -b esp32c5_devkitc/esp32c5/hpcore -- -DEXTRA_CONF_FILE="overlay-debug.conf"

Build with Snippets
=====================

You can enable and test various snippets incrementally:

1. **Add `wifi-credentials` and test it:**

   .. code-block:: console

      west build -p -S wifi-credentials

2. **Add `espressif-psram-8M` and test it:**

   .. code-block:: console

      west build -p -S wifi-credentials -S espressif-psram-8M

3. **Add `espressif-psram-wifi` and test it:**

   .. code-block:: console

      west build -p -S wifi-credentials -S espressif-psram-8M -S espressif-psram-wifi


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

