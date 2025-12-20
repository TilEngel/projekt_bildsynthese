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
    scene->setDescriptorSetLayout(descriptorSetLayout);

    //Erstellen der Objekte #######################

    ObjectFactory factory(physicalDevice, device, commandPool, graphicsQueue,
                         swapChain->getImageFormat(), depthBuffer->getImageFormat(),
                         descriptorSetLayout);

    //Skybox
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

    //Fliegender Holländer
    glm::mat4 modelDutch = glm::mat4(1.0f);
    RenderObject dutch = factory.createGenericObject(
        "./models/flying_dutchman.obj",
        "shaders/test.vert.spv",
        "shaders/testapp.frag.spv",
        "textures/duck.jpg",
        modelDutch, renderPass, PipelineType::STANDARD);
    scene->setRenderObject(dutch);

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

    //Boden
    glm::mat4 modelGround = glm::mat4(1.0f);
    modelGround = glm::scale(modelGround, glm::vec3(20.0f, 10.0f, 20.0f));
    RenderObject ground = factory.createGround(modelGround, renderPass);
    scene->setRenderObject(ground);

    // ========== SPIEGEL KONFIGURATION ==========
    
    // Spiegel steht hinter dem Gnom (negative Z-Richtung vom Gnom aus)
    glm::vec3 mirrorPos = glm::vec3(-2.0f, 1.5f, -3.0f);
    
    // Normale zeigt in POSITIVE Z-Richtung (zum Betrachter/Kamera)
    // Das bedeutet: Der Spiegel reflektiert was VOR ihm ist (Gnom bei Z=0)
    glm::vec3 mirrorNormal = glm::vec3(0.0f, 0.0f, 1.0f);
    
    glm::mat4 mirrorTransform = glm::mat4(1.0f);
    mirrorTransform = glm::translate(mirrorTransform, mirrorPos);
    mirrorTransform = glm::scale(mirrorTransform, glm::vec3(1.5f, 2.5f, 0.1f));

    // PASS 1: Spiegel-Markierung (schreibt in Stencil)
    RenderObject mirrorMark = factory.createMirror(
        mirrorTransform, renderPass, PipelineType::MIRROR_MARK);
    scene->setMirrorMarkObject(mirrorMark);

    // ========== REFLEXIONSMATRIX BERECHNEN (KORRIGIERT) ==========
    
    // Ebenengleichung: n·x + d = 0
    // Für einen Punkt P auf der Ebene: n·P + d = 0 => d = -n·P
    float d = -glm::dot(mirrorNormal, mirrorPos);
    
    // Reflexionsmatrix für Ebene mit Normale n und Distanz d
    // R = I - 2*n*n^T für Ebene durch Ursprung
    // Mit Translation: T(-2dn) * R * T(2dn)
    
    glm::mat4 reflectionMatrix = glm::mat4(1.0f);
    
    // Diagonale: 1 - 2*n_i*n_i
    reflectionMatrix[0][0] = 1.0f - 2.0f * mirrorNormal.x * mirrorNormal.x;
    reflectionMatrix[1][1] = 1.0f - 2.0f * mirrorNormal.y * mirrorNormal.y;
    reflectionMatrix[2][2] = 1.0f - 2.0f * mirrorNormal.z * mirrorNormal.z;
    
    // Off-Diagonal: -2*n_i*n_j
    reflectionMatrix[0][1] = -2.0f * mirrorNormal.x * mirrorNormal.y;
    reflectionMatrix[0][2] = -2.0f * mirrorNormal.x * mirrorNormal.z;
    reflectionMatrix[1][0] = -2.0f * mirrorNormal.y * mirrorNormal.x;
    reflectionMatrix[1][2] = -2.0f * mirrorNormal.y * mirrorNormal.z;
    reflectionMatrix[2][0] = -2.0f * mirrorNormal.z * mirrorNormal.x;
    reflectionMatrix[2][1] = -2.0f * mirrorNormal.z * mirrorNormal.y;
    
    // Translation: -2*d*n
    reflectionMatrix[3][0] = -2.0f * d * mirrorNormal.x;
    reflectionMatrix[3][1] = -2.0f * d * mirrorNormal.y;
    reflectionMatrix[3][2] = -2.0f * d * mirrorNormal.z;
    reflectionMatrix[3][3] = 1.0f;

    std::cout << "Mirror Position: (" << mirrorPos.x << ", " << mirrorPos.y << ", " << mirrorPos.z << ")" << std::endl;
    std::cout << "Mirror Normal: (" << mirrorNormal.x << ", " << mirrorNormal.y << ", " << mirrorNormal.z << ")" << std::endl;
    std::cout << "Plane distance d: " << d << std::endl;

    // PASS 2: Gespiegelter Gartenzwerg
    glm::mat4 reflectedGnomeMatrix = reflectionMatrix * modelGnome;
    RenderObject reflectedGnome = factory.createGenericObject(
        "./models/garden_gnome.obj",
        "shaders/testapp.vert.spv",
        "shaders/testapp.frag.spv",
        "textures/garden_gnome.jpg",
        reflectedGnomeMatrix,
        renderPass,
        PipelineType::MIRROR_REFLECT
    );
    scene->addReflectedObject(reflectedGnome, gnomeIndex);

    // PASS 3: Spiegel mit Transparenz
    RenderObject mirrorBlend = factory.createMirror(
        mirrorTransform, renderPass, PipelineType::MIRROR_BLEND);
    scene->setMirrorBlendObject(mirrorBlend);

    scene->setGnomeIndex(gnomeIndex);

    // Object count
    size_t objectCount = scene->getObjectCount();
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * objectCount);

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = maxSets;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = maxSets;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSets;

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool");
    }

    std::vector<Frame*> framesInFlight(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        framesInFlight[i] = new Frame(
            physicalDevice, device, swapChain, framebuffers,
            graphicsQueue, commandPool, descriptorPool,
            scene->getDescriptorSetLayout());
        framesInFlight[i]->allocateDescriptorSets(
            descriptorPool, scene->getDescriptorSetLayout(),
            scene->getObjectCount());
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
        scene->updateObject(2, modelDutch);

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
    delete camera;
    delete scene;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        delete framesInFlight[i];
    }
    inst.destroyDescriptorPool(device, descriptorPool);
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
