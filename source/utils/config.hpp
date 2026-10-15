#ifndef UTILS_CONFIG_HPP
#define UTILS_CONFIG_HPP

    #include <string>
    #include <vector>

    struct GerstnerConfig {
        std::vector<float> k = {}; // number of waves
        std::vector<float> A = {}; // wave amplitude
        std::vector<float> w = {}; // angular frequency for deep waters as w = sqrt(9.81 * k)
        std::vector<std::vector<float>> D = {{}}; // directionnal vector of a wave
        std::vector<float> s = {}; // steepness
    };

    struct FFTConfig {
        float V = 1.0; //wind speed
        std::vector<float> D = {}; //wind direction vector
        float A = 0.000001; //phillips spectrum amplitude constant
        int seed = 42; //seed for the random Xi values
        float lambda = 1.5; //chopiness control parameter 1.0 <= Lambda <= 2.0
        int N = 16; //equals to cellsnumber
        float size = 100.0; //equals to plane size
    };

    struct RGBcolor {
        float r = 0.0;
        float g = 0.0;
        float b = 0.0;
    };

    struct SunConfig {
        float azimuthDegrees = 45.0f; // 0 = in front of the camera at the start, 90 = to the right
        float elevationDegrees = 35.0f; // 0 = on the horizon, 90 = right above
        RGBcolor color = {1.0f, 0.95f, 0.8f}; // color of the sunlight
        float intensity = 1.0f; // strength of the sunlight
        float ambient = 0.3f; // light that comes from everywhere (so the shadows are not black)
        float shininess = 64.0f; // size of the sun reflection on the water (higher = smaller)
    };

    struct SkyConfig {
        RGBcolor zenith = {0.1f, 0.3f, 0.7f}; // color right above
        RGBcolor horizon = {0.7f, 0.8f, 0.9f}; // color at the horizon
    };

    struct Config {
        int width = 800; // window width in pixels
        int height = 600; // window height in pixels

        int calculusfrequency = 30; // limits the frequency of calculus for the plane
        std::string selectedmethod = "gerstner"; // slected wave method

        // plane config
        int cellsnumber = 16; // number of cells in the plane
        float planeSize = 2.0f; // width and depth of the plane (world units)
        RGBcolor color; // color of the plane
        float baseZpos = 0.0f; // base z position of the plane
        int chunksNumber = 1; // number of chunks generated
        bool showtriangles = false; // show outlines of the triangles


        // camera config
        double nearPlane = 0.2;
        double farPlane = 100.0;
        double halfHeightAtNear = 0.1; // controls how wide the view is
        float cameraDistance = -3.0f; // how far the plane is pushed away from the viewer
        float tiltDegrees = -60.0f; // rotation around the x axis

        // light and sky
        SunConfig sun;
        SkyConfig sky;

        // gerstner config
        GerstnerConfig g;
        FFTConfig f;
    };

    inline const std::vector<std::string> selectablemethods = {"gerstner", "fft", "perlin"};



    Config loadConfig(const std::string& path);

#endif // UTILS_CONFIG_HPP

