#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include "ShaderCompiler.hpp"

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
  std::vector<char> textBuffer;
  bool modified = false;
  static const size_t MAX_FILE_SIZE = 1024 * 1024; // 1MB max

  ShaderCompiler compiler;
  CompilationResult lastCompilationResult;
  bool showCompilationResults = false;
  bool lastResultWasLint = false;

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
};
