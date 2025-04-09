#pragma once
#include <vector>
#include <GL/glew.h>

class ProjectedGrid
{
public:
    ProjectedGrid(unsigned int N);
    ~ProjectedGrid();

    void Draw() const;

private:
    void GenerateGrid();

    unsigned int N;
    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};
