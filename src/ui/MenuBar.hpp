#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include <string>
#include <vector>

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
  void setOnOpenRecentProjectCallback(std::function<void(const std::string&)> callback) {
    onOpenRecentProject = callback;
  }
  void setRecentProjects(const std::vector<std::string>* recent) { recentProjects = recent; }

private:
  std::function<void(const std::string&, const std::string&)> onCreateProject;
  std::function<void(const std::string&)> onOpenProject;
  std::function<void()> onToggleWorkspace;
  std::function<void(const std::string&, const std::string&)> onCreateShader;
  std::function<void(const std::string&)> onOpenRecentProject;

  ProjectManager* projectManager = nullptr;
  const std::vector<std::string>* recentProjects = nullptr;

  bool showCreateProjectDialog = false;
  bool showCreateShaderDialog = false;
  char projectNameBuffer[256] = "";
  char shaderNameBuffer[256] = "";
  std::string selectedShaderLanguage = "glsl"; // "glsl" or "hlsl"
  int selectedShaderType = 0; // 0=vertex, 1=fragment, 2=compute

  void renderCreateProjectDialog();
  void renderCreateShaderDialog();
};
