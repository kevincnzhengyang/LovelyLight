# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# speech_recognition should be the first
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/components/speech_recognition
    ${CMAKE_CURRENT_SOURCE_DIR}/components/asr
    ${CMAKE_CURRENT_SOURCE_DIR}/components/board
    ${CMAKE_CURRENT_SOURCE_DIR}/components/leds
    ${CMAKE_CURRENT_SOURCE_DIR}/components/sensor
    )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lovlylight)