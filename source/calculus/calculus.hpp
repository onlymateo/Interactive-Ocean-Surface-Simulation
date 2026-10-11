#ifndef CALCULUS_HPP
#define CALCULUS_HPP

    #include <string>
    #include <cmath>
    #include <vector>
    #include "utils/config.hpp"

    struct Vertex {
        float x, y, z;
        float r, g, b;
    };

    Vertex gerstnerWaveHeight(const Vertex& vertex, float time, const GerstnerConfig& g);

    //float perlinWaveHeight(float x, float y, float time);
    //float fftWaveHeight(float x, float y, float time);

#endif // CALCULUS_HPP
