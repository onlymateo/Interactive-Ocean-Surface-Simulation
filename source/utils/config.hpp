#ifndef UTILS_CONFIG_HPP
#define UTILS_CONFIG_HPP

    #include <string>

    struct Config {
        int width = 800;                // window width in pixels
        int height = 600;               // window height in pixels
        std::string method = "gerstner"; // wave method: "gerstner", "perlin" or "fft"
        int cellsnumber = 20;              // number of cells in the plane

        float planeSize = 2.0f;            // width and depth of the plane (world units)

        // Camera
        double nearPlane = 0.2;
        double farPlane = 100.0;
        double halfHeightAtNear = 0.1;     // controls how wide the view is
        float cameraDistance = -3.0f;      // how far the plane is pushed away from the viewer
        float tiltDegrees = -60.0f;        // rotation around the x axis
    };

    Config loadConfig(const std::string& path);

#endif // UTILS_CONFIG_HPP
