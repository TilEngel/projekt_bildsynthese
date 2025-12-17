// Window.cpp
#include "Window.hpp"

#include <stdexcept>
#include <vector>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Standardfenstergröße (kann nach Bedarf geändert werden)
static const uint32_t WINDOW_WIDTH  = 800;
static const uint32_t WINDOW_HEIGHT = 600;

Window::Window() {
    // GLFW initialisieren
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Keine OpenGL-Kontext-Erzeugung (wir verwenden Vulkan)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Erzeuge das GLFW-Fenster
    _window = glfwCreateWindow(static_cast<int>(WINDOW_WIDTH),
                               static_cast<int>(WINDOW_HEIGHT),
                               "Vulkan Window",
                               nullptr,
                               nullptr);
    if (!_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    // setze das "user pointer" auf dieses Objekt, damit der statische Callback Zugriff hat
    glfwSetWindowUserPointer(_window, this);

    // setze framebuffer resize callback
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}

Window::~Window() {
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
    glfwTerminate();
}

std::vector<const char *> Window::getRequiredExtensions() {
    uint32_t count = 0;
    // glfwGetRequiredInstanceExtensions gibt ein const char** zurück (oder nullptr)
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> result;
    if (extensions) {
        result.assign(extensions, extensions + count);
    }
    return result;
}

bool Window::shouldClose() {
    if (!_window) return true;
    return glfwWindowShouldClose(_window);
}

bool Window::wasResized() {
    bool resized = _framebufferResized;
    _framebufferResized = false;
    return resized;
}

VkExtent2D Window::getExtent() {
    if (!_window) {
        return VkExtent2D{0, 0};
    }
    int width = 0, height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    // Falls das Fenster minimiert wurde, kann height==0 sein — Caller muss ggf. warten/neu prüfen
    return VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

void Window::pollEvents() {
    glfwPollEvents();
}

VkSurfaceKHR Window::createSurface(VkInstance instance) {
    if (!_window) {
        throw std::runtime_error("Window is not created");
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err = glfwCreateWindowSurface(instance, _window, nullptr, &surface);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface (glfwCreateWindowSurface returned non-success)");
    }
    return surface;
}

// statischer Callback: setzt das Flag im zugehörigen Window-Objekt
void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    (void) width; (void) height;
    // wir holen uns das Window-Objekt aus dem user pointer
    Window* win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        win->_framebufferResized = true;
    }
}

//liefert die aktuelle Cursorposition im Fenster 
void Window::getCursorPos(double* xpos, double* ypos){
    if(_window){
        glfwGetCursorPos(_window, xpos, ypos);
    }else{ //Falls gerade kein Window existiert
        if(xpos){
            *xpos = 0.0;
        }
        if(ypos){
            *ypos =0.0;
        }
    }
}

//setzt halt Input mode
void Window::setInputMode(int mode, int value){
    if(_window){
        glfwSetInputMode(_window, mode, value);
    }
}

//liefert, welche Taste gedrückt wird
int Window::getKey(int key){
    if(_window){
        return glfwGetKey(_window,key);
    }
    return GLFW_RELEASE; //keine Taste
}