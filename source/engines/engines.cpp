#include "engines/engine.hpp"

#include "engines/opengl/opengl.hpp"

namespace {

// Add new engines here, e.g. {"sfml", runSFML}.
const Engine ENGINES[] = {
    {"opengl", runOpenGL},
};

}

const Engine* findEngine(const std::string& name) {
    for (const Engine& engine : ENGINES) {
        if (name == engine.name) {
            return &engine;
        }
    }
    return nullptr;
}
