add_subdirectory(interpreter)
add_subdirectory(util)

add_library(pee pee.cc)

add_executable(main main.cc)
target_link_libraries(main pee interpreter util)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
