#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct Vertex {
  float position[3];
  float normal[3];
  float uv[2];
};

enum class PrimitiveType {
  Quad,
  Cube,
  Sphere,
};

class Mesh {
public:
  std::string name;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  // Generate built-in primitives
  static Mesh createQuad();
  static Mesh createCube();
  static Mesh createSphere(int segments = 32, int rings = 16);
  static Mesh createPrimitive(PrimitiveType type);
};
