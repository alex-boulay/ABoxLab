#include "MenuBar.hpp"
#include "FileDialog.hpp"
#include "imgui.h"
#include <cstring>

void MenuBar::renderCreateProjectDialog() {
  if (ImGui::BeginPopupModal("Create Project", &showCreateProjectDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Enter project name:");
    ImGui::InputText("##project_name", projectNameBuffer, sizeof(projectNameBuffer));

    ImGui::Separator();

    if (ImGui::Button("Create", ImVec2(120, 0))) {
      if (onCreateProject && strlen(projectNameBuffer) > 0) {
        // Open file dialog to select directory
        auto folderPath = FileDialog::selectFolder("Select Project Location");
        if (folderPath.has_value()) {
          std::string fullPath = folderPath.value() + "/" + std::string(projectNameBuffer);
          onCreateProject(std::string(projectNameBuffer), fullPath);
        }
      }
      showCreateProjectDialog = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      showCreateProjectDialog = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void MenuBar::render(GLFWwindow* window) {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Project...")) {
        showCreateProjectDialog = true;
        memset(projectNameBuffer, 0, sizeof(projectNameBuffer));
      }
      if (ImGui::MenuItem("Open Project...")) {
        if (onOpenProject) {
          auto folderPath = FileDialog::selectFolder("Open Project");
          if (folderPath.has_value()) {
            onOpenProject(folderPath.value());
          }
        }
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit", "Alt+F4")) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Toggle Workspace", "Ctrl+B")) {
        if (onToggleWorkspace) {
          onToggleWorkspace();
        }
      }
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  // Open popup after menu bar is closed
  if (showCreateProjectDialog && !ImGui::IsPopupOpen("Create Project")) {
    ImGui::OpenPopup("Create Project");
  }

  // Render dialogs
  renderCreateProjectDialog();
}
