#ifndef RENDER_PLANE_HPP
#define RENDER_PLANE_HPP

    #include <vector>
    #include "calculus/calculus.hpp"

    class Plane {
    public:
        Plane(float size, int cells, HeightFunction heightAt);

        void update(float time);

        struct Vertex {
            float x, y, z;
            float r, g, b;
        };

        const std::vector<Vertex>& vertices() const { return vertices_; }
        const std::vector<unsigned int>& indices() const { return indices_; }

    private:
        void buildMesh();

        float size_;
        int cells_;
        HeightFunction heightAt_;
        std::vector<Vertex> vertices_;
        std::vector<unsigned int> indices_;
    };

#endif // RENDER_PLANE_HPP
