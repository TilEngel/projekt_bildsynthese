#include "GraphicsPipeline.hpp"

#include <stdexcept>
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

//RenderPass

void GraphicsPipeline::createRenderPass(VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat) {

    // --- COLOR ATTACHMENT ---
    VkAttachmentDescription color{};
    color.format = colorAttachmentFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // --- DEPTH ATTACHMENT ---
    VkAttachmentDescription depth{};
    depth.format = depthAttachmentFormat;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // References
    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    // Dependency
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // RenderPass create info
    std::array<VkAttachmentDescription, 2> attachments = { color, depth };

    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = attachments.size();
    info.pAttachments = attachments.data();
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dep;

    if (vkCreateRenderPass(_device, &info, nullptr, &_renderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass!");
}

//Pipeline layout


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

// ------------------------------------------------------
// 4) Graphics Pipeline
// ------------------------------------------------------
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

// In GraphicsPipeline.cpp - nur der relevante Teil der createPipeline() Methode

// --- Depth-Stencil State basierend auf Pipeline-Typ ---
VkPipelineDepthStencilStateCreateInfo depthStencil{};
depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

switch (_pipelineType) {
    case PipelineType::MIRROR_MARK:
        // Pass 1: Nur Stencil schreiben, keine Farbe/Depth
        depthStencil.depthTestEnable = VK_TRUE;      // Depth-Test AN
        depthStencil.depthWriteEnable = VK_FALSE;    // Aber nicht schreiben
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.stencilTestEnable = VK_TRUE;
        
        // Stencil wird auf 1 gesetzt wo der Spiegel ist
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
        
        // Nur rendern wo Stencil == 1 (wo der Spiegel markiert ist)
        depthStencil.front.compareOp = VK_COMPARE_OP_EQUAL;
        depthStencil.front.failOp = VK_STENCIL_OP_KEEP;
        depthStencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
        depthStencil.front.passOp = VK_STENCIL_OP_KEEP;
        depthStencil.front.compareMask = 0xFF;
        depthStencil.front.writeMask = 0x00;  // Stencil nicht modifizieren
        depthStencil.front.reference = 1;     // Vergleich mit 1
        
        depthStencil.back = depthStencil.front;
        break;

    case PipelineType::MIRROR_BLEND:
        // Pass 3: Transparenter Spiegel
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_FALSE;  // WICHTIG: Kein Depth schreiben!
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
    
    if (_pipelineType == PipelineType::MIRROR_MARK) {
        // Keine Farbe schreiben beim Stencil-Marking
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
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &blendAttachment;

    // --- Rasterizer ---
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;

    // WICHTIG: Für gespiegelte Objekte Culling BEIBEHALTEN, aber frontFace umkehren!
    if (_pipelineType == PipelineType::MIRROR_REFLECT) {
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    } else {
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }

    // WICHTIG: Spiegel verschwindet nicht mehr von hinten
    if (_pipelineType == PipelineType::MIRROR_BLEND) {
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
    

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttachment;


        // --- Dynamic State ---
    std::vector<VkDynamicState> dynamicStates{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    // WICHTIG: Für MIRROR_REFLECT Pipeline Stencil Reference dynamisch machen!
    if (_pipelineType == PipelineType::MIRROR_REFLECT) {
        dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    }

    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = dynamicStates.size();
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
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = _pipelineLayout;
    info.renderPass = _renderPass;
    info.subpass = 0;

    if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &info, nullptr, &_graphicsPipeline)
        != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline!");

    vkDestroyShaderModule(_device, vertModule, nullptr);
    vkDestroyShaderModule(_device, fragModule, nullptr);
}

// ------------------------------------------------------
// Cleanup
// ------------------------------------------------------
void GraphicsPipeline::cleanupRenderPass() {
    if (_renderPass)
        vkDestroyRenderPass(_device, _renderPass, nullptr);
}
void GraphicsPipeline::cleanupPipelineLayout() {
    if (_pipelineLayout)
        vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
}

void GraphicsPipeline::cleanupPipeline() {
    if (_graphicsPipeline)
        vkDestroyPipeline(_device, _graphicsPipeline, nullptr);
}
