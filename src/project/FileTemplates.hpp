#pragma once

#include <string>

class FileTemplates {
public:
  static std::string getTemplateForShaderType(const std::string& shaderType);
  static std::string getExtensionForShaderType(const std::string& shaderType);
};
