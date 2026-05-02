#pragma once

#include <string>
#include <vector>
#include <spirv_reflect.h>
#include <imnodes.h>

struct ShaderNode {
  int id;
  std::string name;
  std::string path;
  bool needsPositioning = true;

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
  int id;
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
  int nextLinkId = 0;

  ImNodesContext* imnodesCtx = nullptr;

  void extractShaderInterface(const std::string& filePath, ShaderNode& node);

  // Attribute ID encoding: nodeId * 1000 + pin index
  // Inputs:  [nodeId * 1000 .. nodeId * 1000 + 499]
  // Outputs: [nodeId * 1000 + 500 .. nodeId * 1000 + 999]
  static int inputAttrId(int nodeId, int pinIndex)  { return nodeId * 1000 + pinIndex; }
  static int outputAttrId(int nodeId, int pinIndex) { return nodeId * 1000 + 500 + pinIndex; }
};
