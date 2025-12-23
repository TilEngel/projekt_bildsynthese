//main.cpp (Merged - Snow + Lighting + Mirrors)
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
#include "helper/MirrorSystem.hpp"

int main() {
    InitInstance inst;
    Scene* scene = new Scene();
    Window* window = new Window();
    
    Camera* camera = new Camera(glm::vec3(-2.0f, 4.0f, 4.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        -90.0f,
        -10.0f
    );
    window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    VkExtent2D windowExtent = window->getExtent();
    double lastX = windowExtent.width / 2.0;
    double lastY = windowExtent.height / 2.0;
    bool firstMouse = true;

    auto extensions = window->getRequiredExtensions();
    VkInstance instance = inst.createInstance(extensions);

    Surface* surface = new Surface(window, instance);

    uint32_t graphicsIndex, presentIndex;
    VkPhysicalDevice physicalDevice = inst.pickPhysicalDevice(
        instance, surface, &graphicsIndex, &presentIndex
    );
    
    VkDevice device = inst.createLogicalDevice(physicalDevice, graphicsIndex, presentIndex);
    
    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, graphicsIndex, 0, &graphicsQueue);

    VkQueue presentQueue;
    vkGetDeviceQueue(device, presentIndex, 0, &presentQueue);

    VkQueue computeQueue;
    vkGetDeviceQueue(device, graphicsIndex, 0, &computeQueue);
    
    SwapChain* swapChain = new SwapChain(
        surface, physicalDevice, device,
        presentQueue, graphicsIndex, presentIndex
    );
    
    VkCommandPool commandPool = inst.createCommandPool(device, graphicsIndex);
    DepthBuffer* depthBuffer = new DepthBuffer(physicalDevice, device, swapChain->getExtent());
    
    RenderPass rp;
    VkRenderPass renderPass = rp.createRenderPass(device, swapChain->getImageFormat(), depthBuffer->getImageFormat());
    
    Framebuffers* framebuffers = new Framebuffers(device, swapChain, depthBuffer, renderPass);
    
    VkDescriptorSetLayout descriptorSetLayout = inst.createStandardDescriptorSetLayout(device);
    VkDescriptorSetLayout snowDescriptorSetLayout = inst.createSnowDescriptorSetLayout(device);
    VkDescriptorSetLayout litDescriptorSetLayout = inst.createLitDescriptorSetLayout(device);
    scene->setDescriptorSetLayout(descriptorSetLayout);

    // Schneeflocken-Simulation erstellen
    Snow* snow = new Snow(physicalDevice, device, graphicsIndex);

    // Objekte erstellen
    ObjectFactory factory(physicalDevice, device, commandPool, graphicsQueue,
                         swapChain->getImageFormat(), depthBuffer->getImageFormat(),
                         descriptorSetLayout, litDescriptorSetLayout);

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

    // Licht 1
    LightSourceObject light1 = factory.createLightSource(
        glm::vec3(-2.3f, 3.0f, 0.2f),
        glm::vec3(1.0f, 0.5f, 0.5f),
        5.0f,
        10.0f,
        renderPass
    );
    scene->addLightSource(light1);
    scene->setRenderObject(light1.renderObject);

    // Licht 2
    LightSourceObject light2 = factory.createLightSource(
        glm::vec3(-8.2f, 12.2f, -6.5f),
        glm::vec3(0.7f, 0.7f, 0.7f),
        5.0f,
        100.0f,
        renderPass
    );
    scene->addLightSource(light2);
    scene->setRenderObject(light2.renderObject);

    //Monobloc Gartenstuhl
    glm::mat4 modelChair = glm::mat4(1.0f);
    modelChair = glm::translate(modelChair, glm::vec3(-2.0f, 0.92f, 0.0f));
    modelChair = glm::scale(modelChair, glm::vec3(3.0f, 3.0f, 3.0f));
    RenderObject chair = factory.createGenericObject(
        "./models/plastic_monobloc_chair.obj",
        "shaders/testapp.vert.spv",
        "shaders/testapp.frag.spv",
        "textures/plastic_monobloc_chair.jpg",
        modelChair, renderPass, PipelineType::STANDARD);
    scene->setRenderObject(chair);
    size_t chairIndex = scene->getObjectCount() - 1;

    //Fliegender Holländer
    glm::mat4 modelDutch = glm::mat4(1.0f);
    RenderObject dutch = factory.createGenericObject(
        "./models/flying_dutchman.obj",
        "shaders/test.vert.spv",
        "shaders/testapp.frag.spv",
        "textures/duck.jpg",
        modelDutch, renderPass, PipelineType::STANDARD);
    scene->setRenderObject(dutch);
    size_t dutchIndex = scene->getObjectCount() - 1;

    //Gartenzwerg
    glm::mat4 modelGnome = glm::mat4(1.0f);
    modelGnome = glm::translate(modelGnome, glm::vec3(-2.0f, 2.25f, 0.0f));
    modelGnome = glm::scale(modelGnome, glm::vec3(3.0f, 3.0f, 3.0f));
    RenderObject gnome = factory.createGenericObject(
        "./models/garden_gnome.obj",
        "shaders/testapp.vert.spv",
        "shaders/testapp.frag.spv",
        "textures/garden_gnome.jpg",
        modelGnome, renderPass, PipelineType::STANDARD);
    scene->setRenderObject(gnome);
    size_t gnomeIndex = scene->getObjectCount() - 1;

    // Sonnenschirm
    glm::mat4 modelUmbrella = glm::mat4(1.0f);
    modelUmbrella = glm::translate(modelUmbrella, glm::vec3(-1.0f, 0.3f, 0.0f));
    modelUmbrella = glm::scale(modelUmbrella, glm::vec3(0.04f, 0.04f, 0.04f));
    modelUmbrella = glm::rotate(modelUmbrella, glm::radians(-100.0f), glm::vec3(1.0f,0.0f,0.0f));
    RenderObject umbrella = factory.createLitObject("./models/sonnenschirm.obj",
        "textures/sonnenschirm.jpg", modelUmbrella, renderPass);
    scene->setRenderObject(umbrella);

    // Lampe
    glm::mat4 modelLamp = glm::mat4(1.0f);
    modelLamp = glm::translate(modelLamp, glm::vec3(-10.0f, 0.0f, -10.0f));
    modelLamp = glm::scale(modelLamp, glm::vec3(20.0f, 20.0f, 20.0f));
    modelLamp = glm::rotate(modelLamp, glm::radians(90.0f), glm::vec3(0.0f,1.0f,0.0f));
    RenderObject lamp = factory.createGenericObject("./models/desk_lamp.obj",
        "shaders/test.vert.spv", "shaders/testapp.frag.spv",
        "textures/desk_lamp.jpg", modelLamp, renderPass, PipelineType::STANDARD);
    scene->setRenderObject(lamp);

    // Boden
    glm::mat4 modelGround = glm::mat4(1.0f);
    modelGround = glm::scale(modelGround, glm::vec3(20.0f, 10.0f, 20.0f));
    RenderObject ground = factory.createLitObject("./models/wooden_bowl.obj",
        "textures/wooden_bowl.jpg", modelGround, renderPass);
    scene->setRenderObject(ground);

    // ========== SPIEGEL-SYSTEM SETUP ==========
    
    MirrorSystem* mirrorSystem = new MirrorSystem(&factory, renderPass);
    
    // Spiegel 1: Hinter dem Gnom
    MirrorConfig mirror1;
    mirror1.position = glm::vec3(-2.0f, 1.5f, -3.0f);
    mirror1.normal = glm::vec3(0.0f, 0.0f, 1.0f); //zur Kamera zeigend
    mirror1.scale = glm::vec3(1.5f, 2.5f, 0.1f);
    mirrorSystem->addMirror(scene, mirror1);
    
    // Spiegel 2: Rechts
    MirrorConfig mirror2;
    mirror2.position = glm::vec3(2.0f, 1.5f, 0.0f);
    mirror2.normal = glm::vec3(-1.0f, 0.0f, 0.0f); //nach links zeigend
    mirror2.scale = glm::vec3(1.5f, 2.5f, 0.1f);
    mirrorSystem->addMirror(scene, mirror2);
    
    // Objekte markieren, die gespiegelt werden sollen
    mirrorSystem->addReflectableObject(gnomeIndex);
    mirrorSystem->addReflectableObject(chairIndex);
    scene->markObjectAsReflectable(gnomeIndex);
    scene->markObjectAsReflectable(chairIndex);
    
    // Reflexionen erstellen
    mirrorSystem->createReflections(scene);

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
    std::cout << "Lit objects: " << litObjectCount << std::endl;
    
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t maxNormalSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * normalObjectCount);
    uint32_t maxSnowSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * snowObjectCount);
    uint32_t maxLitSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * litObjectCount);

    // Descriptor pool
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = maxNormalSets + maxSnowSets + maxLitSets;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = maxNormalSets + maxSnowSets + maxLitSets;
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

    // Frames in flight
    std::vector<Frame*> framesInFlight(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        framesInFlight[i] = new Frame(physicalDevice, device, swapChain, framebuffers,
                                     graphicsQueue, commandPool, descriptorPool,
                                     scene->getDescriptorSetLayout());
        
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

        // Schiff animation
        dutchAngle += deltaTime * glm::radians(5.0f);
        float radius = 60.0f;
        float circleX = radius * cos(dutchAngle);
        float circleY = radius * sin(dutchAngle);
        modelDutch = glm::mat4(1.0f);
        modelDutch = glm::translate(modelDutch, glm::vec3(circleX, -10.0f, circleY));
        modelDutch = glm::rotate(modelDutch, -1.75f - dutchAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        modelDutch = glm::scale(modelDutch, glm::vec3(2.0f, 2.0f, 2.0f));
        scene->updateObject(dutchIndex, modelDutch);

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
    delete mirrorSystem;
    snow->destroy();
    delete snow;
    delete camera;
    delete scene;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        delete framesInFlight[i];
    }
    inst.destroyDescriptorPool(device, descriptorPool);
    inst.destroyDescriptorSetLayout(device, descriptorSetLayout);
    inst.destroyDescriptorSetLayout(device, snowDescriptorSetLayout);
    inst.destroyDescriptorSetLayout(device, litDescriptorSetLayout);
    inst.destroyCommandPool(device, commandPool);
    delete framebuffers;
    delete depthBuffer;
    delete swapChain;
    inst.destroyDevice(device);
    delete surface;
    inst.destroyInstance(instance);
    delete window;

    return EXIT_SUCCESS;
}
