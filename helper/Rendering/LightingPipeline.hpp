#pragma once

#include <vulkan/vulkan.h>

class LightingPipeline {
public:
    static void createLightingPipeline(VkDevice device,
                                      VkFormat colorFormat,
                                      VkRenderPass renderPass,
                                      VkDescriptorSetLayout descriptorSetLayout,
                                      VkPipeline& pipeline,
                                      VkPipelineLayout& pipelineLayout);
};