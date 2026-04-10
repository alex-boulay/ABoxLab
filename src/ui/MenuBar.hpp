#pragma once

#include <GLFW/glfw3.h>

class MenuBar {
public:
  MenuBar() = default;
  ~MenuBar() = default;

  void render(GLFWwindow* window);

private:
  bool showFileMenu = false;
};
