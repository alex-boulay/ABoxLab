#include "ShaderGraph.hpp"
#include "imgui.h"
#include "ShaderCompiler.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>

ShaderGraph::ShaderGraph() {
  imnodesCtx = ImNodes::CreateContext();
}

ShaderGraph::~ShaderGraph() {
  if (imnodesCtx) {
    ImNodes::DestroyContext(imnodesCtx);
  }
}

void ShaderGraph::render(float offsetX, float offsetY, float width, float height) {
  ImNodes::SetCurrentContext(imnodesCtx);

  // Lazy init: configure style/IO on first render when ImGui context is guaranteed ready
  static bool initialized = false;
  if (!initialized) {
    ImNodes::StyleColorsDark();
    ImNodes::GetIO().LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
    initialized = true;
  }

  ImNodes::BeginNodeEditor();

  // Render nodes
  for (auto& node : nodes) {
    // Position new nodes so they don't stack
    if (node.needsPositioning) {
      float x = 50.0f + (node.id % 4) * 300.0f;
      float y = 50.0f + (node.id / 4) * 250.0f;
      ImNodes::SetNodeGridSpacePos(node.id, ImVec2(x, y));
      node.needsPositioning = false;
    }

    ImNodes::BeginNode(node.id);

    // Title bar
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(node.name.c_str());
    ImNodes::EndNodeTitleBar();

    // Input pins (left side)
    for (int i = 0; i < static_cast<int>(node.inputs.size()); ++i) {
      const auto& var = node.inputs[i];
      ImNodesPinShape shape = (var.type == "resource")
          ? ImNodesPinShape_QuadFilled
          : ImNodesPinShape_CircleFilled;
      ImNodes::BeginInputAttribute(inputAttrId(node.id, i), shape);
      ImGui::Text("%s (%s)", var.name.c_str(), var.type.c_str());
      ImNodes::EndInputAttribute();
    }

    // Output pins (right side)
    for (int i = 0; i < static_cast<int>(node.outputs.size()); ++i) {
      const auto& var = node.outputs[i];
      ImNodes::BeginOutputAttribute(outputAttrId(node.id, i));
      ImGui::Text("%s (%s)", var.name.c_str(), var.type.c_str());
      ImNodes::EndOutputAttribute();
    }

    ImNodes::EndNode();
  }

  // Render existing links
  for (const auto& conn : connections) {
    int startAttr = outputAttrId(conn.fromNodeId, conn.fromOutputIndex);
    int endAttr = inputAttrId(conn.toNodeId, conn.toInputIndex);
    ImNodes::Link(conn.id, startAttr, endAttr);
  }

  ImNodes::MiniMap(0.15f, ImNodesMiniMapLocation_BottomRight);
  ImNodes::EndNodeEditor();

  // Handle new link creation
  int startAttr, endAttr;
  if (ImNodes::IsLinkCreated(&startAttr, &endAttr)) {
    // Decode which node/pin the attributes belong to
    int fromNodeId = startAttr / 1000;
    int fromPin = startAttr % 1000;
    int toNodeId = endAttr / 1000;
    int toPin = endAttr % 1000;

    // Determine direction: output attrs are >= 500, input attrs are < 500
    // imnodes may swap start/end, so normalize
    if (fromPin < 500 && toPin >= 500) {
      std::swap(fromNodeId, toNodeId);
      std::swap(fromPin, toPin);
    }

    if (fromPin >= 500) {
      ShaderConnection conn;
      conn.id = nextLinkId++;
      conn.fromNodeId = fromNodeId;
      conn.fromOutputIndex = fromPin - 500;
      conn.toNodeId = toNodeId;
      conn.toInputIndex = toPin;
      connections.push_back(conn);
    }
  }

  // Handle link deletion
  int destroyedLinkId;
  if (ImNodes::IsLinkDestroyed(&destroyedLinkId)) {
    connections.erase(
        std::remove_if(connections.begin(), connections.end(),
                       [destroyedLinkId](const ShaderConnection& c) { return c.id == destroyedLinkId; }),
        connections.end());
  }
}

void ShaderGraph::addShaderNode(const std::string& name, const std::string& filePath) {
  // Check if shader already loaded and remove old version
  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    if (it->path == filePath) {
      nodes.erase(it); // Remove old version
      break;
    }
  }

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
  connections.push_back({nextLinkId++, fromNodeId, fromOutput, toNodeId, toInput});
}

void ShaderGraph::removeConnection(int connectionId) {
  if (connectionId >= 0 && connectionId < static_cast<int>(connections.size())) {
    connections.erase(connections.begin() + connectionId);
  }
}

