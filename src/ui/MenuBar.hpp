#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include <string>

class ProjectManager;

class MenuBar {
public:
  MenuBar() = default;
  ~MenuBar() = default;

  void render(GLFWwindow* window);
  void setProjectManager(ProjectManager* pm) { projectManager = pm; }

  // Callbacks
  void setOnCreateProjectCallback(std::function<void(const std::string&, const std::string&)> callback) {
    onCreateProject = callback;
  }
  void setOnOpenProjectCallback(std::function<void(const std::string&)> callback) {
    onOpenProject = callback;
  }
  void setOnToggleWorkspaceCallback(std::function<void()> callback) {
    onToggleWorkspace = callback;
  }
  void setOnCreateShaderCallback(std::function<void(const std::string&, const std::string&)> callback) {
    onCreateShader = callback;
  }

private:
  std::function<void(const std::string&, const std::string&)> onCreateProject;
  std::function<void(const std::string&)> onOpenProject;
  std::function<void()> onToggleWorkspace;
  std::function<void(const std::string&, const std::string&)> onCreateShader;

  ProjectManager* projectManager = nullptr;

  bool showCreateProjectDialog = false;
  bool showCreateShaderDialog = false;
  char projectNameBuffer[256] = "";
  char shaderNameBuffer[256] = "";
  std::string selectedShaderLanguage = "glsl"; // "glsl" or "hlsl"
  int selectedShaderType = 0; // 0=vertex, 1=fragment, 2=compute

  void renderCreateProjectDialog();
  void renderCreateShaderDialog();
};
