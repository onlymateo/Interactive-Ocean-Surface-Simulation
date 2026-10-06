#ifndef ENGINES_ENGINE_HPP
#define ENGINES_ENGINE_HPP

    #include <string>
    #include "calculus/calculus.hpp"

    struct Engine {
        const char* name;
        int (*run)(int width, int height, HeightFunction wave);
    };

    const Engine* findEngine(const std::string& name);

#endif // ENGINES_ENGINE_HPP
