cmake_minimum_required(VERSION 3.12)
project(pico_kiss_protocol_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

add_executable(host_tests
  host_tests.c
  ../src/pico-kiss-protocol.c
)

target_include_directories(host_tests PRIVATE
  ../src
)

add_test(NAME host_tests COMMAND host_tests)
