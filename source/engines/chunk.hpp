#ifndef CHUNK_HPP
#define CHUNK_HPP

    #include <vector>
    #include "render/plane.hpp"

    int gridSide(const Config& config);
    std::vector<Plane> generateChunks(const Config& config);
    bool updateChunks(std::vector<Plane>& chunks, const Config& config, float cameraX, float cameraY, float time);

#endif // CHUNK_HPP
