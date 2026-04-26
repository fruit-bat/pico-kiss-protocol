add_library(pico_kiss_protocol INTERFACE)

target_sources(pico_kiss_protocol INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/pico-kiss-protocol.c
)

target_include_directories(pico_kiss_protocol INTERFACE
    src
)
