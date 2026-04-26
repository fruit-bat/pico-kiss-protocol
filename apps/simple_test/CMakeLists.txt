
add_executable(simple_test
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

target_compile_definitions(simple_test PRIVATE
)

target_link_libraries(simple_test
  pico_kiss_protocol
)

# create map/bin/hex file etc.
pico_add_extra_outputs(simple_test)
