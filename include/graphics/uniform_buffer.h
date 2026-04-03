#ifndef VULKAN_UNIFORM_BUFFER_H
#define VULKAN_UNIFORM_BUFFER_H

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <graphics/buffer.h>
#include <graphics/light.h>

struct UniformBufferObject {
    alignas(16) glm::vec3 cameraPos;
    alignas(16) glm::mat4 matrix;
    alignas(16) DirectionalLight sunLight;
    alignas(16) DirectionalLight moonLight;
};

class Device;

class UniformBuffer {
public:
    void createUniformBuffer(Device& p_device);
    void updateUniformBuffer(glm::vec3 camPos, glm::mat4 matrix, glm::vec3 sunPos, glm::vec3 moonPos);
    void cleanup();

    VkBuffer& getBuffer() { return buffer.getBuffer(); }
    VkDeviceMemory& getBufferMemory() { return buffer.getBufferMemory(); }

private:
    Buffer buffer;
    void* uniformBufferMapped;
};

#endif
