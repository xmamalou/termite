# FILEPATH: ${DRAGONFLY_ROOT}/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)
set(CMAKE_C_STANDARD 17) 

set(HOME $ENV{HOME})
set(VULKAN_SDK $ENV{VULKAN_SDK})

# project info
project(Termite VERSION 0.1.0 LANGUAGES C)

add_library(vulkan SHARED IMPORTED)
set_target_properties(vulkan PROPERTIES
    IMPORTED_LOCATION ${VULKAN_SDK}/lib/libvulkan.so
)

include_directories(Termite-C/)
include_directories(Termite-C/Control)

# source files
set(SOURCES
    Termite-C/Main.c
    Termite-C/Control/Thread.c
    Termite-C/Control/Memory.c
)

# this is only temporary, when in a finished state, Dragonfly will be a (dynamically linked) library
add_executable(termite ${SOURCES})

target_link_libraries(termite
    vulkan
)
target_include_directories(termite
    PUBLIC
        ${VULKAN_SDK}/include
)