#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in mat3 TBN;

uniform sampler2D baseColorMap;
uniform sampler2D metalnessMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float mipLevel; 

void main()
{
    float distanceToCamera = length(viewPos - FragPos);

    vec2 dx = dFdx(TexCoords);
    vec2 dy = dFdy(TexCoords);
    float texelSize = max(length(dx), length(dy));

    float autoMipLevel = log2(distanceToCamera * texelSize * 10.0);

    float finalMipLevel = clamp(autoMipLevel + mipLevel, 0.0, 5.0);

    vec3 albedo = textureLod(baseColorMap, TexCoords, finalMipLevel).rgb;
    float metalness = textureLod(metalnessMap, TexCoords, finalMipLevel).r;
    float roughness = textureLod(roughnessMap, TexCoords, finalMipLevel).r;

    vec3 normalMap = textureLod(normalMap, TexCoords, finalMipLevel).rgb * 2.0 - 1.0;
    vec3 N = normalize(TBN * normalMap);

    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);

    float diff = max(dot(N, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(N, halfwayDir), 0.0), 16.0);

    vec3 ambient = 0.3 * albedo;
    vec3 diffuse = diff * albedo * lightColor;
    vec3 specular = spec * vec3(1.0) * (1.0 - roughness);

    vec3 finalColor = ambient + diffuse + specular;

    FragColor = vec4(finalColor, 1.0);
}
