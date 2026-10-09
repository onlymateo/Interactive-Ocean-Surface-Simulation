#ifndef RENDER_PLANE_HPP
#define RENDER_PLANE_HPP

    #include <vector>
    #include "calculus/calculus.hpp"
    #include "utils/config.hpp"

    class Plane {
        public:
            Plane(Config config);

            void update(float time);

            struct Vertex {
                float x, y, z;
                float r, g, b;
            };

            const std::vector<Vertex>& vertices() const { return vertices_; }
            const std::vector<unsigned int>& indices() const { return indices_; }

        private:
            void buildMesh();

            Config config;
            std::vector<Vertex> vertices_;
            std::vector<unsigned int> indices_;
    };

#endif // RENDER_PLANE_HPP
