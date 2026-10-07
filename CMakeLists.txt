cmake_minimum_required(VERSION 3.16)
project(FrameMouse CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_options(-Wall -Wextra)

# SteamVR ships the OpenVR client library; link against it instead of installing anything.
set(OPENVR_LIB_DIR "/opt/steamvr/bin/linuxarm64" CACHE PATH "Directory containing libopenvr_api.so")
find_library(OPENVR_LIB openvr_api PATHS ${OPENVR_LIB_DIR} NO_DEFAULT_PATH REQUIRED)

add_library(tracker STATIC src/surface_tracker.cpp)
target_include_directories(tracker PUBLIC src)

add_executable(framemouse
  src/main.cpp
  src/config.cpp
  src/uinput_mouse.cpp
  src/vr_input.cpp)
target_include_directories(framemouse PRIVATE third_party/openvr)
target_link_libraries(framemouse PRIVATE tracker ${OPENVR_LIB})
target_compile_definitions(framemouse PRIVATE FRAMEMOUSE_ACTIONS_DIR="${CMAKE_SOURCE_DIR}/actions")
set_target_properties(framemouse PROPERTIES BUILD_RPATH ${OPENVR_LIB_DIR} INSTALL_RPATH ${OPENVR_LIB_DIR})

enable_testing()
add_executable(test_surface_tracker tests/test_surface_tracker.cpp)
target_link_libraries(test_surface_tracker PRIVATE tracker)
add_test(NAME surface_tracker COMMAND test_surface_tracker)
