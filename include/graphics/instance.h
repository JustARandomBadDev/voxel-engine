#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

class Instance {
public:
    void createInstance(bool p_enable_validation_layers);
    void setupDebugMessenger();
    void createSurface(GLFWwindow* window);
    void cleanup();

    const VkInstance& getInstance() const { return instance; }
    const VkSurfaceKHR& getSurface() const { return surface; }

private:
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    bool _validation_layers_enabled = false;

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions() const;
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};

#endif // VULKAN_INSTANCE_H
