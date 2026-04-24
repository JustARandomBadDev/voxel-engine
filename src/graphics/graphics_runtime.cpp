#include "graphics/graphics_runtime.h"

#include <filesystem>
#include <stdexcept>

namespace {

void validateRequiredResourcePath(const std::filesystem::path& path, const char* resource_name) {
    if (path.empty()) {
        throw std::runtime_error(std::string("missing required resource path: ") + resource_name);
    }

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error(
            std::string("required resource does not exist: ") + resource_name + " -> " + path.string()
        );
    }

    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error(
            std::string("required resource is not a file: ") + resource_name + " -> " + path.string()
        );
    }
}

void validateGraphicsResources(const GraphicsResourceConfig& resources) {
    validateRequiredResourcePath(resources.terrainTexture, "graphicsResources.terrainTexture");
    validateRequiredResourcePath(resources.voxelVertexShader, "graphicsResources.voxelVertexShader");
    validateRequiredResourcePath(resources.voxelFragmentShader, "graphicsResources.voxelFragmentShader");
}

// Reject invalid public init config eagerly through exceptions before any runtime setup proceeds.
void validateInitConfig(const VoxelEngineInitConfig& config) {
    if (!config.vulkanHost.createSurface) {
        throw std::runtime_error("GraphicsRuntime::init() -> vulkanHost.createSurface callback must be set");
    }

    if (!config.vulkanHost.getFramebufferExtent) {
        throw std::runtime_error("GraphicsRuntime::init() -> vulkanHost.getFramebufferExtent callback must be set");
    }

    if (config.vulkanHost.requiredInstanceExtensions.empty()) {
        throw std::runtime_error("GraphicsRuntime::init() -> vulkanHost.requiredInstanceExtensions must not be empty");
    }

    if (config.framesInFlight == 0) {
        throw std::runtime_error("GraphicsRuntime::init() -> framesInFlight must be greater than 0");
    }
}

} // namespace

GraphicsRuntime::GraphicsRuntime()
: runtimeLifecycle(instance, device, renderer, swapchain, texture, descriptor, graphicPipeline, bufferManager),
  frameCommandRecorder(renderer, swapchain, graphicPipeline, descriptor, bufferManager),
  frameRenderer(device, renderer, swapchain, bufferManager, frameCommandRecorder) {}

// Framebuffer extent is queried through the host callback copied from init config.
// The runtime stores the callback object, but not the captured host-side state behind it.
VkExtent2D GraphicsRuntime::getFramebufferExtent() const {
    return _host_config.getFramebufferExtent();
}

float GraphicsRuntime::getAspectRatio() const {
    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width > 0 && framebufferExtent.height > 0) {
        return static_cast<float>(framebufferExtent.width) / static_cast<float>(framebufferExtent.height);
    }

    if (swapchain.getImageCount() > 0) {
        return swapchain.getAspectRatio();
    }

    return 1.0f;
}

bool GraphicsRuntime::recreateSwapchain() {
    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width == 0 || framebufferExtent.height == 0) {
        _swapchain_needs_recreate = true;
        return false;
    }

    runtimeLifecycle.recreateSwapchain(framebufferExtent);
    frameRenderer.onFrameResourcesRecreated();
    _swapchain_needs_recreate = false;
    return true;
}

// Single render-time gate for swapchain validity.
// It handles host extent changes and deferred recreate requests before frame execution proceeds.
bool GraphicsRuntime::ensureSwapchainReady() {
    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width == 0 || framebufferExtent.height == 0) {
        _swapchain_needs_recreate = true;
        return false;
    }

    const VkExtent2D swapchainExtent = swapchain.getSwapChainExtent();
    const bool hostExtentChanged =
        swapchain.getImageCount() > 0 &&
        (swapchainExtent.width != framebufferExtent.width || swapchainExtent.height != framebufferExtent.height);

    if (_swapchain_needs_recreate || hostExtentChanged) {
        return recreateSwapchain();
    }

    return true;
}

void GraphicsRuntime::init(const VoxelEngineInitConfig& config) {
    validateInitConfig(config);
    validateGraphicsResources(config.graphicsResources);

    _host_config = config.vulkanHost;
    _clear_color = config.clearColor;

    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width == 0 || framebufferExtent.height == 0) {
        throw std::runtime_error("GraphicsRuntime::init() -> host framebuffer extent must be non-zero during initialization");
    }

    runtimeLifecycle.initialize(
        config.vulkanHost,
        config.graphicsResources,
        config.gpuAllocator,
        config.framesInFlight,
        config.enableValidationLayers,
        framebufferExtent
    );
    frameRenderer.onFrameResourcesRecreated();
}

// Pending uploads are flushed before frame rendering.
// A frame can request swapchain recreation, which is handled immediately through the same readiness path.
void GraphicsRuntime::render(const Camera& camera) {
    if (!ensureSwapchainReady()) return;

    bufferManager.applyCopies();

    if (frameRenderer.render(camera, _clear_color) == FrameRenderStatus::NeedsRecreate) {
        _swapchain_needs_recreate = true;
        ensureSwapchainReady();
    }
}

// Cleanup happens only after idling the device.
// Resources are then released in dependency order before device and instance teardown.
void GraphicsRuntime::cleanup() {
    if (device.getDevice() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device.getDevice());
    }

    runtimeLifecycle.cleanupSwapchainDependentResources();
    swapchain.cleanup(device);
    texture.cleanup(device);
    graphicPipeline.cleanupDescriptorSetLayout(device);
    chunkRenderStateCache.cleanup(bufferManager);
    bufferManager.cleanupBuffers();
    renderer.cleanup(device);
    device.cleanup();
    instance.cleanup();
}
