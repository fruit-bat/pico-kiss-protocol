cmake_minimum_required(VERSION 3.12)
project(pico_kiss_protocol_monitor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(monitor
  src/main.c
  ../../src/pico-kiss-protocol.c
)

target_include_directories(monitor PRIVATE
  ../../include
  ../../src
)

# Build and link the pico-serial-proxy helper library (sibling project)
add_subdirectory(../../../pico-serial-proxy ${CMAKE_CURRENT_BINARY_DIR}/pico_serial_proxy_build)
add_subdirectory(../../../pico-serial-recording ${CMAKE_CURRENT_BINARY_DIR}/pico_serial_recording_build)
target_link_libraries(monitor PRIVATE pico_serial_proxy pico_serial_recording)
