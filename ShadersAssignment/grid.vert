#version 330 core

layout(location = 0) in vec2 inClipPos; // [-1,1]
out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

uniform sampler2D uWaveTex;  
uniform sampler2D uTex; 
uniform sampler2D vTex;  
uniform sampler2D uJacobianTex;
uniform samplerCube uSkybox;  

uniform mat4 view;
uniform mat4 projection;
uniform mat4 inverseVP;
uniform vec3 cameraPos;

uniform float waveAmplify;
uniform float chopAmplify;

vec3 unprojectToYPlane(vec2 clipPos) {
    vec4 nearClip = vec4(clipPos, -1.0, 1.0);
    vec4 farClip = vec4(clipPos, 1.0, 1.0);

    vec4 nearWorld = inverseVP * nearClip;
    nearWorld /= nearWorld.w;
    vec4 farWorld = inverseVP * farClip;
    farWorld /= farWorld.w;

    float t = -nearWorld.y / (farWorld.y - nearWorld.y);
    return mix(nearWorld.xyz, farWorld.xyz, t);
}

//void main()
//{
//    vec3 oceanPos = unprojectToYPlane(inClipPos);
//
//    // vec3 debugColor = vec3((oceanPos.x + 100.0) / 200.0, (oceanPos.z + 100.0) / 200.0, 0.0);
//    // gl_Position = vec4(debugColor, 1.0);
//    // return;
//
//    float domain = 200.0; 
//    vec2 localUV = (oceanPos.xz + vec2(domain * 0.5)) / domain;
//
//    vec2 choppy = vec2(texture(uTex, localUV).r, texture(vTex, localUV).r);

//    float h = texture(uWaveTex, localUV).r * 5.0;
//
//    vec3 finalPos = vec3(oceanPos.x + choppy.x, h, oceanPos.z + choppy.y);
//
//    vWorldPos = finalPos;
//    vNormal = vec3(0.0, 1.0, 0.0);
//    vUV = localUV;
//
//    //gl_Position = projection * view * vec4(finalPos, 1.0);
//    gl_Position = projection * view * vec4(oceanPos.x, h * 50.0, oceanPos.z, 1.0);
//}

void main()
{
    vec3 oceanPos = unprojectToYPlane(inClipPos);

    float domain = 200.0;
    vec2 localUV = (oceanPos.xz + vec2(domain * 0.5)) / domain;

    //float waveAmplify = 100.0;
    //float chopAmplify = 5.0;
    vec2 choppy = vec2(texture(uTex, localUV).r, texture(vTex, localUV).r) * chopAmplify;
    float h = texture(uWaveTex, localUV).r * waveAmplify;

    vec3 finalPos = vec3(oceanPos.x + choppy.x, h, oceanPos.z + choppy.y);

    vWorldPos = finalPos;
    vNormal = vec3(0.0, 1.0, 0.0);
    vUV = localUV;

    gl_Position = projection * view * vec4(finalPos, 1.0);
}