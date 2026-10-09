#ifndef CONTROLER_HPP
#define CONTROLER_HPP

    #include <windows.h>

    struct GLFWwindow;

    struct CameraState {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawDegrees = 0.0f;
        float tiltDegrees = -60.0f;
    };

    const float MOVE_SPEED = 20.0f; 
    const float ROTATE_SPEED = 60.0f;

    inline bool isDown(int virtualKey) {
        return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
    }

    void updateCamera(GLFWwindow* window, CameraState& camera, float dt);

#endif // CONTROLER_HPP
