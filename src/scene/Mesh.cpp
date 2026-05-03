#include "Mesh.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Mesh Mesh::createQuad() {
  Mesh mesh;
  mesh.name = "Quad";

  mesh.vertices = {
    // pos                    normal            uv
    {{ -1.0f, -1.0f, 0.0f }, { 0, 0, 1 }, { 0, 0 }},
    {{  1.0f, -1.0f, 0.0f }, { 0, 0, 1 }, { 1, 0 }},
    {{  1.0f,  1.0f, 0.0f }, { 0, 0, 1 }, { 1, 1 }},
    {{ -1.0f,  1.0f, 0.0f }, { 0, 0, 1 }, { 0, 1 }},
  };

  mesh.indices = { 0, 1, 2, 2, 3, 0 };
  return mesh;
}

Mesh Mesh::createCube() {
  Mesh mesh;
  mesh.name = "Cube";

  // 6 faces, 4 vertices each (separate normals per face)
  mesh.vertices = {
    // Front (+Z)
    {{ -1, -1,  1 }, {  0,  0,  1 }, { 0, 0 }},
    {{  1, -1,  1 }, {  0,  0,  1 }, { 1, 0 }},
    {{  1,  1,  1 }, {  0,  0,  1 }, { 1, 1 }},
    {{ -1,  1,  1 }, {  0,  0,  1 }, { 0, 1 }},
    // Back (-Z)
    {{  1, -1, -1 }, {  0,  0, -1 }, { 0, 0 }},
    {{ -1, -1, -1 }, {  0,  0, -1 }, { 1, 0 }},
    {{ -1,  1, -1 }, {  0,  0, -1 }, { 1, 1 }},
    {{  1,  1, -1 }, {  0,  0, -1 }, { 0, 1 }},
    // Right (+X)
    {{  1, -1,  1 }, {  1,  0,  0 }, { 0, 0 }},
    {{  1, -1, -1 }, {  1,  0,  0 }, { 1, 0 }},
    {{  1,  1, -1 }, {  1,  0,  0 }, { 1, 1 }},
    {{  1,  1,  1 }, {  1,  0,  0 }, { 0, 1 }},
    // Left (-X)
    {{ -1, -1, -1 }, { -1,  0,  0 }, { 0, 0 }},
    {{ -1, -1,  1 }, { -1,  0,  0 }, { 1, 0 }},
    {{ -1,  1,  1 }, { -1,  0,  0 }, { 1, 1 }},
    {{ -1,  1, -1 }, { -1,  0,  0 }, { 0, 1 }},
    // Top (+Y)
    {{ -1,  1,  1 }, {  0,  1,  0 }, { 0, 0 }},
    {{  1,  1,  1 }, {  0,  1,  0 }, { 1, 0 }},
    {{  1,  1, -1 }, {  0,  1,  0 }, { 1, 1 }},
    {{ -1,  1, -1 }, {  0,  1,  0 }, { 0, 1 }},
    // Bottom (-Y)
    {{ -1, -1, -1 }, {  0, -1,  0 }, { 0, 0 }},
    {{  1, -1, -1 }, {  0, -1,  0 }, { 1, 0 }},
    {{  1, -1,  1 }, {  0, -1,  0 }, { 1, 1 }},
    {{ -1, -1,  1 }, {  0, -1,  0 }, { 0, 1 }},
  };

  mesh.indices = {
     0,  1,  2,  2,  3,  0,  // front
     4,  5,  6,  6,  7,  4,  // back
     8,  9, 10, 10, 11,  8,  // right
    12, 13, 14, 14, 15, 12,  // left
    16, 17, 18, 18, 19, 16,  // top
    20, 21, 22, 22, 23, 20,  // bottom
  };

  return mesh;
}

Mesh Mesh::createSphere(int segments, int rings) {
  Mesh mesh;
  mesh.name = "Sphere";

  for (int r = 0; r <= rings; ++r) {
    float phi = static_cast<float>(M_PI) * r / rings;
    float y = std::cos(phi);
    float sinPhi = std::sin(phi);

    for (int s = 0; s <= segments; ++s) {
      float theta = 2.0f * static_cast<float>(M_PI) * s / segments;
      float x = sinPhi * std::cos(theta);
      float z = sinPhi * std::sin(theta);

      Vertex v;
      v.position[0] = x;
      v.position[1] = y;
      v.position[2] = z;
      v.normal[0] = x;
      v.normal[1] = y;
      v.normal[2] = z;
      v.uv[0] = static_cast<float>(s) / segments;
      v.uv[1] = static_cast<float>(r) / rings;

      mesh.vertices.push_back(v);
    }
  }

  for (int r = 0; r < rings; ++r) {
    for (int s = 0; s < segments; ++s) {
      uint32_t a = r * (segments + 1) + s;
      uint32_t b = a + segments + 1;

      mesh.indices.push_back(a);
      mesh.indices.push_back(b);
      mesh.indices.push_back(a + 1);

      mesh.indices.push_back(a + 1);
      mesh.indices.push_back(b);
      mesh.indices.push_back(b + 1);
    }
  }

  return mesh;
}

Mesh Mesh::createPrimitive(PrimitiveType type) {
  switch (type) {
    case PrimitiveType::Quad:   return createQuad();
    case PrimitiveType::Cube:   return createCube();
    case PrimitiveType::Sphere: return createSphere();
  }
  return createCube();
}
