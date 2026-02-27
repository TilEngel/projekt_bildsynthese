#include "ReflectionProbe.hpp"

std::array<glm::mat4, 6> ReflectionProbe::getCubeFaceViews() const {
   
    return {
        // Face 0: +X (right)
        glm::lookAt(_position, _position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        
        // Face 1: -X (left)
        glm::lookAt(_position, _position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        
        // Face 2: +Y (up)
        glm::lookAt(_position, _position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        
        // Face 3: -Y (down)
        glm::lookAt(_position, _position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        
        // Face 4: +Z (forward)
        glm::lookAt(_position, _position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        
        // Face 5: -Z (backward)
        glm::lookAt(_position, _position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f))
    };
}

//90Grad FOV Projection für Cubemap
glm::mat4 ReflectionProbe::getProjection() const {
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    proj[1][1] *= -1.0f; // y-flip
    return proj;
}

void ReflectionProbe::createCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(_device, &allocInfo, &_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate reflection probe command buffer!");
    }
}