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
    DepthBuffer* depthBuffer = new DepthBuffer(
        physicalDevice, device, swapChain->getExtent()
    );

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
    //Descriptor Pool
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    VkDescriptorPool descriptorPool = inst.createDescriptorPool(device, MAX_FRAMES_IN_FLIGHT);

    //Vertex-buffer füllen
    std::vector<Vertex> vertices;
    LoadObj obj;
    obj.objLoader("./models/teapot.obj", vertices);
    uint32_t vertexCount = vertices.size();
    InitBuffer buff;
    VkBuffer vertexBuffer = buff.createVertexBuffer(physicalDevice,device,commandPool, graphicsQueue, vertices);

    //Textur laden
    Texture texture(
        physicalDevice,
        device,
        commandPool,
        graphicsQueue,
        "textures/crate.png"
    );
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
    }

    // create scene object
    Scene* scene = new Scene();
    scene->setRenderObject(pipeline, vertexBuffer, vertexCount, texture.getImageView(), texture.getSampler());


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
