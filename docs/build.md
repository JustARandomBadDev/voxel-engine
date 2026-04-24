# Build and Integration

## Introduction

`voxel-engine` is a library, not a standalone application.
It is meant to be integrated into a client program that owns the window, drives the frame loop, and provides the required Vulkan host integration.

## Requirements

- CMake 3.24+
- A C++20 compiler
- Vulkan SDK or Vulkan development libraries

## Building the Engine

Configure:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Build:

```bash
cmake --build build -j
```

This produces the `voxel_engine` library target.

## Using as a Library

If the engine is included directly in a larger CMake project:

```cmake
add_subdirectory(path/to/voxel-engine)

target_link_libraries(your_app
    PRIVATE
        voxel_engine::voxel_engine
)
```

**or :**

```cmake
include(FetchContent)

FetchContent_Declare(
    voxel_engine
    GIT_REPOSITORY https://github.com/JustARandomBadDev/voxel-engine.git
    GIT_TAG main # or dev
    GIT_SHALLOW TRUE
)
```

### Includes

Public headers are exposed through `include/`.
Typical client code includes the public facade and config types, for example:

```cpp
#include "engine/voxel_engine.h"
#include "engine/voxel_engine_config.h"
```

## Integration Expectations

The engine expects the client to provide the host-side Vulkan integration needed at initialization.

The client must provide:

- required Vulkan instance extensions
- a surface creation callback
- a framebuffer extent callback

In practice, this means:

- the client owns the window
- the client owns event handling
- the client owns resize/event propagation on the host side
- the engine uses callbacks to query the current framebuffer extent and create the Vulkan surface

These inputs are provided through `VoxelEngineInitConfig`, especially `VulkanHostConfig`.

## Notes and Limitations

- There is currently no install/export/package setup.
- The graphics backend is currently Vulkan-specific.
- The public API is usable, but not yet stabilized as a final long-term integration surface.
