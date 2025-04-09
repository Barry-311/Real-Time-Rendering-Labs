#include "grid.h"
#include <iostream>

ProjectedGrid::ProjectedGrid(unsigned int N)
    : N(N), VAO(0), VBO(0), EBO(0)
{
    GenerateGrid();
}

ProjectedGrid::~ProjectedGrid()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void ProjectedGrid::GenerateGrid()
{
    float step = 2.0f / (N - 1);
    vertices.clear();
    indices.clear();
    vertices.reserve(N * N * 2);

    for (unsigned int y = 0; y < N; y++) {
        for (unsigned int x = 0; x < N; x++) {
            float px = -1.0f + x * step; // [-1..1]
            float py = -1.0f + y * step; // [-1..1]
            vertices.push_back(px);
            vertices.push_back(py);
        }
    }

    indices.reserve((N - 1) * (N - 1) * 6);
    for (unsigned int yy = 0; yy < N - 1; yy++) {
        for (unsigned int xx = 0; xx < N - 1; xx++) {
            unsigned int topLeft = yy * N + xx;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (yy + 1) * N + xx;
            unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void ProjectedGrid::Draw() const
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
