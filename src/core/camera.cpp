#include "core/camera.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>

Camera::Camera(
    glm::vec3 initialPosition,
    float initialFov,
    float aspectRatio,
    float initialNearPlane,
    float initialFarPlane,
    float initialMovementSpeed,
    float initialMouseSensitivity
)
    : position(initialPosition),
      yaw(-90.0f),
      pitch(0.0f),
      movementSpeed(initialMovementSpeed),
      mouseSensitivity(initialMouseSensitivity),
      fov(initialFov),
      nearPlane(initialNearPlane),
      farPlane(initialFarPlane) {

    worldUp = glm::vec3(0.0f, -1.0f, 0.0f);
    front = glm::vec3(0.0f, 0.0f, 0.0f);

    axeX = 0;
    axeY = 0;
    axeZ = 0;

    projectionMatrix = glm::perspective(glm::radians(initialFov), aspectRatio, initialNearPlane, initialFarPlane);
    updateViewMatrix();
}

void Camera::update(float deltaTime) {
    float velocity = movementSpeed * deltaTime;

    glm::vec3 cameraRight = glm::normalize(glm::cross(front, worldUp));
    glm::vec3 forward = glm::normalize(glm::vec3(front.x, 0.0f, front.z));

    glm::vec3 moveDirection = forward * axeZ + worldUp * axeY + cameraRight * axeX;

    if (glm::length(moveDirection) > 0.0f)
        moveDirection = glm::normalize(moveDirection);

    position += moveDirection * velocity;
    updateViewMatrix();
}

int Camera::processKeyboard(int key, int action) {
    if (key == GLFW_KEY_W && action == GLFW_PRESS) axeZ++;
    if (key == GLFW_KEY_S && action == GLFW_PRESS) axeZ--;
    if (key == GLFW_KEY_A && action == GLFW_PRESS) axeX--;
    if (key == GLFW_KEY_D && action == GLFW_PRESS) axeX++;
    if (key == GLFW_KEY_C && action == GLFW_PRESS) axeY++;
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) axeY--;
    if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) movementSpeed *= 5;

    if (key == GLFW_KEY_W && action == GLFW_RELEASE) axeZ--;
    if (key == GLFW_KEY_S && action == GLFW_RELEASE) axeZ++;
    if (key == GLFW_KEY_A && action == GLFW_RELEASE) axeX++;
    if (key == GLFW_KEY_D && action == GLFW_RELEASE) axeX--;
    if (key == GLFW_KEY_C && action == GLFW_RELEASE) axeY--;
    if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) axeY++;
    if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE) movementSpeed /= 5;

    if (action != GLFW_PRESS)   return 0;
    if (key == GLFW_KEY_ESCAPE) return 1;

    return 0;
}

void Camera::processMouse(float offsetX, float offsetY) {
    offsetX *= mouseSensitivity;
    offsetY *= mouseSensitivity;

    yaw = fmod(yaw + offsetX, 360.0f);
    pitch += offsetY;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

void Camera::updateViewMatrix() {
    front = glm::normalize(glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    ));

    viewMatrix = glm::lookAt(position, position + front, worldUp);
}

void Camera::updateProjection(float aspectRatio) {
    projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}


void Camera::debug() {
    std::cout << "Position : " << position.x << " | " << position.y << " | " << position.z << std::endl;
    std::cout << "Yaw : " << yaw << std::endl;
    std::cout << "Pitch : " << pitch << std::endl << std::endl;
}
