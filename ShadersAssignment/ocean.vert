#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float uTime;
uniform float waveSpeed;
uniform float waveScale;
uniform sampler2D heightMap;

out vec2 TexCoord;

void main() {
    vec2 worldXZ = inPos.xz;
    vec2 uv = worldXZ * 0.01 * waveScale;
    uv += uTime * waveSpeed;
    float heightVal = texture(heightMap, uv).r;
    float wave = (heightVal - 0.5) * 2.0;

    vec3 displacedPos = inPos;
    displacedPos.y += wave;

    gl_Position = projection * view * model * vec4(displacedPos, 1.0);
    TexCoord = inTexCoord;
}
