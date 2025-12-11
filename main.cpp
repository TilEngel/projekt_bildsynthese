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
#include "ObjectFactory.hpp"


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
    //Command pool
    VkCommandPool commandPool = inst.createCommandPool(device, graphicsIndex);
    
    // Depth Buffer
    DepthBuffer* depthBuffer = new DepthBuffer(physicalDevice, device, swapChain->getExtent());

    ObjectFactory factory(physicalDevice,device,commandPool,graphicsQueue,swapChain->getImageFormat(), depthBuffer->getImageFormat());
    float time = static_cast<float>(glfwGetTime());
    // Erstelle zwei Objekte:
    glm::mat4 modelTeapot = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f,0.0f,0.0f));
    modelTeapot = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    RenderObject teapot = factory.createTeapot("./models/teapot.obj", "shaders/testapp.vert.spv", "shaders/testapp.frag.spv", "textures/crate.png", modelTeapot);
    scene->setRenderObject(teapot);

    // glm::mat4 modelDutch = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f,0.0f,10.0f));
    // modelDutch = glm::scale(glm::mat4(0.5f),glm::vec3(1.0f,1.0f,1.0f));
    // modelDutch = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // RenderObject dutch = factory.createFlyingDutchman("./models/flying_dutchman.obj", "shaders/testapp.vert.spv", "shaders/testapp.frag.spv", "textures/crate.png", modelDutch);
    // scene->setRenderObject(dutch);

   

    // Framebuffers
    Framebuffers* framebuffers = new Framebuffers(
        device, swapChain, depthBuffer, teapot.pipeline->getRenderPass()
    );

    // //Textur laden
    // Texture texture(
    //     physicalDevice,
    //     device,
    //     commandPool,
    //     graphicsQueue,
    //     "textures/crate.png"
    // );


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
            teapot.pipeline->getDescriptorSetLayout());
            framesInFlight[i]->allocateDescriptorSets(descriptorPool, teapot.pipeline->getDescriptorSetLayout(), scene->getObjectCount());
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
    //texture.destroy();
    //buff.destroyVertexBuffer(device);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        delete framesInFlight[i];
    }
    inst.destroyDescriptorPool(device, descriptorPool);
    inst.destroyCommandPool(device, commandPool);
    delete framebuffers;
    //delete pipeline;
    delete depthBuffer;
    delete swapChain;
    inst.destroyDevice(device);
    delete surface;
    inst.destroyInstance(instance);
    delete window;

    return EXIT_SUCCESS;
}
