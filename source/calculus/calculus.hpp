#ifndef CALCULUS_HPP
#define CALCULUS_HPP

    #include <string>
    #include <cmath>
    #include <vector>
    #include "utils/config.hpp"
    #include <complex>

    using Complex = std::complex<double>;
    const double PI = 3.14159265358979323846;
    const double g = 9.81;

    struct Vertex {
        float x, y, z;
        float r, g, b;
    };

    // gerstner equation
    Vertex gerstnerWaveHeight(const Vertex& vertex, float time, const GerstnerConfig& g);

    // fft equations
    void initFFTSpectrum(FFTConfig& f);
    void fftTimeEvo(int n, int m, float time, FFTConfig& f);
    void ifft2d(std::vector<std::vector<Complex>>& grid);

#endif // CALCULUS_HPP
