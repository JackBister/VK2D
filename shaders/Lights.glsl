
const uint MAX_LIGHTS = 512;
const float PI = 3.14159265359;

struct Light {
    uint isActive;
    mat4 localToWorld;

    vec3 color;
};

// TODO: This is probably not a great idea. This is needed for CalculateLight to compile, and I want CalculateLight to be in this file.
// If I add lights as a parameter to CalculateLight, it appears the array will be copied into the function on each call which completely wrecks the frame rate.
layout (std140, set = 0, binding = 0) uniform lightsData {
	Light lights[MAX_LIGHTS];
};

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 CalculateLight(vec3 WorldPos, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, float ao, vec3 F0)
{
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].isActive == 0) {
            continue;
        }
        vec3 lightPos = lights[i].localToWorld[3].xyz;
        vec3 L = normalize(lightPos - WorldPos);
        vec3 H = normalize(V + L);
        float dist = length(lightPos - WorldPos);
        float attenuation = 1.0 / (dist * dist + 1);
        vec3 radiance = lights[i].color * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular = numerator / max(denominator, 0.001);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    return Lo;
}
