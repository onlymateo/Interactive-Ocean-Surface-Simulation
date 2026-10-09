#include <iostream>
#include "calculus/calculus.hpp"
#include "engines/directx.hpp"
#include "utils/config.hpp"

int main() {
    Config config = loadConfig("./config.json");

    return runDirectX(config);
}
