#include "NodeGraph.hpp"
#include "imgui.h"
#include "ShaderCompiler.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>

static const char* meshPrimitiveNames[] = { "Quad", "Cube", "Sphere" };

NodeGraph::NodeGraph() {
  imnodesCtx = ImNodes::CreateContext();
}

NodeGraph::~NodeGraph() {
  if (imnodesCtx) {
    ImNodes::DestroyContext(imnodesCtx);
  }
}

ImU32 NodeGraph::pinColor(PinType type) {
  switch (type) {
    case PinType::Geometry: return IM_COL32(100, 200, 100, 255);
    case PinType::Shader:   return IM_COL32(150, 100, 255, 255);
    case PinType::Texture:  return IM_COL32(200, 150, 50, 255);
    case PinType::Float:    return IM_COL32(150, 150, 150, 255);
    case PinType::Vec2:     return IM_COL32(100, 200, 200, 255);
    case PinType::Vec3:     return IM_COL32(100, 150, 255, 255);
    case PinType::Vec4:     return IM_COL32(200, 100, 200, 255);
  }
  return IM_COL32(200, 200, 200, 255);
}

ImNodesPinShape NodeGraph::pinShape(PinType type) {
  switch (type) {
    case PinType::Geometry: return ImNodesPinShape_TriangleFilled;
    case PinType::Shader:   return ImNodesPinShape_QuadFilled;
    case PinType::Texture:  return ImNodesPinShape_QuadFilled;
    default:                return ImNodesPinShape_CircleFilled;
  }
}

bool NodeGraph::canConnect(PinType from, PinType to) const {
  if (from == to) return true;
  // Allow float -> vec components, vec3 -> color, etc.
  if (to == PinType::Vec3 && from == PinType::Float) return true;
  if (to == PinType::Vec4 && (from == PinType::Vec3 || from == PinType::Float)) return true;
  return false;
}

void NodeGraph::render(float offsetX, float offsetY, float width, float height) {
  ImNodes::SetCurrentContext(imnodesCtx);

  static bool initialized = false;
  if (!initialized) {
    ImNodes::StyleColorsDark();
    ImNodes::GetIO().LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
    initialized = true;
  }

  // Toolbar
  if (ImGui::Button("+ Mesh"))    addMeshNode();
  ImGui::SameLine();
  if (ImGui::Button("+ Texture")) addTextureNode("Texture", "");
  ImGui::SameLine();
  if (ImGui::Button("+ Output"))  addOutputNode();
  ImGui::SameLine();
  if (ImGui::Button("+ Float"))   addFloatNode();
  ImGui::SameLine();
  if (ImGui::Button("+ Vec3"))    addVec3Node();
  ImGui::SameLine();
  if (ImGui::Button("+ Color"))   addColorNode();
  ImGui::Separator();

  ImNodes::BeginNodeEditor();

  // Render all nodes
  for (auto& node : nodes) {
    if (node.needsPositioning) {
      float x = 50.0f + (node.id % 4) * 300.0f;
      float y = 50.0f + (node.id / 4) * 250.0f;
      ImNodes::SetNodeGridSpacePos(node.id, ImVec2(x, y));
      node.needsPositioning = false;
    }
    renderNode(node);
  }

  // Render links
  for (const auto& conn : connections) {
    int startAttr = outputAttrId(conn.fromNodeId, conn.fromOutputIndex);
    int endAttr = inputAttrId(conn.toNodeId, conn.toInputIndex);
    ImNodes::Link(conn.id, startAttr, endAttr);
  }

  ImNodes::MiniMap(0.15f, ImNodesMiniMapLocation_BottomRight);
  ImNodes::EndNodeEditor();

  // Right-click context menu
  renderContextMenu();

  // Handle new link creation
  int startAttr, endAttr;
  if (ImNodes::IsLinkCreated(&startAttr, &endAttr)) {
    int fromNodeId = startAttr / 1000;
    int fromPin = startAttr % 1000;
    int toNodeId = endAttr / 1000;
    int toPin = endAttr % 1000;

    // Normalize direction: outputs are >= 500
    if (fromPin < 500 && toPin >= 500) {
      std::swap(fromNodeId, toNodeId);
      std::swap(fromPin, toPin);
    }

    if (fromPin >= 500) {
      Connection conn;
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
                       [destroyedLinkId](const Connection& c) { return c.id == destroyedLinkId; }),
        connections.end());
  }
}

