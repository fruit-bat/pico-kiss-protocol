cmake_minimum_required(VERSION 3.12)
project(pico_kiss_protocol_monitor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(monitor
  src/main.c
  ../../src/pico-kiss-protocol.c
)

target_include_directories(monitor PRIVATE
  ../../src
)
