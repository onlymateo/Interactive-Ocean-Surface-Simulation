#include "engines/chunk.hpp"

#include <cmath>


int gridSide(const Config& config) {
    int side = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(config.chunksNumber))));
    return side < 1 ? 1 : side;
}

std::vector<Plane> generateChunks(const Config& config) {
    const int side = gridSide(config);
    std::vector<Plane> chunks;
    for (int i = 0; i < side * side; i++) {
        chunks.push_back(Plane(config));
    }
    return chunks;
}

bool updateChunks(std::vector<Plane>& chunks, const Config& config, float cameraX, float cameraY, float time) {
    const int side = gridSide(config);
    const float size = config.planeSize;

    const int cameraCellX = static_cast<int>(std::floor(cameraX / size + 0.5f));
    const int cameraCellY = static_cast<int>(std::floor(cameraY / size + 0.5f));
    const int firstX = cameraCellX - side / 2;
    const int firstY = cameraCellY - side / 2;

    bool moved = false;
    for (int i = 0; i < side; i++) {
        for (int j = 0; j < side; j++) {
            int cellX = firstX + ((i - firstX) % side + side) % side;
            int cellY = firstY + ((j - firstY) % side + side) % side;

            Plane& chunk = chunks[i * side + j];
            if (chunk.centerX() != cellX * size || chunk.centerY() != cellY * size) {
                chunk.moveTo(cellX * size, cellY * size);
                chunk.update(time); // new place, so new heights
                moved = true;
            }
        }
    }
    return moved;
}
