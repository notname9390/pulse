cmake_minimum_required(VERSION 3.16)
project(Pulse VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Create package manager executable
add_executable(pulpm src/tools/package_manager.cpp)

# Platform-specific linking for package manager
if(WIN32)
    target_link_libraries(pulpm ws2_32)
else()
    target_link_libraries(pulpm pthread)
endif()

# Create build tool executable for multi-target compilation
add_executable(pulbuild src/tools/build_tool.cpp)

# Platform-specific linking for build tool
if(WIN32)
    target_link_libraries(pulbuild ws2_32)
else()
    target_link_libraries(pulbuild pthread)
endif()

# Set output directories
set_target_properties(pulpm PROPERTIES
    OUTPUT_NAME "pulpm"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(pulbuild PROPERTIES
    OUTPUT_NAME "pulbuild"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Create symlink for convenience (Unix-like systems only)
if(UNIX AND NOT APPLE)
    add_custom_command(TARGET pulpm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE:pulpm>
        $<TARGET_FILE_DIR:pulpm>/pul
    )
endif()

# Print build information
message(STATUS "Pulse Package Manager will be built as 'pulpm'")
message(STATUS "Build tool will be built as 'pulbuild'")
message(STATUS "Use 'pulpm' or 'pul' to manage packages and build projects") 