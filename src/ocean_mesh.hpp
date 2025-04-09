#ifndef OCEAN_MESH_HPP
#define OCEAN_MESH_HPP

#include <vector>
#include <GL/glew.h>


struct OceanVertex {
    float x, y, z;
    float u, v;
};

class OceanMesh {
public:
    unsigned int VAO, VBO, EBO;
    unsigned int indexCount;

    OceanMesh() : VAO(0), VBO(0), EBO(0), indexCount(0) {}

    void setup(int N = 128, float size = 100.0f) {
        std::vector<OceanVertex> vertices;
        std::vector<unsigned int> indices;
        vertices.reserve(N * N);

        for (int z = 0; z < N; z++) {
            for (int x = 0; x < N; x++) {
                float u = x / (float)(N - 1);
                float v = z / (float)(N - 1);

                float X = u * size - size * 0.5f;
                float Z = v * size - size * 0.5f;
                vertices.push_back({ X, 0.0f, Z,  u, v });
            }
        }

        for (int row = 0; row < N - 1; row++) {
            for (int col = 0; col < N - 1; col++) {
                int i0 = row * N + col;
                int i1 = i0 + 1;
                int i2 = i0 + N;
                int i3 = i2 + 1;

                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i1);

                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }
        indexCount = (unsigned int)indices.size();

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(OceanVertex),
            &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
            &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OceanVertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(OceanVertex), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);
    }

    void draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

#endif // OCEAN_MESH_HPP
