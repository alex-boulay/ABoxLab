#pragma once

#include <string>
#include <vector>

struct CompilationError {
  std::string file;
  int line;
  int column;
  std::string severity; // "error", "warning"
  std::string message;
};

struct CompilationResult {
  bool success;
  std::vector<CompilationError> diagnostics;
  std::string spirvOutput; // Path to compiled SPIR-V, if generated
};

class ShaderCompiler {
public:
  ShaderCompiler();
  ~ShaderCompiler();

  // Compile shader to SPIR-V
  CompilationResult compile(const std::string& shaderPath);

  // Lint shader from in-memory source (no file save)
  CompilationResult lintSource(const std::string& sourceCode, const std::string& filePath);

  // Just lint/validate without producing output (from disk)
  CompilationResult lint(const std::string& shaderPath);

private:
  std::string getShaderStageFromExtension(const std::string& filePath);
  std::string getTargetProfile(const std::string& shaderType);
  CompilationResult parseSlangDiagnostics(const std::string& diagnosticOutput);
};
