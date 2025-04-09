#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

uniform samplerCube environmentMap;

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    vec3 R = reflect(-V, N);

    // --- Refraction and Dispersion ---
    // For transparent materials like glass, the refractive index is typically around 1.5.
    // Here, we use the air-to-glass ratio.
    // To simulate dispersion, we use slightly different refractive indices for different wavelengths.
    float rf = 1.5, gf = 1.5, bf = 1.5;
    float etaR = 1.0 / rf;  // Red channel
    float etaG = 1.0 / gf;  // Green channel
    float etaB = 1.0 / bf;  // Blue channel

    // Compute refraction vectors for each color channel
    vec3 refractR = refract(-V, N, etaR);
    vec3 refractG = refract(-V, N, etaG);
    vec3 refractB = refract(-V, N, etaB);

    // --- Sample the Environment Map ---
    // Reflection: Directly sample the cubemap using the reflection vector
    vec3 reflectColor = texture(environmentMap, R).rgb;

    // Refraction: Sample the cubemap using refraction vectors for R, G, and B channels to simulate dispersion
    vec3 refractColorR = texture(environmentMap, refractR).rgb;
    vec3 refractColorG = texture(environmentMap, refractG).rgb;
    vec3 refractColorB = texture(environmentMap, refractB).rgb;
    vec3 refractColor = vec3(refractColorR.r, refractColorG.g, refractColorB.b);

    // Fresnel Calculation
    float cosTheta = clamp(dot(-V, N), 0.0, 1.0);
    float F0 = 0.03;
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

    // --- Blend Reflection and Refraction ---
    // The higher the fresnel effect, the more reflection is visible; otherwise, refraction is more dominant.
    vec3 envColor = mix(refractColor, reflectColor, 1);

    // --- Optional: Add Simple Ambient Lighting ---
    vec3 ambient = 0.5 * lightColor;

    // Final color (additional lighting, shadows, or other effects can be added as needed)
    vec3 color = envColor + ambient;

    FragColor = vec4(color, 1.0);
}