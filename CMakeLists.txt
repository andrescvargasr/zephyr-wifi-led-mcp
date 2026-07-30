# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

list(APPEND SNIPPET
  "wifi-credentials"
  "espressif-psram-8M"
  "espressif-psram-reloc" # Actually, there is no official support for this snippet on ESP32C5, yet.
  "espressif-psram-wifi"
)
set(SNIPPET "${SNIPPET}" CACHE STRING "Default snippets")

set(CONFIG_BUILD_OUTPUT_META y CACHE STRING "Enable build output metadata for SPDX")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr-wifi-led-mcp)

# Include external headers
zephyr_include_directories(include)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
