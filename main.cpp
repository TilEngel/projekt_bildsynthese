//main.cpp
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
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
#include "helper/RenderPass.hpp"
#include "helper/Camera.hpp"

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
    //DescriptorSet layout 
    VkDescriptorSetLayout descriptorSetLayout = inst.createStandardDescriptorSetLayout(device);
    scene->setDescriptorSetLayout(descriptorSetLayout);

//Erstellen der Objekte #######################

    ObjectFactory factory(physicalDevice,device,commandPool,graphicsQueue,swapChain->getImageFormat(), depthBuffer->getImageFormat(),descriptorSetLayout);

    //Skybox
    std::array<const char*, 6> skyboxFaces = {
        "textures/skybox/right.jpg",    //rechts
        "textures/skybox/left.jpg",     //links
        "textures/skybox/top.jpg",      //oben
        "textures/skybox/bottom.jpg",   //unten
        "textures/skybox/front.jpg",    //vorne
        "textures/skybox/back.jpg"      //hinten
    };
    RenderObject skybox = factory.createSkybox(renderPass, skyboxFaces);
    scene->setRenderObject(skybox);


    //Monobloc Gartenstuhl
    glm::mat4 modelChair = glm::mat4(1.0f);
    modelChair = glm::translate(modelChair, glm::vec3(-2.0f,0.92f,0.0f));
    modelChair = glm::scale(modelChair, glm::vec3(3.0f,3.0f,3.0f));
    //Model-Matrix wird im Render-Loop gemacht
    RenderObject chair = factory.createGenericObject("./models/plastic_monobloc_chair.obj", "shaders/testapp.vert.spv", "shaders/testapp.frag.spv", "textures/plastic_monobloc_chair.jpg", modelChair, renderPass);
    //Zu Szene hinzufügen
    scene->setRenderObject(chair);

    //Fliegender Holländer
    glm::mat4 modelDutch = glm::mat4(1.0f);
    //modelDutch= glm::rotate(modelDutch, -90.0f, glm::vec3(0.0f,1.0f,0.0f));
    RenderObject dutch = factory.createGenericObject("./models/flying_dutchman.obj", "shaders/test.vert.spv", "shaders/testapp.frag.spv", "textures/duck.jpg", modelDutch, renderPass);
    scene->setRenderObject(dutch);

    //Gartenzwerg
    glm::mat4 modelGnome = glm::mat4(1.0f);
    modelGnome = glm::translate(modelGnome, glm::vec3(-2.0f,2.25f,0.0f));
    modelGnome = glm::scale(modelGnome, glm::vec3(3.0f,3.0f,3.0f));
    //Model-Matrix wird im Render-Loop gemacht
    RenderObject gnome = factory.createGenericObject("./models/garden_gnome.obj", "shaders/testapp.vert.spv", "shaders/testapp.frag.spv", "textures/garden_gnome.jpg", modelGnome, renderPass);
    //Zu Szene hinzufügen
    scene->setRenderObject(gnome);

    //Boden
    glm::mat4 modelGround = glm::mat4(1.0f);
    modelGround = glm::scale(modelGround, glm::vec3(20.0f,10.0f,20.0f));
    RenderObject ground = factory.createGround(modelGround, renderPass);
    scene->setRenderObject(ground);

    // vertikaler Spiegel
    glm::mat4 modelMirror = glm::mat4(1.0f);
    modelMirror = glm::translate(modelMirror, glm::vec3(-2.0f, 2.25f, 2.0f));
    modelMirror = glm::rotate(modelMirror, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMirror = glm::scale(modelMirror, glm::vec3(5.0f, 5.0f, 5.0f));
    RenderObject mirror = factory.createMirror(modelMirror, renderPass);
    int mirrorIdx = scene->setRenderObject(mirror);
    scene->setMirrorIndex(mirrorIdx); // Wichtig!


    // Object count
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
            scene->getDescriptorSetLayout());
            framesInFlight[i]->allocateDescriptorSets(descriptorPool, scene->getDescriptorSetLayout(), scene->getObjectCount());
    }

// start render loop ########################
    float lastTime = static_cast<float>(glfwGetTime());
    float dutchAngle = 0.0f;
    uint32_t currentFrame = 0;
    while (!window->shouldClose()) {
        window->pollEvents();

        // Berechne Delta-Time
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        //maus-input verarbeiten
        double xpos, ypos;
        window->getCursorPos(&xpos, &ypos);

        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = static_cast<float>(xpos - lastX);
        float yoffset = static_cast<float>(lastY - ypos); // Umgekehrt: y geht von oben nach unten
        lastX = xpos;
        lastY = ypos;

        //Tastatureingabe checken
        camera->processMouseMovement(xoffset, yoffset);
        camera->checkKeyboard(window, deltaTime);
        if (window->getKey(GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true; //Reset falls man wieder zurück in capture mode geht
        }


       // Fliegender Holländer: Kreisbewegung um den Ursprung
        dutchAngle += deltaTime * glm::radians(5.0f); // Winkel erhöhen
        
        float radius = 60.0f;
        float circleX = radius * cos(dutchAngle);
        float circleY = radius * sin(dutchAngle);
        
        modelDutch = glm::mat4(1.0f);
        modelDutch = glm::translate(modelDutch, glm::vec3(circleX, -10.0f, circleY)); // Y=3.0 (Höhe)
        modelDutch = glm::rotate(modelDutch, -1.75f-dutchAngle, glm::vec3(0.0f, 1.0f, 0.0f)); // Optional: Rotation des Modells
        modelDutch = glm::scale(modelDutch, glm::vec3(2.0f, 2.0f, 2.0f));
        scene->updateObject(2,modelDutch);
    
        //Basic stuff
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
    //TODO: cleanup überarbeiten 
    delete camera;
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
