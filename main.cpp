//main.cpp
#include <iostream>
#include <vector>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "helper/initBuffer.hpp"
#include "helper/initInstance.hpp"
#include "helper/ObjectLoading/loadObj.hpp"

#include "helper/Rendering/Window.hpp"
#include "helper/Rendering/Surface.hpp"
#include "helper/Rendering/Swapchain.hpp"
#include "helper/Rendering/Depthbuffer.hpp"
#include "helper/Rendering/GraphicsPipeline.hpp"
#include "helper/Texture/Texture.hpp"
#include "Scene.hpp"
#include "helper/Frames/Frame.hpp"
#include "ObjectFactory.hpp"
#include "helper/Rendering/RenderPass.hpp"
#include "helper/Frames/Camera.hpp"
#include "helper/Compute/Snow.hpp"

int main() {
    InitInstance inst;
    Scene* scene = new Scene();
    // Window erstellen
    Window* window = new Window();
    //Kamera erstellen
    Camera* camera = new Camera(glm::vec3(-2.0f, 4.0f, 4.0f),  // Startposition
        glm::vec3(0.0f, 1.0f, 0.0f),   // World Up
        -90.0f,                        // Yaw 
        -10.0f                          // Pitch 
    );
    window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // Maus-Tracking Variablen
    VkExtent2D windowExtent = window->getExtent();
    double lastX = windowExtent.width / 2.0;
    double lastY = windowExtent.height / 2.0;
    bool firstMouse = true;

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

    VkQueue computeQueue;
    vkGetDeviceQueue(device, graphicsIndex, 0, &computeQueue);
    // Swapchain
    SwapChain* swapChain = new SwapChain(
        surface, physicalDevice, device,
        presentQueue, graphicsIndex, presentIndex
    );
    //Command pool
    VkCommandPool commandPool = inst.createCommandPool(device, graphicsIndex);
    // Depth Buffer
    DepthBuffer* depthBuffer = new DepthBuffer(physicalDevice, device, swapChain->getExtent());
    //RenderPass
    RenderPass rp;
    VkRenderPass renderPass = rp.createRenderPass(device, swapChain->getImageFormat(), depthBuffer->getImageFormat());
    // Framebuffers mit diesem RenderPass erzeugen
    Framebuffers* framebuffers = new Framebuffers(device, swapChain, depthBuffer, renderPass);
    // Descriptor Set Layouts
    VkDescriptorSetLayout descriptorSetLayout = inst.createStandardDescriptorSetLayout(device);
    VkDescriptorSetLayout snowDescriptorSetLayout = inst.createSnowDescriptorSetLayout(device);
    VkDescriptorSetLayout litDescriptorSetLayout = inst.createLitDescriptorSetLayout(device);
    scene->setDescriptorSetLayout(descriptorSetLayout);

    // Schneeflocken-Simulation erstellen
    Snow* snow = new Snow(physicalDevice, device, graphicsIndex);

    // Objekte erstellen
    ObjectFactory factory(physicalDevice, device, commandPool, graphicsQueue,
                         swapChain->getImageFormat(), depthBuffer->getImageFormat(),
                         descriptorSetLayout,litDescriptorSetLayout);

    // Skybox
    std::array<const char*, 6> skyboxFaces = {
        "textures/skybox/right.jpg",
        "textures/skybox/left.jpg",
        "textures/skybox/top.jpg",
        "textures/skybox/bottom.jpg",
        "textures/skybox/front.jpg",
        "textures/skybox/back.jpg"
    };
    RenderObject skybox = factory.createSkybox(renderPass, skyboxFaces);
    scene->setRenderObject(skybox);

    //Licht
    LightSourceObject light1 = factory.createLightSource(
        glm::vec3(-2.3f, 3.0f, 0.2f),  // Position
        glm::vec3(1.0f, 0.5f, 0.5f),  // Warmes Licht
        5.0f,                          // Intensity
        10.0f,                         // Radius
        renderPass
    );
    scene->addLightSource(light1);
    scene->setRenderObject(light1.renderObject);


    //Licht2
    LightSourceObject light2 = factory.createLightSource(
        glm::vec3(-8.2f, 12.2f, -6.5f),  // Position
        glm::vec3(0.7f, 0.7f, 0.7f),  // Warmes Licht
        5.0f,                          // Intensity
        100.0f,                         // Radius
        renderPass
    );
    scene->addLightSource(light2);
    scene->setRenderObject(light2.renderObject);

    // Monobloc Gartenstuhl
    glm::mat4 modelChair = glm::mat4(1.0f);
    modelChair = glm::translate(modelChair, glm::vec3(-2.0f, 0.92f, 0.0f));
    modelChair = glm::scale(modelChair, glm::vec3(3.0f, 3.0f, 3.0f));
    RenderObject chair = factory.createLitObject("./models/plastic_monobloc_chair.obj", 
        "textures/plastic_monobloc_chair.jpg", modelChair, renderPass);
    scene->setRenderObject(chair);

    // Fliegender Holländer
    glm::mat4 modelDutch = glm::mat4(1.0f);
    RenderObject dutch = factory.createGenericObject("./models/flying_dutchman.obj", 
        "shaders/test.vert.spv", "shaders/testapp.frag.spv", 
        "textures/duck.jpg", modelDutch, renderPass);
    scene->setRenderObject(dutch);

    // Gartenzwerg
    glm::mat4 modelGnome = glm::mat4(1.0f);
    modelGnome = glm::translate(modelGnome, glm::vec3(-2.0f, 2.25f, 0.0f));
    modelGnome = glm::scale(modelGnome, glm::vec3(3.0f, 3.0f, 3.0f));
    RenderObject gnome = factory.createLitObject("./models/garden_gnome.obj",
        "textures/garden_gnome.jpg", modelGnome, renderPass);
    scene->setRenderObject(gnome);


    glm::mat4 modelUmbrella = glm::mat4(1.0f);
    modelUmbrella = glm::translate(modelUmbrella, glm::vec3(-1.0f, 0.3f, 0.0f));
    modelUmbrella = glm::scale(modelUmbrella, glm::vec3(0.04f, 0.04f, 0.04f));
    modelUmbrella = glm::rotate(modelUmbrella, glm::radians(-100.0f), glm::vec3(1.0f,0.0f,0.0f));
    RenderObject umbrella = factory.createLitObject("./models/sonnenschirm.obj",
        "textures/sonnenschirm.jpg", modelUmbrella, renderPass);
    scene->setRenderObject(umbrella);


    glm::mat4 modelLamp = glm::mat4(1.0f);
    modelLamp = glm::translate(modelLamp, glm::vec3(-10.0f, 0.0f, -10.0f));
    modelLamp = glm::scale(modelLamp, glm::vec3(20.0f, 20.0f, 20.0f));
    modelLamp = glm::rotate(modelLamp, glm::radians(90.0f), glm::vec3(0.0f,1.0f,0.0f));
    RenderObject lamp = factory.createGenericObject("./models/desk_lamp.obj",
        "shaders/test.vert.spv", "shaders/testapp.frag.spv",
        "textures/desk_lamp.jpg", modelLamp, renderPass);
    scene->setRenderObject(lamp);

    // Boden
    glm::mat4 modelGround = glm::mat4(1.0f);
    modelGround = glm::scale(modelGround, glm::vec3(20.0f, 10.0f, 20.0f));
    RenderObject ground = factory.createLitObject("./models/wooden_bowl.obj",
        "textures/wooden_bowl.jpg", modelGround, renderPass);
    scene->setRenderObject(ground);

    // Schneeflocken ZULETZT hinzufügen
    RenderObject snowflakes = factory.createSnowflake(
        "textures/snowflake.png",
        renderPass,
        snow->getCurrentBuffer(),
        snowDescriptorSetLayout
    );
    scene->setRenderObject(snowflakes);

    // Object counts
    size_t normalObjectCount = scene->getNormalObjectCount();
    size_t snowObjectCount = scene->getSnowObjectCount();
    size_t litObjectCount = scene->getLitObjectCount();

    std::cout << "Normal objects: " << normalObjectCount << std::endl;
    std::cout << "Snow objects: " << snowObjectCount << std::endl;
    std::cout << "Light objects: " << litObjectCount << std::endl;
    
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t maxNormalSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * normalObjectCount);
    uint32_t maxSnowSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * snowObjectCount);
    uint32_t maxLitSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * litObjectCount);

    // Descriptor pool
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = maxNormalSets + maxSnowSets+maxLitSets;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = maxNormalSets + maxSnowSets+ maxLitSets;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = maxSnowSets;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxNormalSets + maxSnowSets + maxLitSets;

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool");
    }

    //frames in flight
    std::vector<Frame*> framesInFlight(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        framesInFlight[i] = new Frame(physicalDevice, device, swapChain, framebuffers,
                                     graphicsQueue, commandPool, descriptorPool,
                                     scene->getDescriptorSetLayout());
        
        // Allocate descriptor sets
        framesInFlight[i]->allocateDescriptorSets(descriptorPool, descriptorSetLayout, normalObjectCount);
        framesInFlight[i]->allocateSnowDescriptorSets(descriptorPool, snowDescriptorSetLayout, snowObjectCount);
        framesInFlight[i]->allocateLitDescriptorSets(descriptorPool, litDescriptorSetLayout, litObjectCount);
    }

    // Render loop
    float lastTime = static_cast<float>(glfwGetTime());
    float dutchAngle = 0.0f;
    uint32_t currentFrame = 0;
    
    while (!window->shouldClose()) {
        window->pollEvents();

        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Kamera-Input
        double xpos, ypos;
        window->getCursorPos(&xpos, &ypos);
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }
        float xoffset = static_cast<float>(xpos - lastX);
        float yoffset = static_cast<float>(lastY - ypos);
        lastX = xpos;
        lastY = ypos;

        camera->processMouseMovement(xoffset, yoffset);
        if (window->getKey(GLFW_KEY_Q) == GLFW_PRESS) {
            deltaTime *= 5;
        }
        camera->checkKeyboard(window, deltaTime);
        if (window->getKey(GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true;
        }

        // Schiff bewegen
        dutchAngle += deltaTime * glm::radians(5.0f); 
        float radius = 60.0f;
        float circleX = radius * cos(dutchAngle);
        float circleY = radius * sin(dutchAngle);
        modelDutch = glm::mat4(1.0f);
        modelDutch = glm::translate(modelDutch, glm::vec3(circleX, -10.0f, circleY));
        modelDutch = glm::rotate(modelDutch, -1.75f - dutchAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        modelDutch = glm::scale(modelDutch, glm::vec3(2.0f, 2.0f, 2.0f));
        scene->updateObject(4, modelDutch);

        // Compute Shader für Schnee ausführen
        snow->waitForCompute();
        VkSubmitInfo computeSubmit{};
        computeSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        computeSubmit.commandBufferCount = 1;
        VkCommandBuffer computeCmd = snow->getCommandBuffer();
        computeSubmit.pCommandBuffers = &computeCmd;
        
        if (vkQueueSubmit(graphicsQueue, 1, &computeSubmit, snow->getComputeFence()) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit compute command buffer");
        }

        framesInFlight[currentFrame]->updateUniformBuffer(camera);
        framesInFlight[currentFrame]->updateLitUniformBuffer(camera, scene);
        framesInFlight[currentFrame]->updateDescriptorSet(scene);
        framesInFlight[currentFrame]->updateLitDescriptorSet(scene);
                
        // Update snow descriptor sets
        size_t snowIdx = 0;
        for (size_t i = 0; i < scene->getObjectCount(); i++) {
            if (scene->isSnowObject(i)) {
                const auto& obj = scene->getObject(i);
                framesInFlight[currentFrame]->updateSnowDescriptorSet(
                    snowIdx,
                    snow->getCurrentBuffer(),
                    obj.textureImageView,
                    obj.textureSampler
                );
                snowIdx++;
            }
        }

        // Render
        bool recreate = framesInFlight[currentFrame]->render(scene, camera);
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
    snow->destroy();
    delete snow;
    inst.destroyDescriptorSetLayout(device, descriptorSetLayout);
    inst.destroyDescriptorSetLayout(device,snowDescriptorSetLayout);
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
