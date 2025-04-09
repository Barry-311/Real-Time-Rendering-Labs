#version 330 core

layout(location = 0) in vec3 aPos;       // vertex position
layout(location = 1) in vec3 aNormal;    // vertex normal
layout(location = 2) in vec2 aTexCoords; // texture position (u,v)

out vec3 FragPos;    // fragment position
out vec3 Normal;     // normal vector (for reflection & diffusion)
out vec2 TexCoords;  // texture position

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() // local -> global
{
    FragPos = vec3(model * vec4(aPos, 1.0));                // aPos: 3D -> 4D
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * vec4(FragPos, 1.0);   // Project to screen
}
