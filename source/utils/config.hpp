#ifndef UTILS_CONFIG_HPP
#define UTILS_CONFIG_HPP

    #include <string>
    #include <vector>

    struct GerstnerConfig {
        std::vector<float> k = {};
        std::vector<float> A = {};
        std::vector<float> w = {};
        std::vector<std::vector<float>> D = {{}};
    };

    struct RGBcolor {
        float r = 0.0;
        float g = 0.0;
        float b = 0.0;
    };

    struct Config {
        int width = 800; // window width in pixels
        int height = 600; // window height in pixels

        int calculusfrequency = 30; // limits the frequency of calculus for the plane
        std::string selectedmethod = "gerstner"; // slected wave method

        // plane config
        int cellsnumber = 20; // number of cells in the plane
        float planeSize = 2.0f; // width and depth of the plane (world units)
        RGBcolor color;
        float baseZpos = 0.0f;


        // camera config
        double nearPlane = 0.2;
        double farPlane = 100.0;
        double halfHeightAtNear = 0.1; // controls how wide the view is
        float cameraDistance = -3.0f; // how far the plane is pushed away from the viewer
        float tiltDegrees = -60.0f; // rotation around the x axis

        // gerstner config
        GerstnerConfig g;
    };

    inline const std::vector<std::string> selectablemethods = {"gerstner", "fft", "perlin"};



    Config loadConfig(const std::string& path);

#endif // UTILS_CONFIG_HPP
