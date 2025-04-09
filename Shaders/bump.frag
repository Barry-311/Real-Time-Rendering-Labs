#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform sampler2D texture_diffuse;
uniform sampler2D texture_height;  // Bump Map

uniform float heightScale;

void main()
{
    // Get height texture, [0,1]
    float height1 = texture(texture_height, TexCoords + vec2(0.001, 0.0)).r;
    float height2 = texture(texture_height, TexCoords + vec2(0.0, 0.001)).r;
    float height0 = texture(texture_height, TexCoords).r;

    // Calculate gradient of U & V
    float dU = (height1 - height0) * heightScale;
    float dV = (height2 - height0) * heightScale;

    // Calculate normal
    vec3 perturbedNormal = normalize(vec3(-dU, -dV, 1.0));

    vec3 worldNormal = normalize(TBN * perturbedNormal);

    // illumination
    vec3 ambient = 0.5 * lightColor;
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(worldNormal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, worldNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * lightColor;

    vec3 textureColor = texture(texture_diffuse, TexCoords).rgb;
    vec3 result = (ambient + diffuse + specular) * textureColor;
    FragColor = vec4(result, 1.0);
}
