#include "GraphicsPipeline.hpp"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <fstream>
#include <array>

// Helper: read SPIR-V file
static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open shader file!");

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

// Helper: create shader module
VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(device, &info, nullptr, &module) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module!");

    return module;
}

void GraphicsPipeline::createPipelineLayout() {
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(glm::mat4); // model matrix

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;

    if(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout)!=VK_SUCCESS){
        throw std::runtime_error("Failed to create pipeline layout!");
    }
}

void GraphicsPipeline::createPipeline() {
    // Load shader modules
    auto vertCode = readFile(_vertexShaderPath);
    auto fragCode = readFile(_fragmentShaderPath);

    VkShaderModule vertModule = createShaderModule(_device, vertCode);
    VkShaderModule fragModule = createShaderModule(_device, fragCode);

    // Vertex Shader Stage
    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    // Fragment Shader Stage
    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

    // --- Vertex Input ---
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributes{};
    attributes[0].binding = 0;
    attributes[0].location = 0;
    attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset = offsetof(Vertex, pos);

    attributes[1].binding = 0;
    attributes[1].location = 1;
    attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[1].offset = offsetof(Vertex, tex);

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = attributes.size();
    vertexInput.pVertexAttributeDescriptions = attributes.data();

    // --- Input Assembly ---
    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly.primitiveRestartEnable = VK_FALSE;

    // --- Viewport State (dynamic) ---
    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    // --- Multisampling ---
    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // --- Depth-Stencil State basierend auf Pipeline-Typ ---
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    switch (_pipelineType) {
        case PipelineType::DEPTH_ONLY:
            // Depth Prepass: Nur Depth schreiben, kein Color Output
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.stencilTestEnable = VK_FALSE;
            break;
            
        case PipelineType::GBUFFER:
            // G-Buffer Pass: Depth Test, aber kein Write (bereits vom Prepass)
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_FALSE;  // WICHTIG!
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;  // Nur Pixel vom Prepass
            depthStencil.stencilTestEnable = VK_FALSE;
            break;
            
        case PipelineType::LIGHTING:
            // Lighting Pass: Kein Depth Test (Fullscreen Quad)
            depthStencil.depthTestEnable = VK_FALSE;
            depthStencil.depthWriteEnable = VK_FALSE;
            depthStencil.stencilTestEnable = VK_FALSE;
            break;
        
        case PipelineType::MIRROR_MARK:
            // Pass 1: Nur Stencil schreiben, keine Farbe/Depth
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_FALSE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            depthStencil.stencilTestEnable = VK_TRUE;
            
            depthStencil.front.compareOp = VK_COMPARE_OP_ALWAYS;
            depthStencil.front.failOp = VK_STENCIL_OP_KEEP;
            depthStencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
            depthStencil.front.passOp = VK_STENCIL_OP_REPLACE;
            depthStencil.front.compareMask = 0xFF;
            depthStencil.front.writeMask = 0xFF;
            depthStencil.front.reference = 1;
            
            depthStencil.back = depthStencil.front;
            break;
            
        case PipelineType::MIRROR_REFLECT:
            // Pass 2: Gespiegelte Objekte - nur rendern wo Stencil == 1
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.stencilTestEnable = VK_TRUE;
            
            depthStencil.front.compareOp = VK_COMPARE_OP_EQUAL;
            depthStencil.front.failOp = VK_STENCIL_OP_KEEP;
            depthStencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
            depthStencil.front.passOp = VK_STENCIL_OP_KEEP;
            depthStencil.front.compareMask = 0xFF;
            depthStencil.front.writeMask = 0x00;
            depthStencil.front.reference = 1;
            
            depthStencil.back = depthStencil.front;
            break;

        case PipelineType::MIRROR_BLEND:
            // Pass 3: Transparenter Spiegel
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_FALSE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            depthStencil.stencilTestEnable = VK_FALSE;
            break;
            
        case PipelineType::SKYBOX:
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_FALSE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            depthStencil.stencilTestEnable = VK_FALSE;
            break;
        default:  // STANDARD
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            depthStencil.stencilTestEnable = VK_FALSE;
            break;
    }

    // --- Color Blend State ---
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    
    if (_pipelineType == PipelineType::DEPTH_ONLY) {
        // Depth Prepass: Kein Color Write
        blendAttachment.colorWriteMask = 0;
        blendAttachment.blendEnable = VK_FALSE;
    } else if (_pipelineType == PipelineType::MIRROR_MARK) {
        // Stencil Marking: Keine Farbe schreiben
        blendAttachment.colorWriteMask = 0;
        blendAttachment.blendEnable = VK_FALSE;
    } else if (_pipelineType == PipelineType::MIRROR_BLEND) {
        // Alpha Blending für transparenten Spiegel
        blendAttachment.blendEnable = VK_TRUE;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    } else {
        blendAttachment.blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    
    // KRITISCH: Depth-Only Pipeline hat KEINE Color Attachments!
    if (_pipelineType == PipelineType::DEPTH_ONLY) {
        colorBlending.attachmentCount = 0;  // NULL!
        colorBlending.pAttachments = nullptr;
    } else {
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &blendAttachment;
    }

    // --- Rasterizer ---
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;

    //Depth Bias für G-Buffer Pass- sonst zfighting Depth & GBuffer
    if (_pipelineType == PipelineType::GBUFFER) {
        rasterizer.depthBiasEnable = VK_TRUE;
        rasterizer.depthBiasConstantFactor = -1.5f;  //Leicht verschieben
        rasterizer.depthBiasSlopeFactor = -1.5f;
        rasterizer.depthBiasClamp = 0.0f;
    } else {
        rasterizer.depthBiasEnable = VK_FALSE;
    }

    // Culling und Front Face
    if (_pipelineType == PipelineType::MIRROR_REFLECT) {
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    } else if (_pipelineType == PipelineType::MIRROR_BLEND) {
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }else if (_pipelineType == PipelineType::LIGHTING) {
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    } else {
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }

    // --- Dynamic State ---
    std::vector<VkDynamicState> dynamicStates{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    if (_pipelineType == PipelineType::MIRROR_REFLECT) {
        dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    }

    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamic.pDynamicStates = dynamicStates.data();

    // --- Pipeline CreateInfo ---
    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &vertexInput;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &rasterizer;
    info.pMultisampleState = &msaa;
    info.pDepthStencilState = &depthStencil;
    info.pColorBlendState = &colorBlending;
    info.pDynamicState = &dynamic;
    info.layout = _pipelineLayout;
    info.renderPass = _renderPass;
    info.subpass = _subpassIndex;  // WICHTIG: Subpass Index!

    if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &info, nullptr, &_graphicsPipeline)
        != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline!");

    vkDestroyShaderModule(_device, vertModule, nullptr);
    vkDestroyShaderModule(_device, fragModule, nullptr);

    std::cout << "\n=== PIPELINE CREATED ===" << std::endl;
    std::cout << "Type: ";
    switch (_pipelineType) {
        case PipelineType::STANDARD: std::cout << "STANDARD"; break;
        case PipelineType::DEPTH_ONLY: std::cout << "DEPTH_ONLY"; break;
        case PipelineType::GBUFFER: std::cout << "GBUFFER"; break;
        case PipelineType::LIGHTING: std::cout << "LIGHTING"; break;
        case PipelineType::SKYBOX: std::cout << "SKYBOX"; break;
        default: std::cout << "UNKNOWN"; break;
    }
    std::cout << std::endl;
    std::cout << "Vertex Shader: " << _vertexShaderPath << std::endl;
    std::cout << "Fragment Shader: " << _fragmentShaderPath << std::endl;
    std::cout << "Subpass: " << _subpassIndex << std::endl;
    std::cout << "Pipeline Handle: " << (_graphicsPipeline != VK_NULL_HANDLE) << std::endl;
}

void GraphicsPipeline::cleanupPipelineLayout() {
    if (_pipelineLayout)
        vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
}

void GraphicsPipeline::cleanupPipeline() {
    if (_graphicsPipeline)
        vkDestroyPipeline(_device, _graphicsPipeline, nullptr);
}