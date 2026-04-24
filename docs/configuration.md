# Configuration

## `VoxelEngineInitConfig`

Initialization is configured through `VoxelEngineInitConfig`, declared in `include/engine/voxel_engine_config.h`.

It groups the main inputs required to start the engine:

- `vulkanHost`
- `clearColor`
- `framesInFlight`
- `enableValidationLayers`
- `graphicsResources`
- `gpuAllocator`

At a high level, this config mixes:

- host-side Vulkan integration data
- graphics resource paths
- runtime graphics parameters

That reflects the current engine state and public contract.

## `VulkanHostConfig`

`VulkanHostConfig` describes what the client must provide so the engine can integrate with the host application’s Vulkan-capable windowing environment.

It contains:

- `requiredInstanceExtensions`
- `createSurface`
- `getFramebufferExtent`

### `requiredInstanceExtensions`

Type:

- `std::vector<std::string>`

Purpose:

- lists the Vulkan instance extensions required by the host environment

Typical source:

- windowing integration code in the client application

Usage:

- consumed during instance creation

### `createSurface`

Type:

- `std::function<VkResult(VkInstance, VkSurfaceKHR&)>`

Purpose:

- lets the host create a Vulkan surface for the engine’s Vulkan instance

Usage:

- called by the engine during initialization

Ownership:

- the client provides the callback
- the engine stores a copy of the callback
- the engine owns and destroys the resulting `VkSurfaceKHR`

### `getFramebufferExtent`

Type:

- `std::function<VkExtent2D()>`

Purpose:

- lets the engine query the current framebuffer extent from the host

Usage:

- used during initialization
- used again during runtime, especially for aspect ratio queries and swapchain recreation

Ownership:

- the client provides the callback
- the engine stores a copy of the callback
- the engine does **not** own the state captured by that callback

### Important Lifetime Rule

The callbacks in `VulkanHostConfig` must remain valid for the lifetime of the engine runtime.

More precisely:

- the engine copies the `std::function` objects
- but it does **not** own any captured host-side state inside those callables
- if a callback captures a window pointer, platform handle, or client object, that captured state must remain valid until `shutdown()`

This is the most important lifetime rule in the current public init contract.

## Initialization Validation and Failure

Initialization currently validates the provided config and may fail through exceptions.

Typical causes include:

- missing required host callbacks
- empty required Vulkan instance extension lists
- invalid graphics resource paths
- invalid configuration values such as `framesInFlight == 0`

This is the current behavior of the public init path.
It is useful to treat `VoxelEngine::init(...)` as a potentially throwing operation.

## `GraphicsResourceConfig`

`GraphicsResourceConfig` provides file paths for graphics resources needed at initialization.

It currently contains:

- `terrainTexture`
- `voxelVertexShader`
- `voxelFragmentShader`

These are init-time resource paths used to create the current graphics runtime.

At a high level:

- shader paths are used to build the graphics pipelines
- texture paths are used to create the current texture resource used by the graphics runtime

The engine validates that these files exist before continuing initialization.

## Runtime Parameters

### `framesInFlight`

Type:

- `uint32_t`

Purpose:

- configures the number of frames-in-flight used by the graphics runtime

Current default:

- `2`

### `enableValidationLayers`

Type:

- `bool`

Purpose:

- enables Vulkan validation layers when available

Current default:

- `true`

### `gpuAllocator`

Type:

- `GpuAllocatorConfig`

Purpose:

- configures GPU-side allocation/storage sizing used by the current buffer system

It currently includes:

- `meshDataBlockCapacityPerAllocator`
- `indirectCommandCapacityPerAllocator`
- `stagingBufferBytes`
- `allocationMarginBlocks`

This is a runtime tuning/configuration struct for the current allocator-backed graphics storage path.

### `clearColor`

Type:

- `glm::vec4`

Purpose:

- sets the current render clear color used by the graphics runtime

## Lifetime Rules

The easiest way to understand the init config is to split it into two categories.

### Init-only data

These values are consumed during initialization and do not need to stay externally alive afterward:

- `requiredInstanceExtensions`
- `graphicsResources`
- `framesInFlight`
- `enableValidationLayers`
- `gpuAllocator`
- `clearColor`

The engine copies the values it needs from these fields.

### Runtime-used data

These callbacks are stored and may be used after initialization:

- `createSurface`
- `getFramebufferExtent`

The engine copies the callback objects themselves, but does not own the state captured by them.

That means:

- the callback objects inside the engine remain valid
- but the client must keep their captured state valid for the entire engine lifetime

## Practical Ownership Summary

What the client provides:

- host-side Vulkan integration callbacks
- extension names
- graphics resource paths
- runtime configuration values

What the engine owns after init:

- Vulkan instance
- Vulkan surface
- logical device
- swapchain
- graphics runtime resources

What the engine does **not** own:

- the host window
- event loop
- input state
- camera control
- any client-side state captured by init callbacks
