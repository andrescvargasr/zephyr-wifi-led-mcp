# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

list(APPEND SNIPPET
  "wifi-credentials"
  "espressif-psram-8M"
  "espressif-psram-wifi"
)
set(SNIPPET "${SNIPPET}" CACHE STRING "Default snippets")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr-wifi)

# Include external headers
zephyr_include_directories(include)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
