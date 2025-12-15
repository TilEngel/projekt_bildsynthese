#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(4.0f, 4.0f, 4.0f),
           glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -135.0f,
           float pitch = -35.0f)
        : _position(position)
        , _worldUp(worldUp)
        , _yaw(yaw)
        , _pitch(pitch)
        , _movementSpeed(2.5f)
        , _mouseSensitivity(0.1f)
        , _zoom(45.0f)
    {
        updateCameraVectors();
    }

    // View-Matrix zurückgeben
    glm::mat4 getViewMatrix() const {
        return glm::lookAt(_position, _position + _front, _up);
    }

    // Mausbewegung verarbeiten
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= _mouseSensitivity;
        yoffset *= _mouseSensitivity;

        _yaw += xoffset;
        _pitch += yoffset;

        // Pitch begrenzen, damit Kamera nicht überschlägt
        if (constrainPitch) {
            if (_pitch > 89.0f)
                _pitch = 89.0f;
            if (_pitch < -89.0f)
                _pitch = -89.0f;
        }

        updateCameraVectors();
    }

    // Tastatur-Bewegung
    enum Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    //verändert Position abhängig von Bewegung
    void processKeyboard(Movement direction, float deltaTime) {
        
        float velocity = _movementSpeed * deltaTime;
        if (direction == FORWARD)
            _position += _front * velocity;
        if (direction == BACKWARD)
            _position -= _front * velocity;
        if (direction == LEFT)
            _position -= _right * velocity;
        if (direction == RIGHT)
            _position += _right * velocity;
        if (direction == UP)
            _position += _up * velocity;
        if (direction == DOWN)
            _position -= _up * velocity;
    }

    //Checkt ob Taste gedrückt wird und ruft entsprechend processKeyboard auf
    void checkKeyboard(Window* window, float deltaTime){
        
        if (window->getKey(GLFW_KEY_W) == GLFW_PRESS)
            processKeyboard(Camera::FORWARD, deltaTime);
        if (window->getKey(GLFW_KEY_S) == GLFW_PRESS)
            processKeyboard(Camera::BACKWARD, deltaTime);
        if (window->getKey(GLFW_KEY_A) == GLFW_PRESS)
            processKeyboard(Camera::LEFT, deltaTime);
        if (window->getKey(GLFW_KEY_D) == GLFW_PRESS)
            processKeyboard(Camera::RIGHT, deltaTime);
        if (window->getKey(GLFW_KEY_SPACE) == GLFW_PRESS)
            processKeyboard(Camera::UP, deltaTime);
        if (window->getKey(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            processKeyboard(Camera::DOWN, deltaTime);

    }

    // Getter
    glm::vec3 getPosition() const { return _position; }
    glm::vec3 getFront() const { return _front; }
    float getZoom() const { return _zoom; }

    // Setter
    void setPosition(const glm::vec3& pos) { _position = pos; }
    void setMovementSpeed(float speed) { _movementSpeed = speed; }
    void setMouseSensitivity(float sensitivity) { _mouseSensitivity = sensitivity; }

private:
    // Kamera-Attribute
    glm::vec3 _position;
    glm::vec3 _front;
    glm::vec3 _up;
    glm::vec3 _right;
    glm::vec3 _worldUp;

    // Euler-Winkel
    float _yaw;
    float _pitch;

    // Kamera-Optionen
    float _movementSpeed;
    float _mouseSensitivity;
    float _zoom;

    // Kamera-Vektoren aus Euler-Winkeln berechnen
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
        front.y = sin(glm::radians(_pitch));
        front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
        _front = glm::normalize(front);

        _right = glm::normalize(glm::cross(_front, _worldUp));
        _up = glm::normalize(glm::cross(_right, _front));
    }
};