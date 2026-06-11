cmake_minimum_required(VERSION 3.16)

project(solar LANGUAGES CXX)

set_property(GLOBAL PROPERTY TARGET_MESSAGES OFF)

add_executable(solar
    solar.cpp
)

target_compile_options(solar
    PRIVATE
    -O3
)

target_include_directories(solar
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(run
    COMMAND $<TARGET_FILE:solar>
    DEPENDS solar
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    USES_TERMINAL
)
