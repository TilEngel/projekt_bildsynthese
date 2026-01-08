#pragma once
#include <vulkan/vulkan_core.h>
#include <array>
#include <stdexcept>

class RenderPass {
public:
    VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat) {
        // Attachment indices
        enum {
            kAttachment_BACK = 0,
            kAttachment_DEPTH = 1,
            kAttachment_GBUFFER = 2,
        };

        enum {
            kSubpass_DEPTH = 0,
            kSubpass_GBUFFER = 1,
            kSubpass_LIGHTING = 2,
        };
        std::cout<<"Attachments\n";
        // --- ATTACHMENTS ---
        std::array<VkAttachmentDescription, 3> attachments{};

        // Back buffer (final output)
        attachments[kAttachment_BACK].format = colorFormat;
        attachments[kAttachment_BACK].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[kAttachment_BACK].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[kAttachment_BACK].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[kAttachment_BACK].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[kAttachment_BACK].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[kAttachment_BACK].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[kAttachment_BACK].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Depth buffer (with stencil for mirrors)
        attachments[kAttachment_DEPTH].format = depthFormat;
        attachments[kAttachment_DEPTH].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[kAttachment_DEPTH].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[kAttachment_DEPTH].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[kAttachment_DEPTH].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[kAttachment_DEPTH].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[kAttachment_DEPTH].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[kAttachment_DEPTH].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;;

        // G-Buffer (position, normal, albedo packed in RGBA32)
        attachments[kAttachment_GBUFFER].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attachments[kAttachment_GBUFFER].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[kAttachment_GBUFFER].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[kAttachment_GBUFFER].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[kAttachment_GBUFFER].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[kAttachment_GBUFFER].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[kAttachment_GBUFFER].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[kAttachment_GBUFFER].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // --- ATTACHMENT REFERENCES ---
        
        // Depth attachment reference (used in depth prepass and g-buffer pass)
        VkAttachmentReference depthRef{};
        depthRef.attachment = kAttachment_DEPTH;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // G-Buffer output reference
        VkAttachmentReference gBufferRef{};
        gBufferRef.attachment = kAttachment_GBUFFER;
        gBufferRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Lighting input references
        std::array<VkAttachmentReference, 2> lightingInputs{};
        lightingInputs[0].attachment = kAttachment_GBUFFER;
        lightingInputs[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        lightingInputs[1].attachment = kAttachment_DEPTH;
        lightingInputs[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        // Back buffer output reference
        VkAttachmentReference backBufferRef{};
        backBufferRef.attachment = kAttachment_BACK;
        backBufferRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // --- SUBPASSES ---
        std::array<VkSubpassDescription, 3> subpasses{};
        std::cout<<"Subpasses\n";
      
        // Subpass 0: Depth Prepass
        uint32_t preserveAttachment = kAttachment_GBUFFER;
        subpasses[kSubpass_DEPTH].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[kSubpass_DEPTH].colorAttachmentCount = 0;
        subpasses[kSubpass_DEPTH].pColorAttachments = nullptr;
        subpasses[kSubpass_DEPTH].pDepthStencilAttachment = &depthRef;


        // Subpass 1: G-Buffer Generation
        subpasses[kSubpass_GBUFFER].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[kSubpass_GBUFFER].colorAttachmentCount = 1;
        subpasses[kSubpass_GBUFFER].pColorAttachments = &gBufferRef;
        subpasses[kSubpass_GBUFFER].pDepthStencilAttachment = &depthRef;

       //read-only depth attachment reference für Subpass 2
        VkAttachmentReference depthReadRef{};
        depthReadRef.attachment = kAttachment_DEPTH;
        depthReadRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        // Subpass 2: Lighting Pass
        subpasses[kSubpass_LIGHTING].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[kSubpass_LIGHTING].inputAttachmentCount = 2;
        subpasses[kSubpass_LIGHTING].pInputAttachments = lightingInputs.data();
        subpasses[kSubpass_LIGHTING].colorAttachmentCount = 1;
        subpasses[kSubpass_LIGHTING].pColorAttachments = &backBufferRef;
        subpasses[kSubpass_LIGHTING].pDepthStencilAttachment = &depthReadRef;

        // --- SUBPASS DEPENDENCIES ---
        std::array<VkSubpassDependency, 3> dependencies{};
        std::cout<<"Dependencies \n";

        // External -> Depth Prepass
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = kSubpass_DEPTH;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = 0;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        std::cout<<"0 Done\n";
        // Depth Prepass -> G-Buffer
        dependencies[1].srcSubpass = kSubpass_DEPTH;
        dependencies[1].dstSubpass = kSubpass_GBUFFER;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        std::cout<<"1 Done\n";
        // G-Buffer -> Lighting
        dependencies[2].srcSubpass = kSubpass_GBUFFER;
        dependencies[2].dstSubpass = kSubpass_LIGHTING;
        dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        std::cout<<"2 Done\n";
        
        // --- CREATE RENDER PASS ---
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        std::cout<<"Create\n";
        VkRenderPass renderPass;
        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create deferred render pass!");
        }
        std::cout<<"All Done\n";
        return renderPass;
    }
};