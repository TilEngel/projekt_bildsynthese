//main.cpp
#include <iostream>
#include <vector>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "helper/initBuffer.hpp"
#include "helper/initInstance.hpp"
#include "helper/loadObj.hpp"

#include "HelloVulkan17.hpp"
#include "helper/Window.hpp"
#include "helper/Surface.hpp"
#include "helper/Swapchain.hpp"
#include "helper/Depthbuffer.hpp"
#include "helper/GraphicsPipeline.hpp"
#include "helper/Texture.hpp"


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

    HelloVulkanApplication* app = new HelloVulkanApplication();

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

    // App konfigurieren
    app->setVertexBuffer(vertexBuffer, vertexCount);
    app->setWindow(window);
    app->setGraphicsQueue(graphicsQueue);
    app->setPhysicalDevice(physicalDevice);
    app->setLogicalDevice(device);
    app->setSwapChain(swapChain);
    app->setDepthBuffer(depthBuffer);
    app->setGraphicsPipeline(pipeline);
    app->setFramebuffers(framebuffers);
    app->setCommandPool(commandPool);
    app->setVertexBuffer(vertexBuffer, vertexCount);
    app->setTexture(texture.getImageView(), texture.getSampler());
    app->prepareRendering();

    // Render Loop
    while (!window->shouldClose()) {
        window->pollEvents();
        app->render();
    }

    vkDeviceWaitIdle(device);

    // Cleanup
    delete app;
    texture.destroy();
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
