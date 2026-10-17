#ifndef RENDER_PLANE_HPP
#define RENDER_PLANE_HPP

    #include <vector>
    #include "calculus/calculus.hpp"
    #include "utils/config.hpp"

    class Plane {
        public:
            Plane(Config config);

            void update(float time);

            // Moves the whole plane so that its centre is at (centerX, centerY) in the world.
            void moveTo(float centerX, float centerY);
            float centerX() const { return centerX_; }
            float centerY() const { return centerY_; }

            using Vertex = ::Vertex;

            const std::vector<Vertex>& vertices() const { return vertices_; }
            const std::vector<unsigned int>& indices() const { return indices_; }
            

        private:
            void buildMesh();

            Config config;
            float centerX_ = 0.0f;
            float centerY_ = 0.0f;
            std::vector<Vertex> base_;     // the vertices at rest
            std::vector<Vertex> vertices_; // the vertices moved by the waves, sent to the GPU
            std::vector<unsigned int> indices_;
    };

#endif // RENDER_PLANE_HPP
