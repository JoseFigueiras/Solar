cmake_minimum_required(VERSION 3.16)

project(solar LANGUAGES CXX)

set_property(GLOBAL PROPERTY TARGET_MESSAGES OFF)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/raylib "${CMAKE_CURRENT_BINARY_DIR}/raylib")

add_executable(solar
    solar.cpp
    physics.cpp
    render.cpp
)

target_compile_options(solar
    PRIVATE
    -O3
)

target_include_directories(solar
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(solar
    PRIVATE
        raylib
)

add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E env LIBGL_ALWAYS_SOFTWARE=1 $<TARGET_FILE:solar>
    DEPENDS solar
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    USES_TERMINAL
)
