#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <spirv_reflect.h>

struct ShaderNode {
  int id;
  std::string name;
  std::string path;

  // Input/output information from SPIR-V reflection
  struct Variable {
    std::string name;
    std::string type;
    int location;
  };

  std::vector<Variable> inputs;
  std::vector<Variable> outputs;
};

struct ShaderConnection {
  int fromNodeId;
  int fromOutputIndex;
  int toNodeId;
  int toInputIndex;
};

class ShaderGraph {
public:
  ShaderGraph();
  ~ShaderGraph();

  void render(float offsetX, float offsetY, float width, float height);

  void addShaderNode(const std::string& name, const std::string& filePath);
  void removeShaderNode(int nodeId);
  void addConnection(int fromNodeId, int fromOutput, int toNodeId, int toInput);
  void removeConnection(int connectionId);

  const std::vector<ShaderNode>& getNodes() const { return nodes; }
  const std::vector<ShaderConnection>& getConnections() const { return connections; }

private:
  std::vector<ShaderNode> nodes;
  std::vector<ShaderConnection> connections;
  int nextNodeId = 0;

  void extractShaderInterface(const std::string& filePath, ShaderNode& node);
  void renderNode(const ShaderNode& node);
};
