//main.cpp
#include <iostream>
#include <vector>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "helper/initBuffer.hpp"
#include "helper/initInstance.hpp"
#include "helper/loadObj.hpp"

#include "helper/Window.hpp"
#include "helper/Surface.hpp"
#include "helper/Swapchain.hpp"
#include "helper/Depthbuffer.hpp"
#include "helper/GraphicsPipeline.hpp"
#include "helper/Texture.hpp"
#include "helper/Scene.hpp"
#include "helper/Frame.hpp"


int main() {
    InitInstance inst;
    Scene* scene = new Scene();
    // Window erstellen
    Window* window = new Window();

    // Instance + Extensions
    auto extensions = window->getRequiredExtensions();
    VkInstance instance = inst.createInstance(extensions);

    // Surface
    Surface* surface = new Surface(window, instance);

    // Physical Device
    uint32_t graphicsIndex, presentIndex;
    VkPhysicalDevice physicalDevice = inst.pickPhysicalDevice(
        instance,
        surface,
        &graphicsIndex,
        &presentIndex
    );

    // Logical Device
    VkDevice device = inst.createLogicalDevice(physicalDevice, graphicsIndex, presentIndex);

    // Queues
    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, graphicsIndex, 0, &graphicsQueue);

    VkQueue presentQueue;
    vkGetDeviceQueue(device, presentIndex, 0, &presentQueue);

    // Swapchain
    SwapChain* swapChain = new SwapChain(
        surface, physicalDevice, device,
        presentQueue, graphicsIndex, presentIndex
    );

    // Depth Buffer
    DepthBuffer* depthBuffer = new DepthBuffer(physicalDevice, device, swapChain->getExtent());

    // Pipeline
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        device,
        swapChain->getImageFormat(),
        depthBuffer->getImageFormat()
    );

    // Framebuffers
    Framebuffers* framebuffers = new Framebuffers(
        device, swapChain, depthBuffer, pipeline->getRenderPass()
    );

    //Command pool
    VkCommandPool commandPool = inst.createCommandPool(device, graphicsIndex);
    
    //Vertex-buffer füllen
    std::vector<Vertex> vertices;
    LoadObj obj;
    obj.objLoader("./models/teapot.obj", vertices);
    uint32_t vertexCount = vertices.size();
    InitBuffer buff;
    VkBuffer vertexBuffer = buff.createVertexBuffer(physicalDevice,device,commandPool, graphicsQueue, vertices);


    //Vertex-buffer füllen
    std::vector<Vertex> vertices2;
    obj.objLoader("./models/flying_dutchman.obj", vertices2);
    uint32_t vertexCount2 = vertices2.size();
    VkBuffer vertexBuffer2 = buff.createVertexBuffer(physicalDevice,device,commandPool, graphicsQueue, vertices2);

    //Textur laden
    Texture texture(
        physicalDevice,
        device,
        commandPool,
        graphicsQueue,
        "textures/crate.png"
    );


    
    scene->setRenderObject(pipeline, vertexBuffer2, vertexCount2, texture.getImageView(), texture.getSampler());
    scene->setRenderObject(pipeline, vertexBuffer, vertexCount, texture.getImageView(), texture.getSampler());
    
    // nach dem Laden der Modelle und nachdem scene Objekte enthält:
    size_t objectCount = scene->getObjectCount();
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * objectCount);

    // Descriptor pool: einen Eintrag pro (frame,object) für UB und ImageSampler
    std::array<VkDescriptorPoolSize,2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = maxSets; // ein UBO-Eintrag pro Set (Binding 0)
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = maxSets; // ein imageSampler pro Set (Binding 1)

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSets;

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool");
    }



    // create frames in flight
    std::vector<Frame*> framesInFlight(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        framesInFlight[i] = new Frame(physicalDevice,
            device,
            swapChain,
            framebuffers,
            graphicsQueue,
            commandPool,
            descriptorPool,
            pipeline->getDescriptorSetLayout());
            framesInFlight[i]->allocateDescriptorSets(descriptorPool, pipeline->getDescriptorSetLayout(), scene->getObjectCount());
    }


    // start render loop
    uint32_t currentFrame = 0;
    while (!window->shouldClose()) {
        window->pollEvents();
        bool recreate = framesInFlight[currentFrame]->render(scene);
        if (recreate || window->wasResized()) {
            vkDeviceWaitIdle(device);
            swapChain->recreate();
            depthBuffer->recreate(swapChain->getExtent());
            framebuffers->recreate();
        }
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    vkDeviceWaitIdle(device);
    
    // Cleanup
    delete scene;
    texture.destroy();
    buff.destroyVertexBuffer(device);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        delete framesInFlight[i];
    }
    inst.destroyDescriptorPool(device, descriptorPool);
    inst.destroyCommandPool(device, commandPool);
    delete framebuffers;
    delete pipeline;
    delete depthBuffer;
    delete swapChain;
    inst.destroyDevice(device);
    delete surface;
    inst.destroyInstance(instance);
    delete window;

    return EXIT_SUCCESS;
}
