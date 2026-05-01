#include "ShaderGraph.hpp"
#include "imgui.h"
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

ShaderGraph::ShaderGraph() {
  // TODO: Initialize imnodes context when available
}

ShaderGraph::~ShaderGraph() {
  // TODO: Cleanup imnodes context when available
}

void ShaderGraph::render(float offsetX, float offsetY, float width, float height) {
  // Placeholder UI for shader graph (renders within current window)
  ImGui::Text("Shader Graph (imnodes integration in progress)");
  ImGui::Separator();

  ImGui::Text("Nodes: %zu", nodes.size());
  ImGui::Text("Connections: %zu", connections.size());

  if (ImGui::CollapsingHeader("Loaded Shaders")) {
    for (const auto& node : nodes) {
      ImGui::BulletText("%s", node.name.c_str());
      ImGui::Indent();
      ImGui::Text("Inputs: %zu", node.inputs.size());
      ImGui::Text("Outputs: %zu", node.outputs.size());
      ImGui::Unindent();
    }
  }
}

void ShaderGraph::addShaderNode(const std::string& name, const std::string& filePath) {
  ShaderNode node;
  node.id = nextNodeId++;
  node.name = name;
  node.path = filePath;

  extractShaderInterface(filePath, node);
  nodes.push_back(node);
}

void ShaderGraph::removeShaderNode(int nodeId) {
  nodes.erase(
      std::remove_if(nodes.begin(), nodes.end(),
                     [nodeId](const ShaderNode& n) { return n.id == nodeId; }),
      nodes.end());
}

void ShaderGraph::addConnection(int fromNodeId, int fromOutput, int toNodeId, int toInput) {
  connections.push_back({fromNodeId, fromOutput, toNodeId, toInput});
}

void ShaderGraph::removeConnection(int connectionId) {
  if (connectionId >= 0 && connectionId < static_cast<int>(connections.size())) {
    connections.erase(connections.begin() + connectionId);
  }
}

void ShaderGraph::extractShaderInterface(const std::string& filePath, ShaderNode& node) {
  // TODO: Use ABox's SPIR-V reflection to extract shader interface
  // For now, add placeholder inputs/outputs
  node.inputs.push_back({"input_data", "vec4", 0});
  node.outputs.push_back({"output_color", "vec4", 0});
}
