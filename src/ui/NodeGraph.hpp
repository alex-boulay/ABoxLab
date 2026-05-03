#pragma once

#include <string>
#include <vector>
#include <spirv_reflect.h>
#include <imnodes.h>

// Pin types for type-safe connections
enum class PinType {
  Geometry,   // Mesh data
  Shader,     // Compiled shader stage
  Texture,    // Texture resource
  Float,      // Scalar value
  Vec2,
  Vec3,
  Vec4,
};

struct Pin {
  std::string name;
  PinType type;
};

enum class NodeType {
  Shader,
  Mesh,
  Texture,
  Output,    // Material output — the final bundle
  Float,
  Vec3,
  Color,
};

struct GraphNode {
  int id;
  NodeType type;
  std::string name;
  bool needsPositioning = true;

  std::vector<Pin> inputs;
  std::vector<Pin> outputs;

  // Type-specific data
  std::string filePath;       // Shader/Texture path
  int primitiveIndex = 0;     // Mesh: which primitive (Quad=0, Cube=1, Sphere=2)
  float valueFloat = 0.0f;    // Float node value
  float valueVec3[3] = {0,0,0}; // Vec3/Color node value
};

struct Connection {
  int id;
  int fromNodeId;
  int fromOutputIndex;
  int toNodeId;
  int toInputIndex;
};

// Result of evaluating the Output node's connections
struct GraphBindings {
  bool valid = false;
  int meshPrimitive = -1;         // -1 = no mesh bound
  std::string vertShaderSpv;      // empty = not bound
  std::string fragShaderSpv;      // empty = not bound
  std::string texturePath;        // empty = not bound

  bool operator==(const GraphBindings& o) const {
    return meshPrimitive == o.meshPrimitive &&
           vertShaderSpv == o.vertShaderSpv &&
           fragShaderSpv == o.fragShaderSpv &&
           texturePath == o.texturePath;
  }
  bool operator!=(const GraphBindings& o) const { return !(*this == o); }
};

class NodeGraph {
public:
  NodeGraph();
  ~NodeGraph();

  void render(float offsetX, float offsetY, float width, float height);

  // Evaluate the Output node and return what's connected
  GraphBindings evaluate() const;

  void addShaderNode(const std::string& name, const std::string& filePath);
  void addMeshNode();
  void addTextureNode(const std::string& name, const std::string& filePath);
  void addOutputNode();
  void addFloatNode(float value = 0.0f);
  void addVec3Node(float x = 0, float y = 0, float z = 0);
  void addColorNode(float r = 1, float g = 1, float b = 1);
  void removeNode(int nodeId);

  const std::vector<GraphNode>& getNodes() const { return nodes; }
  const std::vector<Connection>& getConnections() const { return connections; }

private:
  std::vector<GraphNode> nodes;
  std::vector<Connection> connections;
  int nextNodeId = 0;
  int nextLinkId = 0;

  ImNodesContext* imnodesCtx = nullptr;

  void renderNode(GraphNode& node);
  void renderContextMenu();
  void extractShaderInterface(const std::string& filePath, GraphNode& node);
  bool canConnect(PinType from, PinType to) const;

  static int inputAttrId(int nodeId, int pinIndex)  { return nodeId * 1000 + pinIndex; }
  static int outputAttrId(int nodeId, int pinIndex) { return nodeId * 1000 + 500 + pinIndex; }

  static ImU32 pinColor(PinType type);
  static ImNodesPinShape pinShape(PinType type);
};
