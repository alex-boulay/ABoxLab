#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple directional lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diffuse = max(dot(normalize(fragNormal), lightDir), 0.0);
    vec3 baseColor = vec3(0.3, 0.6, 0.9);
    vec3 color = baseColor * (0.3 + 0.7 * diffuse);
    outColor = vec4(color, 1.0);
}
