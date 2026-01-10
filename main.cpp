//main.cpp
#include <cstdlib>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <map>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "helper/initInstance.hpp"
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
    if(device == VK_NULL_HANDLE){
        std::cout<<"AAAAAHHHHHHHHAHAHAHAHAHAHAHAHAHA\n";
    }
    
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
    
    Framebuffers* framebuffers = new Framebuffers(device,physicalDevice, swapChain, depthBuffer, renderPass);
    
    std::cout << "\nValidating G-Buffer setup..." << std::endl;
    VkImageView gBufferView = framebuffers->getGBufferNormalView();
    if (gBufferView == VK_NULL_HANDLE) {
        throw std::runtime_error("G-Buffer view is NULL!");
    }
    std::cout << "G-Buffer view valid: " << (gBufferView != VK_NULL_HANDLE) << std::endl;

    VkDescriptorSetLayout descriptorSetLayout = inst.createStandardDescriptorSetLayout(device);
    VkDescriptorSetLayout snowDescriptorSetLayout = inst.createSnowDescriptorSetLayout(device);
    VkDescriptorSetLayout litDescriptorSetLayout = inst.createLitDescriptorSetLayout(device);
    VkDescriptorSetLayout lightingDescriptorSetLayout = inst.createLightingDescriptorSetLayout(device);
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
    RenderObject skybox = factory.createSkybox(renderPass, skyboxFaces, static_cast<uint32_t>(SubpassIndex::LIGHTING));
    scene->setRenderObject(skybox);

    // Licht 1 (Beim Zwerg)
    LightSourceObject light1 = factory.createLightSource(
        glm::vec3(-2.3f, 3.0f, 0.2f),
        glm::vec3(1.0f, 0.5f, 0.5f),
        5.0f,
        10.0f,
        renderPass
    );
    scene->addLightSource(light1);
    scene->setRenderObject(light1.renderObject);

    // Licht 2 (Bei der Lampe)
    //glm::vec3(-8.2f, 12.2f, -6.5f)
    LightSourceObject light2 = factory.createLightSource(
        glm::vec3(-8.0f, 12.2f, -6.0f),
        glm::vec3(0.0f, 0.1f, 0.7f),
        5.0f,
        300.0f,
        renderPass
    );
    scene->addLightSource(light2);
    scene->setRenderObject(light2.renderObject);

    //Monobloc Gartenstuhl
    glm::mat4 modelChair = glm::mat4(1.0f);
    modelChair = glm::translate(modelChair, glm::vec3(-2.0f, 0.92f, 0.0f));
    modelChair = glm::scale(modelChair, glm::vec3(3.0f, 3.0f, 3.0f));
    DeferredRenderObject chair = factory.createDeferredLitObject(
        "./models/plastic_monobloc_chair.obj",
        "textures/plastic_monobloc_chair.jpg",
        modelChair, renderPass);
    scene->setDeferredRenderObject(chair);
    size_t chairIndex = scene->getDeferredObjectCount() - 1;

    // //Fliegender Holländer
    glm::mat4 modelDutch = glm::mat4(1.0f);
    RenderObject dutch = factory.createGenericObject(
        "./models/flying_dutchman.obj",
        "shaders/test.vert.spv",
        "shaders/testapp.frag.spv",
        "textures/duck.jpg",
        modelDutch, renderPass, PipelineType::STANDARD, static_cast<uint32_t>(SubpassIndex::LIGHTING));
    scene->setRenderObject(dutch);
    size_t dutchIndex = scene->getObjectCount() - 1;

    //Gartenzwerg
    glm::mat4 modelGnome = glm::mat4(1.0f);
    modelGnome = glm::translate(modelGnome, glm::vec3(-2.0f, 2.25f, 0.0f));
    modelGnome = glm::scale(modelGnome, glm::vec3(3.0f, 3.0f, 3.0f));
    DeferredRenderObject gnome = factory.createDeferredLitObject(
        "./models/garden_gnome.obj",
        "textures/garden_gnome.jpg",
        modelGnome, renderPass);
    scene->setDeferredRenderObject(gnome);
    size_t gnomeIndex = scene->getDeferredObjectCount() - 1;

    // Sonnenschirm
    glm::mat4 modelUmbrella = glm::mat4(1.0f);
    modelUmbrella = glm::translate(modelUmbrella, glm::vec3(-1.0f, 0.3f, 0.0f));
    modelUmbrella = glm::scale(modelUmbrella, glm::vec3(0.04f, 0.04f, 0.04f));
    modelUmbrella = glm::rotate(modelUmbrella, glm::radians(-100.0f), glm::vec3(1.0f,0.0f,0.0f));
    RenderObject umbrella = factory.createGenericObject("./models/sonnenschirm.obj", "shaders/test.vert.spv", "shaders/testapp.frag.spv",
        "textures/sonnenschirm.jpg", modelUmbrella, renderPass,PipelineType::STANDARD,static_cast<uint32_t>(SubpassIndex::LIGHTING));
    scene->setRenderObject(umbrella);
    size_t umbrellaIndex = scene->getObjectCount() - 1;

    // Lampe
    glm::mat4 modelLamp = glm::mat4(1.0f);
    modelLamp = glm::translate(modelLamp, glm::vec3(-10.0f, 0.0f, -10.0f));
    modelLamp = glm::scale(modelLamp, glm::vec3(20.0f, 20.0f, 20.0f));
    modelLamp = glm::rotate(modelLamp, glm::radians(90.0f), glm::vec3(0.0f,1.0f,0.0f));
    DeferredRenderObject lamp = factory.createDeferredObject("./models/desk_lamp.obj",
        "textures/desk_lamp.jpg", modelLamp, renderPass);
    scene->setDeferredRenderObject(lamp);
    size_t lampIndex = scene->getDeferredObjectCount() - 1;

    // Boden
    glm::mat4 modelGround = glm::mat4(1.0f);
    modelGround = glm::scale(modelGround, glm::vec3(20.0f, 10.0f, 20.0f));
    RenderObject ground = factory.createGenericObject("./models/wooden_bowl.obj",
        "shaders/test.vert.spv",
        "shaders/testapp.frag.spv",
        "textures/wooden_bowl.jpg", modelGround, renderPass,PipelineType::STANDARD,static_cast<uint32_t>(SubpassIndex::LIGHTING));
    scene->setRenderObject(ground);
    size_t groundfIndex = scene->getObjectCount() - 1;

    // Schneeflocken ZULETZT hinzufügen
    RenderObject snowflakes = factory.createSnowflake(
        "textures/snowflake.png",
        renderPass,
        snow->getCurrentBuffer(),
        snowDescriptorSetLayout,
        static_cast<uint32_t>(SubpassIndex::LIGHTING)
    );
    scene->setRenderObject(snowflakes);

    //========== SPIEGEL-SYSTEM SETUP ==========
    
    MirrorSystem* mirrorSystem = new MirrorSystem(device, &factory, renderPass);
    
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
    mirror2.scale = glm::vec3(3.5f, 4.5f, 0.1f);
    mirrorSystem->addMirror(scene, mirror2);
    
    // Objekte markieren, die gespiegelt werden sollen