void NodeGraph::renderNode(GraphNode& node) {
  ImNodes::BeginNode(node.id);

  // Title bar with type-based coloring
  ImNodes::BeginNodeTitleBar();
  switch (node.type) {
    case NodeType::Shader:  ImGui::TextColored(ImVec4(0.6f, 0.4f, 1.0f, 1.0f), "%s", node.name.c_str()); break;
    case NodeType::Mesh:    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", node.name.c_str()); break;
    case NodeType::Texture: ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1.0f), "%s", node.name.c_str()); break;
    case NodeType::Output:  ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", node.name.c_str()); break;
    default:                ImGui::TextUnformatted(node.name.c_str()); break;
  }
  ImNodes::EndNodeTitleBar();

  // Inline controls for specific node types
  switch (node.type) {
    case NodeType::Mesh: {
      ImGui::PushItemWidth(120.0f);
      ImGui::Combo("##prim", &node.primitiveIndex, meshPrimitiveNames, 3);
      ImGui::PopItemWidth();
      break;
    }
    case NodeType::Float: {
      ImGui::PushItemWidth(100.0f);
      ImGui::DragFloat("##val", &node.valueFloat, 0.01f);
      ImGui::PopItemWidth();
      break;
    }
    case NodeType::Vec3:
    case NodeType::Color: {
      ImGui::PushItemWidth(150.0f);
      if (node.type == NodeType::Color)
        ImGui::ColorEdit3("##col", node.valueVec3, ImGuiColorEditFlags_NoInputs);
      else
        ImGui::DragFloat3("##vec", node.valueVec3, 0.01f);
      ImGui::PopItemWidth();
      break;
    }
    default: break;
  }

  // Input pins
  for (int i = 0; i < static_cast<int>(node.inputs.size()); ++i) {
    const auto& pin = node.inputs[i];
    ImNodes::BeginInputAttribute(inputAttrId(node.id, i), pinShape(pin.type));
    ImGui::TextUnformatted(pin.name.c_str());
    ImNodes::EndInputAttribute();
  }

  // Output pins
  for (int i = 0; i < static_cast<int>(node.outputs.size()); ++i) {
    const auto& pin = node.outputs[i];
    ImNodes::BeginOutputAttribute(outputAttrId(node.id, i), pinShape(pin.type));
    ImGui::TextUnformatted(pin.name.c_str());
    ImNodes::EndOutputAttribute();
  }

  ImNodes::EndNode();
}

void NodeGraph::renderContextMenu() {
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImNodes::IsEditorHovered()) {
    ImGui::OpenPopup("NodeGraphContextMenu");
  }

  if (ImGui::BeginPopup("NodeGraphContextMenu")) {
    if (ImGui::BeginMenu("Add Node")) {
      if (ImGui::MenuItem("Mesh"))       addMeshNode();
      if (ImGui::MenuItem("Texture"))    addTextureNode("Texture", "");
      if (ImGui::MenuItem("Output"))     addOutputNode();
      ImGui::Separator();
      if (ImGui::MenuItem("Float"))      addFloatNode();
      if (ImGui::MenuItem("Vec3"))       addVec3Node();
      if (ImGui::MenuItem("Color"))      addColorNode();
      ImGui::EndMenu();
    }

    // Delete selected nodes
    int numSelected = ImNodes::NumSelectedNodes();
    if (numSelected > 0 && ImGui::MenuItem("Delete Selected")) {
      std::vector<int> selectedIds(numSelected);
      ImNodes::GetSelectedNodes(selectedIds.data());
      for (int id : selectedIds) {
        removeNode(id);
      }
    }

    ImGui::EndPopup();
  }
}

// --- Node creation ---

void NodeGraph::addShaderNode(const std::string& name, const std::string& filePath) {
  // Remove old version if recompiling same file
  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    if (it->filePath == filePath) {
      nodes.erase(it);
      break;
    }
  }

  GraphNode node;
  node.id = nextNodeId++;
  node.type = NodeType::Shader;
  node.name = name;
  node.filePath = filePath;

  // "Stage" output for connecting to the Material Output
  node.outputs.push_back({"Stage", PinType::Shader});

  extractShaderInterface(filePath, node);

  nodes.push_back(node);
}

