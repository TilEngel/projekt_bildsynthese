#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window {
public:

    // initialize GLFW
    // create GLFW window (use _window)
    // set window user pointer to this object
    // set framebuffer size callback to static method framebufferResizeCallback
    Window();

    // destroy GLFW window
    // terminate GLFW
    ~Window();

    // return all instance extensions required by GLFW
    std::vector<const char *> getRequiredExtensions();

    // return true if the window should close
    bool shouldClose();

    // return value of member variable _framebufferResized
    // reset _framebufferResized to false
    bool wasResized();

    // return width and height of the GLFW framebuffer
    VkExtent2D getExtent();

    // poll GLFW events
    void pollEvents();
    
    // create a surface for this window
    VkSurfaceKHR createSurface(VkInstance instance);
    
private:

    // get the window user pointer (Window object, "this" in the constructor)
    // set of the Window object's member variable _framebufferResized to true
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* _window = nullptr;
    bool _framebufferResized = false;
};

