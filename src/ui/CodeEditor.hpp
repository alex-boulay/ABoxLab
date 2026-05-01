#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include "ShaderCompiler.hpp"
#include "ShaderGraph.hpp"
#include "TextEditor.h"

class CodeEditor {
public:
  CodeEditor();
  ~CodeEditor();

  void render(float offsetX);
  void openFile(const std::string& filePath);
  void saveFile();
  void closeFile();
  void compileShader();

  bool hasOpenFile() const { return !currentFilePath.empty(); }
  const std::string& getCurrentFilePath() const { return currentFilePath; }
  bool isModified() const { return modified; }

private:
  std::string currentFilePath;
  bool modified = false;

  TextEditor editor;
  ShaderCompiler compiler;
  ShaderGraph shaderGraph;
  CompilationResult lastCompilationResult;
  bool showCompilationResults = false;
  bool lastResultWasLint = false;
  bool editorHasFocus = false;

  // Tab system
  enum ViewMode { CODE_EDITOR, SHADER_GRAPH };
  ViewMode currentView = CODE_EDITOR;

  // Real-time linting with debounce
  std::chrono::steady_clock::time_point lastEditTime;
  bool lintScheduled = false;
  std::thread lintThread;
  std::mutex lintResultMutex;
  CompilationResult pendingLintResult;
  std::atomic<bool> lintResultReady{false};
  static constexpr int LINT_DEBOUNCE_MS = 600;

  void loadFileContent(const std::string& filePath);
  void renderCompilationResults();
  bool isShaderFile(const std::string& filePath) const;
  void updateLinting();
  void renderCodeEditor();
  void renderShaderGraphView();
};