//    mirrorSystem->addReflectableObject(gnomeIndex);
    mirrorSystem->addReflectableObject(chairIndex);
    mirrorSystem->addReflectableObject(umbrellaIndex);
//    scene->markObjectAsReflectable(gnomeIndex);
    scene->markObjectAsReflectable(chairIndex);
    scene->markObjectAsReflectable(umbrellaIndex);
    
    // Reflexionen erstellen
    mirrorSystem->createReflections(scene);

    // Lighting Quad für deferred Shading
    std::cout << "Creating lighting quad..." << std::endl;
    RenderObject lightingQuad = factory.createLightingQuad(
        renderPass,
        lightingDescriptorSetLayout
    );
    scene->setLightingQuad(lightingQuad);
    std::cout << "Lighting quad created successfully!" << std::endl;


    
    // Zähle Forward Objects nach Typ
    size_t normalForwardCount = 0;
    size_t snowCount = 0;
    size_t litCount = 0;

    for (size_t i = 0; i < scene->getObjectCount(); i++) {
        const auto& obj = scene->getObject(i);
        if (obj.isDeferred) continue;  // Deferred werden separat gezählt
        
        if (obj.isSnow) {
            snowCount++;
        } else if (obj.isLit) {
            litCount++;
        } else {
            normalForwardCount++;
        }
    }

    // ========== DESCRIPTOR SET COUNTS ==========

    // 1. Normale Descriptor Sets:
    //    - Deferred Objects: 2 pro object (depth pass + gbuffer pass)
    //    - Normal Forward: 1 pro object
    size_t normalDescriptorSets = (scene->getDeferredObjectCount() * 2) + normalForwardCount;
    // 2. Snow Descriptor Sets
    size_t snowDescriptorSets = snowCount;
    // 3. Lit Descriptor Sets
    size_t litDescriptorSets = litCount;
    // 4. Lighting Descriptor Sets
    size_t lightingDescriptorSets = scene->hasLightingQuad() ? 1 : 0;

    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t maxNormalSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * normalDescriptorSets);
    uint32_t maxSnowSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * snowDescriptorSets);
    uint32_t maxLitSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * litDescriptorSets);
    uint32_t maxLightingSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * lightingDescriptorSets);

    // Descriptor pool
    std::array<VkDescriptorPoolSize, 4> poolSizes{};

    // UBOs: Normal + Snow + Lit + Lighting 
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = maxNormalSets + maxSnowSets + maxLitSets+ maxLightingSets;

    // Samplers: Normal + Snow + Lit
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = maxNormalSets + maxSnowSets + maxLitSets;

    // Storage Buffers: Nur Snow
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = maxSnowSets;

    // Input Attachments: Nur Lighting (1 pro set)
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    poolSizes[3].descriptorCount = maxLightingSets * 3;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxNormalSets + maxSnowSets + maxLitSets + maxLightingSets;
    //Descriptor Pool
    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool in main");
    }

    // Frames in flight
    std::vector<Frame*> framesInFlight(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        std::cout << "\n=== Frame " << i << " Initialization ===" << std::endl;
        
        framesInFlight[i] = new Frame(physicalDevice, device, swapChain, framebuffers,
                                    graphicsQueue, commandPool, descriptorPool,
                                    scene->getDescriptorSetLayout());
        
        // Normale Descriptor Sets
        std::cout << "Allocating " << normalDescriptorSets << " normal descriptor sets..." << std::endl;
        framesInFlight[i]->allocateDescriptorSets(descriptorPool, descriptorSetLayout, normalDescriptorSets);
        
        // Snow Descriptor Sets
        if (snowDescriptorSets > 0) {
            std::cout << "Allocating " << snowDescriptorSets << " snow descriptor sets..." << std::endl;
            framesInFlight[i]->allocateSnowDescriptorSets(descriptorPool, snowDescriptorSetLayout, snowDescriptorSets);
        }
        
        // Lit Descriptor Sets
        if (litDescriptorSets > 0) {
            std::cout << "Allocating " << litDescriptorSets << " lit descriptor sets..." << std::endl;
            framesInFlight[i]->allocateLitDescriptorSets(descriptorPool, litDescriptorSetLayout, litDescriptorSets);
        }
        
        // Lighting Descriptor Sets
        if (lightingDescriptorSets > 0) {
            std::cout << "Allocating " << lightingDescriptorSets << " lighting descriptor sets..." << std::endl;
            framesInFlight[i]->allocateLightingDescriptorSets(descriptorPool, lightingDescriptorSetLayout, lightingDescriptorSets);
        }
        
        std::cout << "Frame " << i << " initialized successfully!" << std::endl;
    }

    std::cout << "\n=== All frames initialized! ===" << std::endl;
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

        // // Schiff animation
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
        framesInFlight[currentFrame]->updateLightingUniformBuffer(camera,scene);
        framesInFlight[currentFrame]->updateDescriptorSet(scene);
        if (litCount > 0) {
            framesInFlight[currentFrame]->updateLitDescriptorSet(scene);
        }
                
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
        if (scene->hasLightingQuad()) {
            framesInFlight[currentFrame]->updateLightingDescriptorSet(
                framebuffers->getGBufferNormalView(),
                framebuffers->getGBufferAlbedoView(),
                depthBuffer->getImageView()
            );
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

    // ========== CLEANUP ==========
    vkDeviceWaitIdle(device);

    // 1. Frames zerstören
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        delete framesInFlight[i];
    }

    // 2. Sammle unique Ressourcen
    std::set<GraphicsPipeline*> uniquePipelines;
    std::set<Texture*> uniqueTextures;
    std::map<VkBuffer, VkDeviceMemory> uniqueVertexBuffers;  // Buffer + Memory

    // Normale Objekte
    for (size_t i = 0; i < scene->getObjectCount(); i++) {
        const RenderObject& obj = scene->getObject(i);
        
        if (obj.texture) {
            uniqueTextures.insert(obj.texture);
        }
        
        if (obj.pipeline) {
            uniquePipelines.insert(obj.pipeline);
        }
        
        if (obj.vertexBuffer != VK_NULL_HANDLE) {
            uniqueVertexBuffers[obj.vertexBuffer] = obj.vertexBufferMemory;
        }
    }

    // Reflektierte Objekte (teilen sich Ressourcen!)
    for (size_t i = 0; i < scene->getReflectedObjectCount(); i++) {
        const RenderObject& obj = scene->getReflectedObject(i);
        
        if (obj.texture) {
            uniqueTextures.insert(obj.texture);
        }
        
        if (obj.pipeline) {
            uniquePipelines.insert(obj.pipeline);
        }
        
        if (obj.vertexBuffer != VK_NULL_HANDLE) {
            uniqueVertexBuffers[obj.vertexBuffer] = obj.vertexBufferMemory;
        }
    }

    // 3. Vertex-Buffer UND Memory zerstören
    for (const auto& [buffer, memory] : uniqueVertexBuffers) {
        vkDestroyBuffer(device, buffer, nullptr);
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
        }
    }

    // 4. Texturen zerstören
    for (Texture* tex : uniqueTextures) {
        if (tex) {
            tex->destroy();
            delete tex;
        }
    }

    // 5. Pipelines zerstören
    for (GraphicsPipeline* pipeline : uniquePipelines) {
        if (pipeline) {
            pipeline->destroy();
            delete pipeline;
        }
    }

    // 6. Mirror-System
   delete mirrorSystem;

    // 7. Snow
    snow->destroy();
    delete snow;

    // 8. Scene
    delete scene;

    // 9. Rendering Resources
    delete framebuffers;
    delete depthBuffer;
    delete swapChain;

    // 10. RenderPass
    vkDestroyRenderPass(device, renderPass, nullptr);

    // 11. Descriptor Resources
    inst.destroyDescriptorPool(device, descriptorPool);
    inst.destroyDescriptorSetLayout(device, lightingDescriptorSetLayout);
    inst.destroyDescriptorSetLayout(device, descriptorSetLayout);
    inst.destroyDescriptorSetLayout(device, snowDescriptorSetLayout);
    inst.destroyDescriptorSetLayout(device, litDescriptorSetLayout);

    // 12. Command Pool
    inst.destroyCommandPool(device, commandPool);

    // 13. Device
    inst.destroyDevice(device);

    // 14. Instance-Level
    delete surface;
    inst.destroyInstance(instance);
    delete window;

    return EXIT_SUCCESS;
}
