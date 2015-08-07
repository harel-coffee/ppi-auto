add_subdirectory(interpreter)
add_subdirectory(util)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(common)

add_library(pee pee.cc)
add_library(pep pep.cc)

add_executable(main main.cc)
target_link_libraries(main pee pep interpreter util server client common ${OPENCL_LIBRARIES} ${POCO_FOUNDATION} ${POCO_NET} pthread )
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