void ShaderGraph::extractShaderInterface(const std::string& filePath, ShaderNode& node) {
  // Compile the shader to get SPIR-V
  ShaderCompiler compiler;
  auto compilationResult = compiler.compile(filePath);

  if (!compilationResult.success || compilationResult.spirvOutput.empty()) {
    std::cerr << "[ShaderGraph] Compilation failed or no SPIR-V output for: " << filePath << std::endl;
    return; // Could not compile
  }

  std::cerr << "[ShaderGraph] SPIR-V output: " << compilationResult.spirvOutput << std::endl;

  // Load SPIR-V binary
  std::ifstream spirvFile(compilationResult.spirvOutput, std::ios::binary | std::ios::ate);
  if (!spirvFile.is_open()) {
    std::cerr << "[ShaderGraph] Failed to open SPIR-V file: " << compilationResult.spirvOutput << std::endl;
    return;
  }

  std::streamsize size = spirvFile.tellg();
  std::cerr << "[ShaderGraph] SPIR-V file size: " << size << " bytes" << std::endl;
  spirvFile.seekg(0, std::ios::beg);

  std::vector<uint32_t> spirvCode(size / sizeof(uint32_t));
  if (!spirvFile.read(reinterpret_cast<char*>(spirvCode.data()), size)) {
    std::cerr << "[ShaderGraph] Failed to read SPIR-V file" << std::endl;
    return;
  }

  // Reflect SPIR-V module
  SpvReflectShaderModule module = {};
  SpvReflectResult result = spvReflectCreateShaderModule(spirvCode.size() * sizeof(uint32_t),
                                                          spirvCode.data(), &module);
  if (result != SPV_REFLECT_RESULT_SUCCESS) {
    std::cerr << "[ShaderGraph] spvReflectCreateShaderModule failed with result: " << result << std::endl;
    return;
  }

  std::cerr << "[ShaderGraph] SPIR-V module created successfully" << std::endl;

  // Extract input variables
  uint32_t inputCount = 0;
  spvReflectEnumerateInputVariables(&module, &inputCount, nullptr);
  std::cerr << "[ShaderGraph] Input variable count: " << inputCount << std::endl;

  std::vector<SpvReflectInterfaceVariable*> inputs(inputCount);
  spvReflectEnumerateInputVariables(&module, &inputCount, inputs.data());

  for (const auto& input : inputs) {
    if (input && input->name) {
      ShaderNode::Variable var;
      var.name = input->name;
      var.location = input->location;
      var.type = "in";
      node.inputs.push_back(var);
      std::cerr << "[ShaderGraph] Added input: " << input->name << " (location: " << input->location << ")" << std::endl;
    }
  }

  // Extract output variables
  uint32_t outputCount = 0;
  spvReflectEnumerateOutputVariables(&module, &outputCount, nullptr);
  std::cerr << "[ShaderGraph] Output variable count: " << outputCount << std::endl;

  std::vector<SpvReflectInterfaceVariable*> outputs(outputCount);
  spvReflectEnumerateOutputVariables(&module, &outputCount, outputs.data());

  for (const auto& output : outputs) {
    if (output && output->name) {
      ShaderNode::Variable var;
      var.name = output->name;
      var.location = output->location;
      var.type = "out";
      node.outputs.push_back(var);
      std::cerr << "[ShaderGraph] Added output: " << output->name << " (location: " << output->location << ")" << std::endl;
    }
  }

  // Also extract descriptor sets (textures, buffers, samplers)
  uint32_t descSetCount = 0;
  spvReflectEnumerateDescriptorSets(&module, &descSetCount, nullptr);
  std::cerr << "[ShaderGraph] Descriptor set count: " << descSetCount << std::endl;

  std::vector<SpvReflectDescriptorSet*> descSets(descSetCount);
  spvReflectEnumerateDescriptorSets(&module, &descSetCount, descSets.data());

  for (const auto& descSet : descSets) {
    if (descSet) {
      std::cerr << "[ShaderGraph] Processing descriptor set with " << descSet->binding_count << " bindings" << std::endl;
      for (uint32_t i = 0; i < descSet->binding_count; ++i) {
        const auto& binding = descSet->bindings[i];
        ShaderNode::Variable var;
        var.name = binding->name ? binding->name : "unnamed";
        var.location = binding->binding;
        var.type = "resource"; // texture, buffer, sampler, etc.
        node.inputs.push_back(var);
        std::cerr << "[ShaderGraph] Added resource: " << var.name << " (binding: " << var.location << ")" << std::endl;
      }
    }
  }

  std::cerr << "[ShaderGraph] Extraction complete: " << node.inputs.size() << " total inputs, "
            << node.outputs.size() << " outputs" << std::endl;

  // Cleanup
  spvReflectDestroyShaderModule(&module);
}