void NodeGraph::addMeshNode() {
  GraphNode node;
  node.id = nextNodeId++;
  node.type = NodeType::Mesh;
  node.name = "Mesh";
  node.outputs.push_back({"Geometry", PinType::Geometry});
  nodes.push_back(node);
}

void NodeGraph::addTextureNode(const std::string& name, const std::string& filePath) {
  GraphNode node;
  node.id = nextNodeId++;
  node.type = NodeType::Texture;
  node.name = name;
  node.filePath = filePath;
  node.outputs.push_back({"Color", PinType::Vec4});
  node.outputs.push_back({"UV", PinType::Vec2});
  nodes.push_back(node);
}

void NodeGraph::addOutputNode() {
  // Only allow one output node
  for (const auto& n : nodes) {
    if (n.type == NodeType::Output) return;
  }

  GraphNode node;
  node.id = nextNodeId++;
  node.type = NodeType::Output;
  node.name = "Material Output";
  node.inputs.push_back({"Geometry", PinType::Geometry});
  node.inputs.push_back({"Vertex Shader", PinType::Shader});
  node.inputs.push_back({"Fragment Shader", PinType::Shader});
  node.inputs.push_back({"Albedo", PinType::Vec4});
  nodes.push_back(node);
}

void NodeGraph::addFloatNode(float value) {
  GraphNode node;
  node.id = nextNodeId++;
  node.type = NodeType::Float;
  node.name = "Float";
  node.valueFloat = value;
  node.outputs.push_back({"Value", PinType::Float});
  nodes.push_back(node);
}

void NodeGraph::addVec3Node(float x, float y, float z) {
  GraphNode node;
  node.id = nextNodeId++;
  node.type = NodeType::Vec3;
  node.name = "Vec3";
  node.valueVec3[0] = x; node.valueVec3[1] = y; node.valueVec3[2] = z;
  node.outputs.push_back({"Value", PinType::Vec3});
  nodes.push_back(node);
}

void NodeGraph::addColorNode(float r, float g, float b) {
  GraphNode node;
  node.id = nextNodeId++;
  node.type = NodeType::Color;
  node.name = "Color";
  node.valueVec3[0] = r; node.valueVec3[1] = g; node.valueVec3[2] = b;
  node.outputs.push_back({"RGB", PinType::Vec3});
  node.outputs.push_back({"R", PinType::Float});
  node.outputs.push_back({"G", PinType::Float});
  node.outputs.push_back({"B", PinType::Float});
  nodes.push_back(node);
}

void NodeGraph::removeNode(int nodeId) {
  nodes.erase(
      std::remove_if(nodes.begin(), nodes.end(),
                     [nodeId](const GraphNode& n) { return n.id == nodeId; }),
      nodes.end());
  // Remove connections referencing this node
  connections.erase(
      std::remove_if(connections.begin(), connections.end(),
                     [nodeId](const Connection& c) {
                       return c.fromNodeId == nodeId || c.toNodeId == nodeId;
                     }),
      connections.end());
}

// --- SPIR-V reflection for shader nodes ---

