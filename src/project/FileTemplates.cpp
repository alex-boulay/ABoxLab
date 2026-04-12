#include "FileTemplates.hpp"

std::string FileTemplates::getTemplateForShaderType(const std::string& shaderType) {
  // GLSL shaders (no platform extension - ABox defaults to GLSL)
  if (shaderType == "glsl_vertex") {
    return R"(#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
}
)";
  } else if (shaderType == "glsl_fragment") {
    return R"(#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
)";
  } else if (shaderType == "glsl_compute") {
    return R"(#version 450

layout (local_size_x = 16, local_size_y = 16) in;

void main() {
    // Compute shader code here
}
)";
  }
  // HLSL shaders (use .hlsl.ext - ABox double-extension convention)
  else if (shaderType == "hlsl_vertex") {
    return R"(struct VSInput {
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}
)";
  } else if (shaderType == "hlsl_fragment") {
    return R"(struct PSInput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET {
    return float4(input.color, 1.0);
}
)";
  } else if (shaderType == "hlsl_compute") {
    return R"([numthreads(16, 16, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
    // Compute shader code here
}
)";
  }
  return "";
}

std::string FileTemplates::getExtensionForShaderType(const std::string& shaderType) {
  // GLSL - no platform extension (ABox default)
  if (shaderType == "glsl_vertex") return ".vert";
  if (shaderType == "glsl_fragment") return ".frag";
  if (shaderType == "glsl_compute") return ".comp";

  // HLSL - double extension (.hlsl.stage)
  if (shaderType == "hlsl_vertex") return ".hlsl.vert";
  if (shaderType == "hlsl_fragment") return ".hlsl.frag";
  if (shaderType == "hlsl_compute") return ".hlsl.comp";

  return "";
}
