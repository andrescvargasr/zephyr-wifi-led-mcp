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

option(INCLUDE_HTML_CONTENT "Include the HTML content" ON)

target_link_libraries(app PRIVATE zephyr_interface zephyr)

# Include external headers
zephyr_include_directories(include)

zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_HTTP_SERVICE
  NAME http_resource_desc_led_http_service
  KVMA RAM_REGION GROUP RODATA_REGION
)

# Generate web_assets.h from static web resources
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/web_assets.h
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/src/static_web_resources/embed_assets.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/static_web_resources
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/static_web_resources/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/src/static_web_resources/style.css
    ${CMAKE_CURRENT_SOURCE_DIR}/src/static_web_resources/app.js
    ${CMAKE_CURRENT_SOURCE_DIR}/src/static_web_resources/embed_assets.py
  COMMENT "Generating web_assets.h from static web resources"
)
add_custom_target(generate_web_assets DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/web_assets.h)
add_dependencies(app generate_web_assets)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