void NodeGraph::extractShaderInterface(const std::string& filePath, GraphNode& node) {
  ShaderCompiler compiler;
  auto compilationResult = compiler.compile(filePath);

  if (!compilationResult.success || compilationResult.spirvOutput.empty()) {
    std::cerr << "[NodeGraph] Compilation failed for: " << filePath << std::endl;
    return;
  }

  std::ifstream spirvFile(compilationResult.spirvOutput, std::ios::binary | std::ios::ate);
  if (!spirvFile.is_open()) {
    std::cerr << "[NodeGraph] Failed to open SPIR-V: " << compilationResult.spirvOutput << std::endl;
    return;
  }

  std::streamsize size = spirvFile.tellg();
  spirvFile.seekg(0, std::ios::beg);

  std::vector<uint32_t> spirvCode(size / sizeof(uint32_t));
  if (!spirvFile.read(reinterpret_cast<char*>(spirvCode.data()), size)) return;

  SpvReflectShaderModule module = {};
  SpvReflectResult result = spvReflectCreateShaderModule(
      spirvCode.size() * sizeof(uint32_t), spirvCode.data(), &module);
  if (result != SPV_REFLECT_RESULT_SUCCESS) return;

  // Extract inputs as node input pins
  uint32_t inputCount = 0;
  spvReflectEnumerateInputVariables(&module, &inputCount, nullptr);
  std::vector<SpvReflectInterfaceVariable*> inputs(inputCount);
  spvReflectEnumerateInputVariables(&module, &inputCount, inputs.data());

  for (const auto& input : inputs) {
    if (input && input->name && input->name[0] != '\0') {
      PinType pt = PinType::Float;
      if (input->numeric.vector.component_count == 2) pt = PinType::Vec2;
      else if (input->numeric.vector.component_count == 3) pt = PinType::Vec3;
      else if (input->numeric.vector.component_count == 4) pt = PinType::Vec4;
      node.inputs.push_back({input->name, pt});
    }
  }

  // Extract outputs as node output pins
  uint32_t outputCount = 0;
  spvReflectEnumerateOutputVariables(&module, &outputCount, nullptr);
  std::vector<SpvReflectInterfaceVariable*> outputs(outputCount);
  spvReflectEnumerateOutputVariables(&module, &outputCount, outputs.data());

  for (const auto& output : outputs) {
    if (output && output->name && output->name[0] != '\0') {
      PinType pt = PinType::Float;
      if (output->numeric.vector.component_count == 2) pt = PinType::Vec2;
      else if (output->numeric.vector.component_count == 3) pt = PinType::Vec3;
      else if (output->numeric.vector.component_count == 4) pt = PinType::Vec4;
      node.outputs.push_back({output->name, pt});
    }
  }

  // Extract descriptor bindings as additional inputs
  uint32_t descSetCount = 0;
  spvReflectEnumerateDescriptorSets(&module, &descSetCount, nullptr);
  std::vector<SpvReflectDescriptorSet*> descSets(descSetCount);
  spvReflectEnumerateDescriptorSets(&module, &descSetCount, descSets.data());

  for (const auto& descSet : descSets) {
    for (uint32_t b = 0; b < descSet->binding_count; ++b) {
      const auto* binding = descSet->bindings[b];
      if (binding && binding->name) {
        PinType pt = (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            ? PinType::Texture : PinType::Float;
        node.inputs.push_back({binding->name, pt});
      }
    }
  }

  spvReflectDestroyShaderModule(&module);
}

// --- Evaluate graph bindings ---

GraphBindings NodeGraph::evaluate() const {
  GraphBindings result;

  // Find the Output node
  const GraphNode* outputNode = nullptr;
  for (const auto& n : nodes) {
    if (n.type == NodeType::Output) { outputNode = &n; break; }
  }
  if (!outputNode) return result;

  // For each input on the Output node, trace the connection back
  // Output node inputs: 0=Geometry, 1=Vertex Shader, 2=Fragment Shader, 3=Albedo
  for (const auto& conn : connections) {
    if (conn.toNodeId != outputNode->id) continue;

    // Find the source node
    const GraphNode* srcNode = nullptr;
    for (const auto& n : nodes) {
      if (n.id == conn.fromNodeId) { srcNode = &n; break; }
    }
    if (!srcNode) continue;

    switch (conn.toInputIndex) {
      case 0: // Geometry
        if (srcNode->type == NodeType::Mesh) {
          result.meshPrimitive = srcNode->primitiveIndex;
        }
        break;
      case 1: // Vertex Shader
        if (srcNode->type == NodeType::Shader) {
          result.vertShaderSpv = srcNode->filePath + ".spv";
        }
        break;
      case 2: // Fragment Shader
        if (srcNode->type == NodeType::Shader) {
          result.fragShaderSpv = srcNode->filePath + ".spv";
        }
        break;
      case 3: // Albedo (texture)
        if (srcNode->type == NodeType::Texture) {
          result.texturePath = srcNode->filePath;
        }
        break;
    }
  }

  result.valid = (result.meshPrimitive >= 0 &&
                  !result.vertShaderSpv.empty() &&
                  !result.fragShaderSpv.empty());
  return result;
}
