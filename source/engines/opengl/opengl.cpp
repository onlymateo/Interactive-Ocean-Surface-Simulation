#include "engines/opengl/opengl.hpp"

#include <GLFW/glfw3.h>
#include <iostream>
#include "render/plane.hpp"

namespace {

const float PLANE_SIZE = 2.0f;  // width and depth of the plane, in world units
const int PLANE_CELLS = 20;     // number of squares along each side

// Camera: a perspective view looking at the origin, with the plane tilted towards the viewer.
const double NEAR_PLANE = 0.2;
const double FAR_PLANE = 100.0;
const double HALF_HEIGHT_AT_NEAR = 0.1;  // controls how wide the view is
const float CAMERA_DISTANCE = -3.0f;     // how far the plane is pushed away from the viewer
const float TILT_DEGREES = -60.0f;       // rotation around the x axis

void setupCamera(int width, int height) {
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    // Projection: how 3D points become 2D pixels.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = static_cast<double>(width) / static_cast<double>(height);
    glFrustum(-aspect * HALF_HEIGHT_AT_NEAR, aspect * HALF_HEIGHT_AT_NEAR,
              -HALF_HEIGHT_AT_NEAR, HALF_HEIGHT_AT_NEAR, NEAR_PLANE, FAR_PLANE);

    // Model-view: where the plane sits and how it is turned.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, CAMERA_DISTANCE);
    glRotatef(TILT_DEGREES, 1.0f, 0.0f, 0.0f);
}

}

int runOpenGL(int width, int height, HeightFunction wave) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(width, height, "OpenGL Window", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    Plane plane(PLANE_SIZE, PLANE_CELLS, wave);

    // Each frame: clear, move the waves to the current time, draw, then show the image.
    while (!glfwWindowShouldClose(window)) {
        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        plane.update(static_cast<float>(glfwGetTime()));
        setupCamera(framebufferWidth, framebufferHeight);
        plane.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
