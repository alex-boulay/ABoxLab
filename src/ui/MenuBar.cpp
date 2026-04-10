#include "MenuBar.hpp"
#include "imgui.h"

void MenuBar::render(GLFWwindow* window) {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Close", "Alt+F4")) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
      }
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}
