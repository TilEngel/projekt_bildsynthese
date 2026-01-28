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
#include "helper/renderToTexture/CubemapRenderTarget.hpp"
#include "helper/renderToTexture/ReflectionProbe.hpp"

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

    //######### Objekte erstellen #################

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

    // Licht 1 (Beim Zwerg)
    LightSourceObject light1 = factory.createLightSource(
        glm::vec3(1.4f, 3.2f, -19.5f),
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
        glm::vec3(-10.0f, 14.0f, -16.0f),
        glm::vec3(0.7f, 0.7f, 0.7f),
        3.0f,
        1000.0f,
        renderPass
    );
    scene->addLightSource(light2);
    scene->setRenderObject(light2.renderObject);

    //Fliege, die die Kamera darstellt
    glm::mat4 modelCamera = glm::mat4(1.0f);
    RenderObject cam = factory.createGenericObject(
        "./models/fly.obj",
        "textures/black.png",
        modelCamera, renderPass);
        scene->setRenderObject(cam);
        size_t camIndex = scene->getObjectCount()-1;
    

    //Monobloc Gartenstuhl
    glm::mat4 modelChair = glm::mat4(1.0f);
    modelChair = glm::translate(modelChair, glm::vec3(2.0f, -0.8f, -20.0f));
    modelChair = glm::scale(modelChair, glm::vec3(6.0f, 6.0f, 6.0f));
    DeferredRenderObject chair = factory.createDeferredObject(
        "./models/plastic_monobloc_chair.obj",
        "textures/plastic_monobloc_chair.jpg",
        modelChair, renderPass);
    scene->setDeferredRenderObject(chair);
    size_t chairIndex = scene->getObjectCount() - 1;

    // //Fliegender Holländer
    glm::mat4 modelDutch = glm::mat4(1.0f);
    RenderObject dutch = factory.createGenericObject(
        "./models/flying_dutchman.obj",
        "textures/duck.jpg",
        modelDutch, renderPass);
    scene->setRenderObject(dutch);
    size_t dutchIndex = scene->getObjectCount() - 1;

    //Gartenzwerg
    glm::mat4 modelGnome = glm::mat4(1.0f);
    modelGnome = glm::translate(modelGnome, glm::vec3(2.0f, 1.95f, -20.0f));
    modelGnome = glm::scale(modelGnome, glm::vec3(6.0f, 6.0f, 6.0f));
    DeferredRenderObject gnome = factory.createDeferredObject(
        "./models/garden_gnome.obj",
        "textures/garden_gnome.jpg",
        modelGnome, renderPass);
    scene->setDeferredRenderObject(gnome);
    size_t gnomeIndex = scene->getObjectCount() - 1;

    // Sonnenschirm
    glm::mat4 modelUmbrella = glm::mat4(1.0f);
    modelUmbrella = glm::translate(modelUmbrella, glm::vec3(5.0f, -1.0f, -19.0f));
    modelUmbrella = glm::scale(modelUmbrella, glm::vec3(0.08f, 0.08f, 0.08f));
    modelUmbrella = glm::rotate(modelUmbrella, glm::radians(-100.0f), glm::vec3(1.0f,0.0f,0.0f));
    DeferredRenderObject umbrella = factory.createDeferredObject("./models/sonnenschirm.obj",
        "textures/sonnenschirm.jpg", modelUmbrella, renderPass);
    scene->setDeferredRenderObject(umbrella);
    size_t umbrellaIndex = scene->getObjectCount() - 1;

    // Lampe
    glm::mat4 modelLamp = glm::mat4(1.0f);
    modelLamp = glm::translate(modelLamp, glm::vec3(-10.0f, 2.8f, -20.0f));
    modelLamp = glm::scale(modelLamp, glm::vec3(0.01f, 0.01f, 0.01f));
    modelLamp = glm::rotate(modelLamp, glm::radians(-90.0f), glm::vec3(0.0f,1.0f,0.0f));
    DeferredRenderObject lamp = factory.createDeferredObject("./models/StreetLamp.obj",
        "textures/mirror.jpg", modelLamp, renderPass);
    scene->setDeferredRenderObject(lamp);
    
    //Graffitti
    glm::mat4 modelGraffitti = glm::mat4(1.0f);
    modelGraffitti = glm::translate(modelGraffitti, glm::vec3(5.0f,1.8f,6.0f));
    modelGraffitti = glm::rotate(modelGraffitti, glm::radians(180.0f), glm::vec3(1.0f,0.0f,1.0f));
    RenderObject graffitti = factory.createGraffitti(modelGraffitti, renderPass);
    scene->setRenderObject(graffitti);

    //barrier
    glm::mat4 modelBarrier = glm::mat4(1.0f);

    modelBarrier = glm::translate(modelBarrier, glm::vec3(5.6f,0.0f,5.0f));
    modelBarrier = glm::scale(modelBarrier, glm::vec3(3.0f,3.0f,3.0f));
    modelBarrier = glm::rotate(modelBarrier, glm::radians(-90.0f), glm::vec3(0.0f,1.0f,0.0f));
    modelBarrier = glm::rotate(modelBarrier, glm::radians(4.0f), glm::vec3(1.0f,0.0f,0.0f));
    RenderObject barrier = factory.createGenericObject(
        "./models/barrier.obj",
        "textures/barrier.jpg",
        modelBarrier, renderPass);
    scene->setRenderObject(barrier);

    // Straße
    glm::mat4 modelStreet = glm::mat4(1.0f);
    modelStreet = glm::scale(modelStreet, glm::vec3(12.0f, 6.0f, 6.0f));
    //modelGround = glm::rotate(modelGround, glm::radians(90.0f), glm::vec3(0.0f,1.0f,0.0f));
    modelStreet = glm::translate(modelStreet, glm::vec3(0.0,-1.12f,0.0f));
    RenderObject street = factory.createGenericObject("./models/untitled.obj",
        "textures/street.jpg", modelStreet, renderPass);
    scene->setRenderObject(street); 
    

    //Wüste
    glm::mat4 modelGround = glm::mat4(1.0f);
    modelGround = glm::scale(modelGround, glm::vec3(1.2f, 0.5f, 1.0f));
    //modelGround = glm::rotate(modelGround, glm::radians(5.0f), glm::vec3(0.9f,0.0f,0.7f));
    modelGround = glm::rotate(modelGround, glm::radians(90.0f), glm::vec3(0.05f,1.0f,0.05f));
    modelGround = glm::translate(modelGround, glm::vec3(0.0,-6.5f,0.0f));
    RenderObject ground = factory.createGenericObject("./models/Desert.obj",
        "textures/desert.png", modelGround, renderPass);
    scene->setRenderObject(ground); 

    //Tisch unter der reflektierenden Kugel
    glm::mat4 modelTable = glm::mat4(1.0f);
    modelTable = glm::translate(modelTable, glm::vec3(-15.0f, -1.0f,-40.0f));
    modelTable= glm::scale(modelTable, glm::vec3(2.0f,2.0f,2.0f));
    RenderObject table = factory.createGenericObject("./models/table.obj",
        "textures/table.jpg", modelTable, renderPass);

    scene->setRenderObject(table);

    //Kakteen
    glm::mat4 baseKaktus = glm::mat4(1.0);
    baseKaktus = glm::scale(baseKaktus, glm::vec3(0.05f,0.05f,0.05f)); 
    baseKaktus = glm::rotate(baseKaktus, glm::radians(-90.0f), glm::vec3(1.0f,0.0f,0.0f)); 
    int kaktusCount = 7;
    float radius = 8.0f;
    glm::vec3 center(-15.0f, -2.0f, -40.0f);
    
    for (int i = 0; i < kaktusCount; ++i) {

    float angle = glm::two_pi<float>() * i / kaktusCount;

    float x = center.x + radius * cos(angle);
    float z = center.z + radius * sin(angle);

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, glm::vec3(x, center.y, z));
    model = glm::rotate(model, -angle + glm::half_pi<float>(),
                         glm::vec3(0.0f, 1.0f, 0.0f));
    model *= baseKaktus;

    RenderObject kaktus = factory.createGenericObject("./models/cactus.obj","textures/cactus.jpg",model,renderPass);

    scene->setRenderObject(kaktus);
}

    
    // Reflektierende (magische) Kugel
    ReflectionProbe* reflectionProbe = new ReflectionProbe(
        device,
        physicalDevice,
        commandPool,
        glm::vec3(-15.0f, 1.5f, -40.0f),
        1024  // Auflösung
    );
    glm::mat4 modelReflective = glm::mat4(1.0f);
    modelReflective = glm::translate(modelReflective, glm::vec3(-15.0f, 0.5f, -40.0f));
    modelReflective = glm::scale(modelReflective, glm::vec3(0.33f, 0.33f, 0.33f));
    
    RenderObject reflectiveSphere = factory.createReflectiveObject(
        "./models/sphere.obj",reflectionProbe, modelReflective,renderPass
    );
    scene->setRenderObject(reflectiveSphere);
    size_t reflectiveIndex = scene->getObjectCount() - 1;
    scene->markObjectAsReflective(reflectiveIndex);
    scene->setReflectionUpdateInterval(3);

    // Schneeflocken zuletzt hinzufügen
    RenderObject snowflakes = factory.createSnowflake(
        "textures/snowflake.png",
        renderPass,
        snow->getCurrentBuffer(),
        snowDescriptorSetLayout);
    scene->setRenderObject(snowflakes);

    //####### Spiegel System Setup ##############
    
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
    mirrorSystem->addReflectableObject(gnomeIndex);
    mirrorSystem->addReflectableObject(chairIndex);
    mirrorSystem->addReflectableObject(umbrellaIndex);
    mirrorSystem->addReflectableObject(camIndex);
    scene->markObjectAsReflectable(gnomeIndex);
    scene->markObjectAsReflectable(chairIndex);
    scene->markObjectAsReflectable(umbrellaIndex);
    scene->markObjectAsReflectable(camIndex);
    
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

    //Descriptor Sets & -Pool

    //normale Descriptor Sets:
    size_t normalDescriptorSets = (scene->getDeferredObjectCount() * 2) + normalForwardCount;
    //snow Descriptor Sets
    size_t snowDescriptorSets = snowCount;
    // Lit Descriptor Sets
    size_t litDescriptorSets = litCount;
    //Lighting Descriptor Sets
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
                                    graphicsQueue, commandPool);
        
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

    //######## Render Loop ###########

    float lastTime = static_cast<float>(glfwGetTime());
    float dutchAngle = 0.0f;
    uint32_t currentFrame = 0;
    
    while (!window->shouldClose()) {
        window->pollEvents();

        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Maus-input
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

        //Tastatur-Input
        modelCamera = camera->checkKeyboard(window, deltaTime);
        if (window->getKey(GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true;
        }
        scene->updateObject(camIndex,modelCamera);
        mirrorSystem->updateReflections(scene, camIndex);

        //Schiff animation
        dutchAngle += deltaTime * glm::radians(5.0f);
        float radius = 60.0f;
        float circleX = radius * cos(dutchAngle);
        float circleY = radius * sin(dutchAngle);
        modelDutch = glm::mat4(1.0f);
        modelDutch = glm::translate(modelDutch, glm::vec3(circleX, -10.0f, circleY));
        modelDutch = glm::rotate(modelDutch, -1.75f - dutchAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        modelDutch = glm::scale(modelDutch, glm::vec3(2.0f, 2.0f, 2.0f));
        scene->updateObject(dutchIndex, modelDutch);

        //Kugel schwebt über Tisch
        modelReflective = glm::mat4(1.0f);
        modelReflective = glm::translate(modelReflective, glm::vec3(-15.0f, 1.5+ 0.25*sin(currentTime), -40.0f));
        modelReflective = glm::scale(modelReflective, glm::vec3(0.33f, 0.33f, 0.33f));
        scene->updateObject(reflectiveIndex, modelReflective);


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
        //UBOs & DescriptorSets updaten
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
        bool recreate = framesInFlight[currentFrame]->render(scene,reflectionProbe);
        if (recreate || window->wasResized()) {
            vkDeviceWaitIdle(device);
            swapChain->recreate();
            depthBuffer->recreate(swapChain->getExtent());
            framebuffers->recreate();
        }
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // ###### Cleanup in main ###########
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
    if(reflectionProbe){
        delete reflectionProbe;
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

    //Vertex-Buffer & memory zerstören
    for (const auto& [buffer, memory] : uniqueVertexBuffers) {
        vkDestroyBuffer(device, buffer, nullptr);
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
        }
    }

    // Texturen zerstören
    for (Texture* tex : uniqueTextures) {
        if (tex) {
            tex->destroy();
            delete tex;
        }
    }

    // Pipelines zerstören
    for (GraphicsPipeline* pipeline : uniquePipelines) {
        if (pipeline) {
            pipeline->destroy();
            delete pipeline;
        }
    }

    // Mirror-System
   delete mirrorSystem;

    //Snow
    snow->destroy();
    delete snow;

    //Scene
    delete scene;

    //Rendering Resources
    delete framebuffers;
    delete depthBuffer;
    delete swapChain;

    // RenderPass
    vkDestroyRenderPass(device, renderPass, nullptr);

    //Descriptor Resources
    inst.destroyDescriptorPool(device, descriptorPool);
    inst.destroyDescriptorSetLayout(device, lightingDescriptorSetLayout);
    inst.destroyDescriptorSetLayout(device, descriptorSetLayout);
    inst.destroyDescriptorSetLayout(device, snowDescriptorSetLayout);
    inst.destroyDescriptorSetLayout(device, litDescriptorSetLayout);

    // Command Pool
    inst.destroyCommandPool(device, commandPool);

    //  Device
    inst.destroyDevice(device);

    //Instance-Level
    delete surface;
    inst.destroyInstance(instance);
    delete window;

    return EXIT_SUCCESS;
}
