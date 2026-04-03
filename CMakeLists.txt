cmake_minimum_required(VERSION 3.20)
project(MinecraftVulkan VERSION 1.0 LANGUAGES CXX)

# -----------------------------
# C++ standard
# -----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Warnings
# -----------------------------
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter)
endif()

# -----------------------------
# Dependencies
# -----------------------------
include(FetchContent)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.10
)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

# -----------------------------
# Vulkan
# -----------------------------
find_package(Vulkan REQUIRED)

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
# Executable
# -----------------------------
add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${HEADER_FILES}
)

# -----------------------------
# Include dirs
# -----------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    include
)

# -----------------------------
# stb (header-only)
# -----------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/external
)

# -----------------------------
# Libraries
# -----------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    glfw
    glm
)

# -----------------------------
# Platform-specific
# -----------------------------
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads dl)
elseif(APPLE)
    find_library(COCOA Cocoa REQUIRED)
    find_library(IOKIT IOKit REQUIRED)
    find_library(COREFOUNDATION CoreFoundation REQUIRED)
    find_library(COREVIDEO CoreVideo REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA}
        ${IOKIT}
        ${COREFOUNDATION}
        ${COREVIDEO}
    )
endif()

# -----------------------------
# Definitions
# -----------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)
