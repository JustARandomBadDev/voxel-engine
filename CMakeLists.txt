cmake_minimum_required(VERSION 3.24...4.3)
project(voxel_engine VERSION 1.0 LANGUAGES CXX)

# -----------------------------
# C++ standard
# -----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Warnings
# -----------------------------
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter)

# -----------------------------
# Dependencies
# -----------------------------
include(FetchContent)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

# STB
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# -----------------------------
# Vulkan
# -----------------------------
find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)

# -----------------------------
# Sources
# -----------------------------
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    src/*.cpp
)

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
    include/*.h
)

# -----------------------------
# Library
# -----------------------------
add_library(voxel_engine
    ${SRC_FILES}
    ${HEADER_FILES}
)

add_library(voxel_engine::voxel_engine ALIAS voxel_engine)

# -----------------------------
# Include dirs
# -----------------------------
target_include_directories(voxel_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${stb_SOURCE_DIR}
)

# -----------------------------
# Libraries
# -----------------------------
target_link_libraries(voxel_engine
    PUBLIC
        glm
        Vulkan::Vulkan
        glfw
    PRIVATE
        Threads::Threads
        dl
)

# -----------------------------
# Definitions
# -----------------------------
target_compile_definitions(voxel_engine
    PUBLIC
        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE
)