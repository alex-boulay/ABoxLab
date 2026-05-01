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
  // Configure the text editor
  editor.SetTabSize(4);
  editor.SetAutoIndentEnabled(true);
  editor.SetShowLineNumbersEnabled(true);
  editor.SetShowMatchingBrackets(true);
  editor.SetPalette(TextEditor::GetDarkPalette());

  // Set up change callback for debounce
  editor.SetChangeCallback([this]() {
    modified = true;
    if (isShaderFile(currentFilePath)) {
      lastEditTime = std::chrono::steady_clock::now();
      lintScheduled = true;
    }
  }, 0);
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

  editor.SetText(content);

  // Set language based on file extension
  std::string ext = fs::path(filePath).extension().string();
  if (ext == ".vert" || ext == ".frag" || ext == ".comp") {
    editor.SetLanguage(TextEditor::Language::Glsl());
  } else if (ext == ".hlsl") {
    editor.SetLanguage(TextEditor::Language::Hlsl());
  }

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

  std::string text = editor.GetText();
  file << text;
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
  editor.ClearText();
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
      std::string source = editor.GetText();
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

    // Update error markers in the editor
    editor.ClearMarkers();
    std::cerr << "[CodeEditor] Updating " << lastCompilationResult.diagnostics.size() << " markers\n";
    for (const auto& diag : lastCompilationResult.diagnostics) {
      if (diag.line > 0) {
        ImU32 color = (diag.severity == "error")
            ? IM_COL32(255, 60, 60, 200)
            : IM_COL32(255, 220, 0, 200);
        std::cerr << "[CodeEditor] Adding marker at line " << diag.line << ": " << diag.message << "\n";
        editor.AddMarker(diag.line, color, color, diag.severity, diag.message);
      }
    }
  }
}

void CodeEditor::renderCompilationResults() {
  // Use CollapsingHeader for proper collapse behavior (no extra space when collapsed)
  if (!ImGui::CollapsingHeader("Compilation Results", &showCompilationResults, ImGuiTreeNodeFlags_DefaultOpen)) {
    return; // Content is collapsed
  }

  // Set background color based on success/failure
  ImVec4 bgColor;
  if (lastCompilationResult.success) {
    bgColor = ImVec4(0.15f, 0.25f, 0.15f, 0.8f); // Dark green tint
  } else {
    bgColor = ImVec4(0.25f, 0.15f, 0.15f, 0.8f); // Dark red tint
  }

  ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
  ImGui::BeginChild("CompilationResultsPanel", ImVec2(0, 0), true);
  ImGui::PopStyleColor();

  // Show whether this is live linting or manual compile
  ImGui::Text("[%s]", lastResultWasLint ? "Live Linting" : "Manual Compile");
  ImGui::SameLine();

  if (lastCompilationResult.success) {
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "✓ Success");
  } else {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "✗ Failed");
  }

  ImGui::Separator();

  // Build diagnostics text for display
  static std::string diagnosticsText;
  if (lastCompilationResult.diagnostics.empty()) {
    diagnosticsText = "No diagnostics";
  } else {
    diagnosticsText.clear();
    for (const auto& diag : lastCompilationResult.diagnostics) {
      if (!diagnosticsText.empty()) diagnosticsText += "\n";
      diagnosticsText += "[" + diag.severity + ":" + std::to_string(diag.line) +
                        ":" + std::to_string(diag.column) + "] " + diag.message;
    }
  }

  // Display as read-only text input (allows selecting all text with Ctrl+A)
  ImVec2 diagSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
  ImGui::InputTextMultiline("##diagnostics", diagnosticsText.data(),
                            diagnosticsText.capacity() + 1, diagSize,
                            ImGuiInputTextFlags_ReadOnly);

  ImGui::EndChild();
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

    // Text editor and compilation results
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    ImVec2 textSize = ImVec2(contentRegion.x, contentRegion.y);

    // Split space between editor and compilation results if showing results
    if (showCompilationResults) {
      textSize.y = contentRegion.y * 0.6f;
    }

    // Render the text editor (change callback handles modified/linting)
    editor.Render("##editor", textSize);

    // Focus handling: Tab to focus, track if editor has focus
    ImGuiIO& io = ImGui::GetIO();
    bool editorItemHovered = ImGui::IsItemHovered();
    bool editorItemFocused = ImGui::IsItemFocused();

    if (editorItemFocused) {
      editorHasFocus = true;
      // Prevent Return key from exiting the editor
      if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        io.WantTextInput = true;
      }
    } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      editorHasFocus = false;
    }

    // Allow Tab to focus the editor if it's not already focused
    if (!editorHasFocus && ImGui::IsKeyPressed(ImGuiKey_Tab) && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
      editorHasFocus = true;
    }

    // Compilation results panel
    renderCompilationResults();

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
