#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightPos;    // light position
uniform vec3 viewPos;     // camera position
uniform vec3 lightColor;  // light color
uniform sampler2D texture_diffuse; // diffusion texture
uniform int modelID;      // Model identifier to apply different shading

void main()
{
    vec3 ambient, diffuse, specular;
    float ambientStrength, specularStrength, shininess; // shininess->reflection strength

    if (modelID == 3)
    {
        // Toon shading for modelID=3
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);

        // Ambient light
        float toonAmbientStrength = 0.3;
        vec3 toonAmbient = toonAmbientStrength * lightColor;

        // Diffuse light with discretization
        float diff = max(dot(norm, lightDir), 0.0);
        float toonLevels = 4.0; // Number of brightness levels
        diff = floor(diff * toonLevels) / toonLevels; // Discretize diffuse component
        vec3 toonDiffuse = diff * lightColor;

        // Specular light with discretization
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); // Shininess fixed for Toon
        spec = floor(spec * toonLevels) / toonLevels;             // Discretize specular component
        vec3 toonSpecular = 0.5 * spec * lightColor;              // Toon specular strength

        // Combine Toon lighting components
        vec3 textureColor = texture(texture_diffuse, TexCoords).rgb;
        vec3 toonResult = (toonAmbient + toonDiffuse + toonSpecular) * textureColor;

        FragColor = vec4(toonResult, 1.0); // Output Toon result
        return;
    }

    // Model 4: Oren-Nayar diffuse lighting (for rough surfaces)
    else if (modelID == 4)
    {
        float roughness = 0.5; // Roughness parameter, controls surface scattering
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        vec3 viewDir = normalize(viewPos - FragPos);

        float NdotL = max(dot(norm, lightDir), 0.0); // cos(Angle of normal & light)
        float NdotV = max(dot(norm, viewDir), 0.0); // cos(Angle of normal & view direction)
        float theta = acos(NdotL); // Angle of light and normal
        float alpha = max(NdotL, NdotV); //main light
        float beta = min(NdotL, NdotV); //diffusion light

        float A = 1.0 - 0.5 * (roughness * roughness) / (roughness * roughness + 0.33); // basic light
        float B = 0.45 * (roughness * roughness) / (roughness * roughness + 0.09); // for rough items

        /// strength diffusion
        float diffuseFactor = A + B * max(0.0, cos(theta - acos(NdotV))) * sin(alpha) * tan(beta);
        diffuse = lightColor * NdotL * diffuseFactor;

        ambientStrength = 0.2; // Low ambient light for rough materials
        ambient = ambientStrength * lightColor;

        specular = vec3(0.0); // No specular for rough Oren-Nayar materials

        vec3 textureColor = texture(texture_diffuse, TexCoords).rgb;
        vec3 result = (ambient + diffuse) * textureColor;

        FragColor = vec4(result, 1.0);
        return;
    }

    // Model 5: Cook-Torrance shading (for metals)
    else if (modelID == 5)
    {
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 halfDir = normalize(lightDir + viewDir);

        // Constants
        float PI = 3.14159265;
        float Kr = pow((1.0 - 0.05) / (1.0 + 0.05), 2.0); // Reflectivity value (F0 = 0.05 for metals)

        // Dot products
        float normalDotHalfAngle = max(dot(norm, halfDir), 0.0);
        float normalDotHalfAngleSquared = normalDotHalfAngle * normalDotHalfAngle;
        float normalDotViewDir = max(dot(norm, viewDir), 0.0);
        float viewDirDotHalfAngle = max(dot(viewDir, halfDir), 0.0);
        float normalDotLightDir = max(dot(norm, lightDir), 0.0);

        // 1.Distribution term (GGX: Ground Glass and Microfacet Distribution)
        // for Physically Based Rendering (PBR)
        float roughness = 0.3; // Roughness parameter
        float alpha = roughness * roughness;
        float alpha2 = alpha * alpha;
        float NdotH2 = normalDotHalfAngle * normalDotHalfAngle;
        float D = alpha2 / (PI * pow((NdotH2 * (alpha2 - 1.0) + 1.0), 2.0));

        // 2.Geometric term (Schlick-GGX)
        // some microfacets are blocked by others, reducing the light reflection
        float g1 = (normalDotHalfAngleSquared * normalDotViewDir) / viewDirDotHalfAngle;
        float g2 = (normalDotHalfAngleSquared * normalDotLightDir) / viewDirDotHalfAngle;
        float G = min(1.0, min(g1, g2));

        // 3.Fresnel term (Schlick approximation)
        float F = Kr + (1.0 - Kr) * pow((1.0 - normalDotLightDir), 5.0); // For different viewpoint

        // Cook-Torrance specular component
        vec3 specular = (((D * F * G) / (PI * normalDotViewDir)) * lightColor);

        // Lambertian diffuse component
        vec3 lambertian = (1.0 - roughness) * (lightColor * normalDotLightDir);

        // Ambient component
        vec3 ambient = 0.3 * lightColor;

        // Combine all components
        vec3 textureColor = texture(texture_diffuse, TexCoords).rgb;
        vec3 result = (ambient + lambertian + specular) * textureColor;

        FragColor = vec4(result, 1.0);
        return;
    }

    // Model 6: Minnaert shading (darkened edges for cloth or powder materials)
    else if (modelID == 6)
    {
        float minnaertExponent = 0.5; // Controls the edge darkening effect
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);

        float NdotL = max(dot(norm, lightDir), 0.0);
        vec3 viewDir = normalize(viewPos - FragPos);
        float NdotV = max(dot(norm, viewDir), 0.0);

        float minnaertFactor = pow(NdotL * NdotV, minnaertExponent);
        diffuse = lightColor * minnaertFactor;

        ambientStrength = 0.3; // Slightly higher ambient light
        ambient = ambientStrength * lightColor;

        specular = vec3(0.0); // No specular for Minnaert materials

        vec3 textureColor = texture(texture_diffuse, TexCoords).rgb;
        vec3 result = (ambient + diffuse) * textureColor;

        FragColor = vec4(result, 1.0);
        return;
    }

    // Standard shading for other models
    if (modelID == 1)
    {
        ambientStrength = 0.2;
        specularStrength = 0.8;
        shininess = 64.0;
    }
    /*else if (modelID == 2)
    {
        ambientStrength = 0.8;
        specularStrength = 0.2;
        shininess = 16.0;
    }*/
    // For Lab3
    else if (modelID == 2)
    {
        ambientStrength = 0.5;
        specularStrength = 0.5;
        shininess = 32.0;
    }
    else
    {
        // Default model
        ambientStrength = 0.2;
        specularStrength = 0.8;
        shininess = 32.0;
    }

    // Ambient light
    ambient = ambientStrength * lightColor;

    // Diffusion
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    diffuse = diff * lightColor;

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    specular = specularStrength * spec * lightColor;

    // Texture
    vec3 textureColor = texture(texture_diffuse, TexCoords).rgb;

    // Combine all colors
    vec3 result = (ambient + diffuse + specular) * textureColor;
    FragColor = vec4(result, 1.0);
}
