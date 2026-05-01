#include "ShaderCompiler.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <slang.h>

namespace fs = std::filesystem;

ShaderCompiler::ShaderCompiler() {
}

ShaderCompiler::~ShaderCompiler() {
}

std::string ShaderCompiler::getShaderStageFromExtension(const std::string& filePath) {
  std::string ext = fs::path(filePath).extension().string();
  if (ext == ".vert") return "vertex";
  if (ext == ".frag") return "fragment";
  if (ext == ".comp") return "compute";
  if (ext == ".hlsl") {
    // Handle .hlsl.vert, .hlsl.frag, .hlsl.comp
    std::string filename = fs::path(filePath).filename().string();
    if (filename.find(".vert") != std::string::npos) return "vertex";
    if (filename.find(".frag") != std::string::npos) return "fragment";
    if (filename.find(".comp") != std::string::npos) return "compute";
  }
  return "fragment"; // Default
}

std::string ShaderCompiler::getTargetProfile(const std::string& shaderType) {
  if (shaderType == "vertex") return "vs_6_0";
  if (shaderType == "fragment") return "ps_6_0";
  if (shaderType == "compute") return "cs_6_0";
  return "ps_6_0";
}

CompilationResult ShaderCompiler::parseSlangDiagnostics(const std::string& diagnosticOutput) {
  CompilationResult result;
  result.success = true;
  result.diagnostics.clear();

  std::istringstream stream(diagnosticOutput);
  std::string line;

  while (std::getline(stream, line)) {
    if (line.empty()) continue;

    // Parse Slang diagnostic format: file(line,col): error/warning: message
    size_t parenPos = line.find('(');
    if (parenPos == std::string::npos) continue;

    size_t closeParenPos = line.find(')', parenPos);
    if (closeParenPos == std::string::npos) continue;

    std::string file = line.substr(0, parenPos);
    std::string coords = line.substr(parenPos + 1, closeParenPos - parenPos - 1);

    int lineNum = 0;
    int colNum = 0;
    size_t commaPos = coords.find(',');
    if (commaPos != std::string::npos) {
      lineNum = std::stoi(coords.substr(0, commaPos));
      colNum = std::stoi(coords.substr(commaPos + 1));
    }

    size_t colonPos = line.find(':', closeParenPos);
    if (colonPos == std::string::npos) continue;

    size_t nextColonPos = line.find(':', colonPos + 1);
    if (nextColonPos == std::string::npos) continue;

    std::string severity = line.substr(colonPos + 1, nextColonPos - colonPos - 1);
    // Trim whitespace
    severity.erase(0, severity.find_first_not_of(" \t"));
    severity.erase(severity.find_last_not_of(" \t") + 1);

    std::string message = line.substr(nextColonPos + 1);
    message.erase(0, message.find_first_not_of(" \t"));

    CompilationError error;
    error.file = file;
    error.line = lineNum;
    error.column = colNum;
    error.severity = severity;
    error.message = message;

    result.diagnostics.push_back(error);

    if (severity == "error") {
      result.success = false;
    }
  }

  return result;
}

CompilationResult ShaderCompiler::compile(const std::string& shaderPath) {
  CompilationResult result;
  result.success = true;

  // Check if file exists
  if (!fs::exists(shaderPath)) {
    result.success = false;
    CompilationError err;
    err.file = shaderPath;
    err.line = 0;
    err.column = 0;
    err.severity = "error";
    err.message = "File not found: " + shaderPath;
    result.diagnostics.push_back(err);
    std::cerr << "[ShaderCompiler] File not found: " << shaderPath << std::endl;
    return result;
  }

  // Check if it's a regular file
  if (!fs::is_regular_file(shaderPath)) {
    result.success = false;
    CompilationError err;
    err.file = shaderPath;
    err.line = 0;
    err.column = 0;
    err.severity = "error";
    err.message = "Path is not a regular file: " + shaderPath;
    result.diagnostics.push_back(err);
    std::cerr << "[ShaderCompiler] Not a regular file: " << shaderPath << std::endl;
    return result;
  }

  // Create Slang session with GLSL support enabled
  slang::ISession* session = nullptr;
  slang::IGlobalSession* globalSession = nullptr;

  SlangGlobalSessionDesc globalSessionDesc = {};
  globalSessionDesc.enableGLSL = true;

  SlangResult slangRes = slang::createGlobalSession(&globalSessionDesc, &globalSession);
  if (SLANG_FAILED(slangRes) || !globalSession) {
    result.success = false;
    CompilationError err;
    err.severity = "error";
    err.message = "Failed to create Slang global session. Slang compiler may not be properly initialized.";
    result.diagnostics.push_back(err);
    std::cerr << "[ShaderCompiler] Failed to create global session: " << slangRes << std::endl;
    return result;
  }

  // Create session with default options
  slang::SessionDesc sessionDesc = {};
  slangRes = globalSession->createSession(sessionDesc, &session);

  if (SLANG_FAILED(slangRes) || !session) {
    result.success = false;
    CompilationError err;
    err.severity = "error";
    err.message = "Failed to create Slang compilation session. Check Slang compiler setup.";
    result.diagnostics.push_back(err);
    std::cerr << "[ShaderCompiler] Failed to create session: " << slangRes << std::endl;
    globalSession->release();
    return result;
  }

  // Read shader source code
  std::ifstream shaderFile(shaderPath);
  if (!shaderFile.is_open()) {
    result.success = false;
    CompilationError err;
    err.file = shaderPath;
    err.severity = "error";
    err.message = "Cannot open shader file for reading.";
    result.diagnostics.push_back(err);
    std::cerr << "[ShaderCompiler] Cannot open file: " << shaderPath << std::endl;
    session->release();
    globalSession->release();
    return result;
  }

  std::stringstream shaderSource;
  shaderSource << shaderFile.rdbuf();
  shaderFile.close();
  std::string sourceCode = shaderSource.str();

  std::cerr << "[ShaderCompiler] Read source code, length: " << sourceCode.length() << " bytes" << std::endl;

  // Create compile request
  slang::ICompileRequest* compileRequest = nullptr;
  SlangResult reqRes = session->createCompileRequest(&compileRequest);
  if (SLANG_FAILED(reqRes) || !compileRequest) {
    result.success = false;
    CompilationError err;
    err.file = shaderPath;
    err.severity = "error";
    err.message = "Failed to create Slang compile request.";
    result.diagnostics.push_back(err);
    std::cerr << "[ShaderCompiler] Failed to create compile request: " << reqRes << std::endl;
    session->release();
    globalSession->release();
    return result;
  }

  // Determine shader language from file extension
  std::string stageStr = getShaderStageFromExtension(shaderPath);
  std::cerr << "[ShaderCompiler] Detected shader stage: " << stageStr << std::endl;

  // Add translation unit with GLSL source language
  int translationUnitIndex = compileRequest->addTranslationUnit(SLANG_SOURCE_LANGUAGE_GLSL, "shader");
  compileRequest->addTranslationUnitSourceString(translationUnitIndex, "shader", sourceCode.c_str());

  // Compile
  SlangResult compileRes = compileRequest->compile();
  if (SLANG_FAILED(compileRes)) {
    result.success = false;
    std::cerr << "[ShaderCompiler] Compilation failed with result: " << compileRes << std::endl;

    // Try to get diagnostics
    const char* diagnostics = compileRequest->getDiagnosticOutput();
    if (diagnostics) {
      std::cerr << "[ShaderCompiler] Diagnostics:\n" << diagnostics << std::endl;
      result = parseSlangDiagnostics(std::string(diagnostics));
      result.success = false; // Ensure marked as failure
    } else {
      CompilationError err;
      err.file = shaderPath;
      err.severity = "error";
      err.message = "Shader compilation failed. Check shader syntax and ensure it's a valid GLSL/HLSL file.";
      result.diagnostics.push_back(err);
    }
  } else {
    std::cerr << "[ShaderCompiler] Compilation succeeded" << std::endl;
    // TODO: Generate SPIR-V output
  }

  compileRequest->release();

  // Clean up
  if (session) session->release();
  if (globalSession) globalSession->release();

  return result;
}

