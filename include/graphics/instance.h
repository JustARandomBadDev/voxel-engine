#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

class Instance {
public:
    void createInstance();
    void setupDebugMessenger();
    void createSurface(GLFWwindow* window);
    void cleanup();

    const VkInstance& getInstance() const { return instance; }
    const VkSurfaceKHR& getSurface() const { return surface; }

private:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};

#endif // VULKAN_INSTANCE_H
