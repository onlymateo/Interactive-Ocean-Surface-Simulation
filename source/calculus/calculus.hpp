#ifndef CALCULUS_HPP
#define CALCULUS_HPP

    #include <string>
    #include <cmath>
    #include <vector>
    #include "utils/config.hpp"
    #include <complex>

    using Complex = std::complex<double>;
    const double PI = 3.14159265358979323846;

    struct Vertex {
        float x, y, z;
        float r, g, b;
    };

    Vertex gerstnerWaveHeight(const Vertex& vertex, float time, const GerstnerConfig& g);

    Vertex fftWaveHeight(const Vertex& vertex, float time, const FFTConfig& f);
    //float perlinWaveHeight(float x, float y, float time);

#endif // CALCULUS_HPP
