#include "CodeEditor.hpp"
#include "imgui.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

CodeEditor::CodeEditor() {
  textBuffer.resize(MAX_FILE_SIZE);
  textBuffer[0] = '\0';
}

void CodeEditor::loadFileContent(const std::string& filePath) {
  std::ifstream file(filePath);
  if (!file) {
    std::cerr << "Failed to open file: " << filePath << std::endl;
    return;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  if (content.size() >= MAX_FILE_SIZE) {
    std::cerr << "File too large: " << filePath << std::endl;
    return;
  }

  std::copy(content.begin(), content.end(), textBuffer.begin());
  textBuffer[content.size()] = '\0';
  modified = false;
}

void CodeEditor::openFile(const std::string& filePath) {
  if (!fs::exists(filePath) || !fs::is_regular_file(filePath)) {
    return;
  }

  currentFilePath = filePath;
  loadFileContent(filePath);
}

void CodeEditor::saveFile() {
  if (currentFilePath.empty()) {
    return;
  }

  std::ofstream file(currentFilePath);
  if (!file) {
    std::cerr << "Failed to save file: " << currentFilePath << std::endl;
    return;
  }

  file << textBuffer.data();
  file.close();
  modified = false;
}

void CodeEditor::closeFile() {
  currentFilePath.clear();
  textBuffer[0] = '\0';
  modified = false;
}

void CodeEditor::render(float offsetX) {
  ImGuiViewport* viewport = ImGui::GetMainViewport();

  // Position editor to the right of the workspace
  ImVec2 editorPos = ImVec2(viewport->Pos.x + offsetX, viewport->Pos.y + ImGui::GetFrameHeight());
  ImVec2 editorSize = ImVec2(viewport->Size.x - offsetX, viewport->Size.y - ImGui::GetFrameHeight());

  ImGui::SetNextWindowPos(editorPos);
  ImGui::SetNextWindowSize(editorSize);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse;

  std::string windowTitle = "Editor";
  if (!currentFilePath.empty()) {
    std::string filename = fs::path(currentFilePath).filename().string();
    windowTitle = filename + (modified ? " *" : "");
  }

  ImGui::Begin(windowTitle.c_str(), nullptr, flags);

  if (currentFilePath.empty()) {
    ImGui::TextDisabled("No file open");
    ImGui::TextWrapped("Click on a file in the workspace to open it");
  } else {
    // File path display
    ImGui::Text("File: %s", currentFilePath.c_str());
    ImGui::Separator();

    // Save button
    if (modified) {
      if (ImGui::Button("Save (Ctrl+S)")) {
        saveFile();
      }
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Modified");
    }

    ImGui::Separator();

    // Text editor
    ImVec2 textSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
    ImGuiInputTextFlags textFlags = ImGuiInputTextFlags_AllowTabInput;

    if (ImGui::InputTextMultiline("##editor", textBuffer.data(), textBuffer.size(), textSize, textFlags)) {
      modified = true;
    }

    // Keyboard shortcut for saving
    if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
      saveFile();
    }
  }

  ImGui::End();
}
