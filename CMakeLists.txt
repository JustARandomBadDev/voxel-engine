cmake_minimum_required(VERSION 4.1)
cmake_policy(VERSION 4.1)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(MinecraftVulkan VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Options de compilation
# -----------------------------
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter)
endif()

# -----------------------------
# FetchContent pour les libs
# -----------------------------
include(FetchContent)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# stb_image (header-only)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.10
)
FetchContent_MakeAvailable(glfw)

# Vulkan
find_package(Vulkan REQUIRED)

# -----------------------------
# Sources et headers
# -----------------------------
file(GLOB_RECURSE CORE_SRC src/core/*.cpp)
file(GLOB_RECURSE ENGINE_SRC src/engine/*.cpp)
file(GLOB_RECURSE GRAPHICS_SRC src/graphics/*.cpp)
file(GLOB_RECURSE WORLD_SRC src/world/*.cpp)
set(SRC_FILES
    src/main.cpp
    ${CORE_SRC}
    ${ENGINE_SRC}
    ${GRAPHICS_SRC}
    ${WORLD_SRC}
)

file(GLOB_RECURSE CORE_INC include/core/*.h)
file(GLOB_RECURSE ENGINE_INC include/engine/*.h)
file(GLOB_RECURSE GRAPHICS_INC include/graphics/*.h)
file(GLOB_RECURSE WORLD_INC include/world/*.h)
set(INC_DIRS
    include
    ${glm_SOURCE_DIR}
    ${stb_SOURCE_DIR}
)

# -----------------------------
# Executable
# -----------------------------
add_executable(${PROJECT_NAME} ${SRC_FILES} ${CORE_INC} ${ENGINE_INC} ${GRAPHICS_INC} ${WORLD_INC})

# -----------------------------
# Include directories
# -----------------------------
target_include_directories(${PROJECT_NAME} PRIVATE ${INC_DIRS})

# -----------------------------
# Link libraries cross-platform
# -----------------------------
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw glm)
elseif(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw glm
        ${COCOA_FRAMEWORK} ${IOKIT_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK} ${COREVIDEO_FRAMEWORK})
else() # Linux / Unix
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw glm Threads::Threads dl)
endif()

# -----------------------------
# Definitions pour STB
# -----------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE STB_IMAGE_IMPLEMENTATION)
