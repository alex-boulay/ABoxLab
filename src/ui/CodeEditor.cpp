#include "CodeEditor.hpp"
#include "imgui.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <set>

namespace fs = std::filesystem;

CodeEditor::CodeEditor() {
  textBuffer.resize(MAX_FILE_SIZE);
  textBuffer[0] = '\0';
}

CodeEditor::~CodeEditor() {
  if (lintThread.joinable()) {
    lintThread.join();
  }
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
  // Wait for any pending lint to complete
  if (lintThread.joinable()) {
    lintThread.join();
  }
  lintScheduled = false;
  lintResultReady = false;

  currentFilePath.clear();
  textBuffer[0] = '\0';
  modified = false;
  showCompilationResults = false;
}

bool CodeEditor::isShaderFile(const std::string& filePath) const {
  std::string ext = fs::path(filePath).extension().string();
  return ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".hlsl";
}

void CodeEditor::compileShader() {
  if (currentFilePath.empty()) return;

  // Save before compiling
  saveFile();

  lastCompilationResult = compiler.compile(currentFilePath);
  lastResultWasLint = false;
  showCompilationResults = true;
}

void CodeEditor::updateLinting() {
  // Check if we should start a new lint (debounce check)
  if (lintScheduled && !currentFilePath.empty() && isShaderFile(currentFilePath)) {
    auto elapsed = std::chrono::steady_clock::now() - lastEditTime;
    if (elapsed > std::chrono::milliseconds(LINT_DEBOUNCE_MS)) {
      lintScheduled = false;

      // Capture source snapshot for the thread
      std::string source(textBuffer.data());
      std::string path = currentFilePath;

      // Join previous thread if still running
      if (lintThread.joinable()) {
        lintThread.join();
      }

      // Launch new lint thread
      lintThread = std::thread([this, source, path]() {
        auto res = compiler.lintSource(source, path);
        std::lock_guard<std::mutex> lock(lintResultMutex);
        pendingLintResult = res;
        lintResultReady = true;
      });
    }
  }

  // Pick up result from background thread
  if (lintResultReady.load()) {
    std::lock_guard<std::mutex> lock(lintResultMutex);
    lastCompilationResult = pendingLintResult;
    lastResultWasLint = true;
    showCompilationResults = true;
    lintResultReady = false;
  }
}

void CodeEditor::renderCompilationResults() {
  ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
  if (ImGui::CollapsingHeader("Compilation Results", &showCompilationResults)) {
    // Show whether this is live linting or manual compile
    ImGui::Text("[%s]", lastResultWasLint ? "Live Linting" : "Manual Compile");
    ImGui::SameLine();

    if (lastCompilationResult.success) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Success");
    } else {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "✗ Failed");
    }

    ImGui::Separator();

    if (lastCompilationResult.diagnostics.empty()) {
      ImGui::TextDisabled("No diagnostics");
    } else {
      for (const auto& diag : lastCompilationResult.diagnostics) {
        ImVec4 color;
        if (diag.severity == "error") {
          color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
        } else if (diag.severity == "warning") {
          color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        } else {
          color = ImVec4(0.5f, 1.0f, 1.0f, 1.0f);
        }

        ImGui::TextColored(color, "[%s:%d:%d] %s", diag.severity.c_str(), diag.line,
                           diag.column, diag.message.c_str());
      }
    }
  }
}

void CodeEditor::render(float offsetX) {
  // Update linting state (check debounce and pick up results)
  updateLinting();

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

    // Compile button for shader files
    if (isShaderFile(currentFilePath)) {
      if (!modified) ImGui::SameLine();
      if (ImGui::Button("Compile (Ctrl+Shift+C)")) {
        compileShader();
      }
    }

    ImGui::Separator();

    // Text editor
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    ImVec2 textSize = ImVec2(contentRegion.x, contentRegion.y);

    // Reduce height if showing compilation results
    if (showCompilationResults) {
      textSize.y *= 0.6f;
    }
    ImGuiInputTextFlags textFlags = ImGuiInputTextFlags_AllowTabInput;

    // Draw error line highlights before the text editor
    if (!lastCompilationResult.diagnostics.empty()) {
      ImVec2 editorPos = ImGui::GetCursorScreenPos();
      ImDrawList* drawList = ImGui::GetWindowDrawList();
      float lineHeight = ImGui::GetTextLineHeight();

      // Get all unique error lines
      std::set<int> errorLines;
      for (const auto& diag : lastCompilationResult.diagnostics) {
        if (diag.line > 0) {
          errorLines.insert(diag.line - 1); // Convert to 0-based
        }
      }

      // Draw highlight rectangles for error lines
      for (int lineNum : errorLines) {
        float lineY = editorPos.y + (lineNum * lineHeight);
        ImVec2 lineStart(editorPos.x, lineY);
        ImVec2 lineEnd(editorPos.x + textSize.x, lineY + lineHeight);

        // Draw semi-transparent red background for error lines
        drawList->AddRectFilled(lineStart, lineEnd, ImGui::GetColorU32(ImVec4(1.0f, 0.0f, 0.0f, 0.15f)));
      }
    }

    if (ImGui::InputTextMultiline("##editor", textBuffer.data(), textBuffer.size(), textSize, textFlags)) {
      modified = true;
      // Start linting with debounce
      if (isShaderFile(currentFilePath)) {
        lastEditTime = std::chrono::steady_clock::now();
        lintScheduled = true;
      }
    }

    // Compilation results panel
    if (showCompilationResults) {
      ImGui::Spacing();
      ImVec2 resultsSize =
          ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
      ImGui::BeginChild("CompilationResults", resultsSize, true);
      renderCompilationResults();
      ImGui::EndChild();
    }

    // Keyboard shortcuts
    if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
      saveFile();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_C) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl) &&
        ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
      compileShader();
    }
  }

  ImGui::End();
}
