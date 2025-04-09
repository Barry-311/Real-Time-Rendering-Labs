#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 FragColor;

uniform sampler2D uWaveTex;
uniform sampler2D uUVTex;
uniform sampler2D uJacobianTex;
uniform samplerCube uSkybox;
uniform vec3 cameraPos;

uniform float foamStart;
uniform float foamEnd;

void main()
{
    // whitecap
    float j = texture(uJacobianTex, vUV).r;
    //float foamMask = smoothstep(1.001, 0.995, j);
    //float foamMask = smoothstep(1.02, 0.95, j);
    float foamMask = smoothstep(foamStart, foamEnd, j);

    vec3 baseColor = vec3(0.0, 0.1, 0.3);
    vec3 foamColor = mix(baseColor, vec3(1.0), foamMask);

    // reflection
    vec3 N = normalize(vNormal);
    vec3 V = normalize(cameraPos - vWorldPos);
    vec3 R = reflect(-V, N);
    vec3 env = texture(uSkybox, R).rgb;

    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0) * 0.8 + 0.02;
    vec3 final = mix(foamColor, env, fresnel);

    FragColor = vec4(final, 1.0);
    //FragColor = vec4(j, j, j, 1.0);
    //FragColor = vec4(1.0);
}
