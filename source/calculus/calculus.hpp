#ifndef CALCULUS_HPP
#define CALCULUS_HPP

#include <string>

    using HeightFunction = float (*)(float x, float y, float time);

    HeightFunction findWaveMethod(const std::string& name);

    float gerstnerWaveHeight(float x, float y, float time); // gerstner.cpp
    float perlinWaveHeight(float x, float y, float time);   // perlin.cpp
    float fftWaveHeight(float x, float y, float time);      // fft.cpp

#endif // CALCULUS_HPP
