#include "engines/controler.hpp"

#include <windows.h>
#include <GLFW/glfw3.h>
#include <cmath>

void updateCamera(GLFWwindow* window, CameraState& camera, float dt) {
    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        return;
    }

    if (isDown(37)) { // ArrowLeft
        camera.yawDegrees -= ROTATE_SPEED * dt;
    }
    if (isDown(39)) { // ArrowRight
        camera.yawDegrees += ROTATE_SPEED * dt;
    }

    if (isDown(38)) { // ArrowUp
        camera.tiltDegrees -= ROTATE_SPEED * dt;
    }
    if (isDown(40)) { // ArrowDown
        camera.tiltDegrees += ROTATE_SPEED * dt;
    }
    if (camera.tiltDegrees < -180.0f) {
        camera.tiltDegrees = -180.0f;
    }
    if (camera.tiltDegrees > 0.0f) {
        camera.tiltDegrees = 0.0f;
    }

    float yaw = camera.yawDegrees * 3.14159265358979f / 180.0f;
    float forwardX = std::sin(yaw), forwardY = std::cos(yaw);
    float rightX = std::cos(yaw), rightY = -std::sin(yaw);
    float step = MOVE_SPEED * dt;

    if (isDown('Z')) {
        camera.x += forwardX * step;
        camera.y += forwardY * step;
    }
    if (isDown('S')) {
        camera.x -= forwardX * step;
        camera.y -= forwardY * step;
    }
    if (isDown('D')) {
        camera.x += rightX * step;
        camera.y += rightY * step;
    }
    if (isDown('Q')) {
        camera.x -= rightX * step;
        camera.y -= rightY * step;
    }
    if (isDown(VK_SHIFT)) {
        camera.z += step;
    }
    if (isDown(VK_CONTROL)) {
        camera.z -= step;
    }
}
