#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal1; // Normal Map

void main()
{
    // Sample normal texture 
    vec3 normalTex = texture(texture_normal1, TexCoords).rgb;

    if (normalTex == vec3(0.0)) {
        normalTex = vec3(0.5, 0.5, 1.0); // (0.5,0.5,1) = (0,0,1) in [-1,1] space
    }

    // Normal [0,1] -> [-1,1]
    normalTex = normalize(normalTex * 2.0 - 1.0);

    vec3 perturbedNormal = normalize(TBN * normalTex);

    // illumination
    vec3 ambient = 0.5 * lightColor;
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(perturbedNormal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, perturbedNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * lightColor;

    vec3 textureColor = texture(texture_diffuse, TexCoords).rgb;
    vec3 result = (ambient + diffuse + specular) * textureColor;
    FragColor = vec4(result, 1.0);
}
