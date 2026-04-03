#include "graphics/uniform_buffer.h"

#include <string.h>
#include <stdexcept>

#include "graphics/device.h"

void UniformBuffer::createUniformBuffer(Device& p_device) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject); 

    buffer.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_device);

    VkResult result = vkMapMemory(p_device.getDevice(), buffer.getBufferMemory(), 0, bufferSize, 0, &uniformBufferMapped);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to map uniform buffer memory!");
    }
}

void UniformBuffer::updateUniformBuffer(glm::vec3 camPos, glm::mat4 matrix, glm::vec3 sunDir, glm::vec3 moonDir) {
    sun_light.direction = sunDir;
    moon_light.direction = moonDir;

    UniformBufferObject ubo {
        camPos,
        matrix,
        sun_light,
        moon_light
    };

    memcpy(uniformBufferMapped, &ubo, sizeof(ubo));
}

void UniformBuffer::cleanup() {
    buffer.cleanup();
}
