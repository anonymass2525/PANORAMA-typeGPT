cmake_minimum_required(VERSION 3.21)
project(PANORAMA_typeGPT VERSION 0.4.3 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)
add_library(panorama_dsp STATIC src/PanoramaDSP.cpp src/Presets.cpp)
target_include_directories(panorama_dsp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(MSVC)
  target_compile_options(panorama_dsp PRIVATE /W4 /permissive-)
else()
  target_compile_options(panorama_dsp PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(panorama_render tools/render_demos.cpp)
target_link_libraries(panorama_render PRIVATE panorama_dsp)
add_executable(panorama_test tests/test_panorama.cpp)
target_link_libraries(panorama_test PRIVATE panorama_dsp)
add_test(NAME PANORAMA_DSP COMMAND panorama_test)
option(PANORAMA_BUILD_VST3 "Build the JUCE VST3 if JUCE is installed" OFF)
set(PANORAMA_JUCE_PATH "" CACHE PATH "Path to local JUCE source (JUCE/CMakeLists.txt)")
if(PANORAMA_BUILD_VST3)
 if(PANORAMA_JUCE_PATH)
   add_subdirectory("${PANORAMA_JUCE_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/juce")
 else()
   find_package(JUCE CONFIG REQUIRED)
 endif()
 juce_add_plugin(PANORAMA_typeGPT_Plugin
  COMPANY_NAME "Independent Audio Lab"
  BUNDLE_ID "com.independent.panorama.typegpt.v04"
  PLUGIN_MANUFACTURER_CODE PnGP
  PLUGIN_CODE PnG4
  FORMATS VST3
  PRODUCT_NAME "PANORAMA typeGPT v0.4.3"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)
 juce_generate_juce_header(PANORAMA_typeGPT_Plugin)
 target_sources(PANORAMA_typeGPT_Plugin PRIVATE juce/PluginProcessor.cpp juce/PluginEditor.cpp)
 target_include_directories(PANORAMA_typeGPT_Plugin PRIVATE include juce)
 target_link_libraries(PANORAMA_typeGPT_Plugin PRIVATE panorama_dsp juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors juce::juce_gui_extra)
 target_compile_definitions(PANORAMA_typeGPT_Plugin PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
 # Windows VST3 plugin identity (manufacturer/plugin codes) must remain stable
 # within the v0.4 family to preserve DAW sessions and parameter automation.
 option(PANORAMA_BUILD_JUCE_SMOKE "Build host-independent JUCE processor/editor integration checks" ON)
 if(PANORAMA_BUILD_JUCE_SMOKE)
  juce_add_console_app(panorama_juce_smoke PRODUCT_NAME "PANORAMA JUCE Smoke")
  juce_generate_juce_header(panorama_juce_smoke)
  target_sources(panorama_juce_smoke PRIVATE
   tests/test_juce_smoke.cpp juce/PluginProcessor.cpp juce/PluginEditor.cpp)
  target_include_directories(panorama_juce_smoke PRIVATE include juce)
  target_link_libraries(panorama_juce_smoke PRIVATE panorama_dsp
   juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors juce::juce_gui_extra)
  target_compile_definitions(panorama_juce_smoke PRIVATE
   JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  add_test(NAME PANORAMA_JUCE_Integration COMMAND panorama_juce_smoke)
 endif()
endif()