CompilationResult ShaderCompiler::lintSource(const std::string& sourceCode, const std::string& filePath) {
  CompilationResult result;
  result.success = true;

  // Create Slang session with GLSL support enabled
  slang::ISession* session = nullptr;
  slang::IGlobalSession* globalSession = nullptr;

  SlangGlobalSessionDesc globalSessionDesc = {};
  globalSessionDesc.enableGLSL = true;

  SlangResult slangRes = slang::createGlobalSession(&globalSessionDesc, &globalSession);
  if (SLANG_FAILED(slangRes) || !globalSession) {
    result.success = false;
    CompilationError err;
    err.severity = "error";
    err.message = "Failed to create Slang global session.";
    result.diagnostics.push_back(err);
    std::cerr << "[ShaderCompiler] Failed to create global session: " << slangRes << std::endl;
    return result;
  }

  // Create session with default options
  slang::SessionDesc sessionDesc = {};
  slangRes = globalSession->createSession(sessionDesc, &session);

  if (SLANG_FAILED(slangRes) || !session) {
    result.success = false;
    CompilationError err;
    err.severity = "error";
    err.message = "Failed to create Slang compilation session.";
    result.diagnostics.push_back(err);
    std::cerr << "[ShaderCompiler] Failed to create session: " << slangRes << std::endl;
    globalSession->release();
    return result;
  }

  // Create compile request
  slang::ICompileRequest* compileRequest = nullptr;
  SlangResult reqRes = session->createCompileRequest(&compileRequest);
  if (SLANG_FAILED(reqRes) || !compileRequest) {
    result.success = false;
    CompilationError err;
    err.severity = "error";
    err.message = "Failed to create Slang compile request.";
    result.diagnostics.push_back(err);
    std::cerr << "[ShaderCompiler] Failed to create compile request: " << reqRes << std::endl;
    session->release();
    globalSession->release();
    return result;
  }

  // Determine shader language from file extension
  std::string stageStr = getShaderStageFromExtension(filePath);
  std::cerr << "[ShaderCompiler] Linting: detected shader stage: " << stageStr << std::endl;

  // Add translation unit with GLSL source
  int translationUnitIndex = compileRequest->addTranslationUnit(SLANG_SOURCE_LANGUAGE_GLSL, "shader");
  compileRequest->addTranslationUnitSourceString(translationUnitIndex, "shader", sourceCode.c_str());

  // Compile
  SlangResult compileRes = compileRequest->compile();
  if (SLANG_FAILED(compileRes)) {
    result.success = false;
    std::cerr << "[ShaderCompiler] Linting failed with result: " << compileRes << std::endl;

    // Get diagnostics
    const char* diagnostics = compileRequest->getDiagnosticOutput();
    if (diagnostics) {
      result = parseSlangDiagnostics(std::string(diagnostics));
      result.success = false;
    } else {
      CompilationError err;
      err.severity = "error";
      err.message = "Shader linting failed.";
      result.diagnostics.push_back(err);
    }
  } else {
    std::cerr << "[ShaderCompiler] Linting succeeded" << std::endl;
  }

  compileRequest->release();

  // Clean up
  if (session) session->release();
  if (globalSession) globalSession->release();

  return result;
}

CompilationResult ShaderCompiler::lint(const std::string& shaderPath) {
  // Linting is same as compile but without generating output
  return compile(shaderPath);
}
