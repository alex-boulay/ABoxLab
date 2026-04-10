#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include <string>

class MenuBar {
public:
  MenuBar() = default;
  ~MenuBar() = default;

  void render(GLFWwindow* window);

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

private:
  std::function<void(const std::string&, const std::string&)> onCreateProject;
  std::function<void(const std::string&)> onOpenProject;
  std::function<void()> onToggleWorkspace;

  bool showCreateProjectDialog = false;
  char projectNameBuffer[256] = "";

  void renderCreateProjectDialog();
};
