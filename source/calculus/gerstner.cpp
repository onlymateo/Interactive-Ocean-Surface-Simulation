#include "calculus/calculus.hpp"


float gerstnerWaveHeight(float x, float y, float time) {
    int k = 1;
    int A = 1;
    std::vector<int> D = {1, 1};

    float teta = k * (D[0] * x + D[1] * y) - sqrt(9.81 * k) * time;
    return (A * sin(teta));
} 
