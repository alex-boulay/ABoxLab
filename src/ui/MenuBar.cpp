#include "MenuBar.hpp"
#include "FileDialog.hpp"
#include "imgui.h"
#include <cstring>
#include "src/project/ProjectManager.hpp"

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

void MenuBar::renderCreateShaderDialog() {
  if (ImGui::BeginPopupModal("Create Shader", &showCreateShaderDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
    const char* shaderStages[] = { "Vertex", "Fragment", "Compute" };
    const char* glslExtensions[] = { ".vert", ".frag", ".comp" };
    const char* hlslExtensions[] = { ".hlsl.vert", ".hlsl.frag", ".hlsl.comp" };

    bool isHLSL = (selectedShaderLanguage == "hlsl");
    const char** extensions = isHLSL ? hlslExtensions : glslExtensions;

    ImGui::Text("Language: %s", isHLSL ? "HLSL" : "GLSL");
    ImGui::Text("Stage: %s Shader", shaderStages[selectedShaderType]);
    ImGui::Separator();

    ImGui::Text("Enter shader name:");
    ImGui::InputText("##shader_name", shaderNameBuffer, sizeof(shaderNameBuffer));
    ImGui::TextDisabled("Extension %s will be added automatically", extensions[selectedShaderType]);

    ImGui::Separator();

    if (ImGui::Button("Create", ImVec2(120, 0))) {
      if (onCreateShader && strlen(shaderNameBuffer) > 0) {
        std::string shaderName = std::string(shaderNameBuffer);
        std::string shaderType;

        // Build shader type string: "glsl_vertex", "hlsl_fragment", etc.
        shaderType = selectedShaderLanguage + "_";
        if (selectedShaderType == 0) shaderType += "vertex";
        else if (selectedShaderType == 1) shaderType += "fragment";
        else if (selectedShaderType == 2) shaderType += "compute";

        onCreateShader(shaderName, shaderType);
      }
      showCreateShaderDialog = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      showCreateShaderDialog = false;
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

      // Skip menu if no project is open
      bool hasProject = projectManager && projectManager->hasActiveProject();
      if (hasProject && ImGui::BeginMenu("New Shader")) {
        if (ImGui::BeginMenu("GLSL")) {
          if (ImGui::MenuItem("Vertex Shader")) {
            showCreateShaderDialog = true;
            selectedShaderLanguage = "glsl";
            selectedShaderType = 0;
            memset(shaderNameBuffer, 0, sizeof(shaderNameBuffer));
          }
          if (ImGui::MenuItem("Fragment Shader")) {
            showCreateShaderDialog = true;
            selectedShaderLanguage = "glsl";
            selectedShaderType = 1;
            memset(shaderNameBuffer, 0, sizeof(shaderNameBuffer));
          }
          if (ImGui::MenuItem("Compute Shader")) {
            showCreateShaderDialog = true;
            selectedShaderLanguage = "glsl";
            selectedShaderType = 2;
            memset(shaderNameBuffer, 0, sizeof(shaderNameBuffer));
          }
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("HLSL")) {
          if (ImGui::MenuItem("Vertex Shader")) {
            showCreateShaderDialog = true;
            selectedShaderLanguage = "hlsl";
            selectedShaderType = 0;
            memset(shaderNameBuffer, 0, sizeof(shaderNameBuffer));
          }
          if (ImGui::MenuItem("Pixel Shader")) {
            showCreateShaderDialog = true;
            selectedShaderLanguage = "hlsl";
            selectedShaderType = 1;
            memset(shaderNameBuffer, 0, sizeof(shaderNameBuffer));
          }
          if (ImGui::MenuItem("Compute Shader")) {
            showCreateShaderDialog = true;
            selectedShaderLanguage = "hlsl";
            selectedShaderType = 2;
            memset(shaderNameBuffer, 0, sizeof(shaderNameBuffer));
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
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

  // Open popups after menu bar is closed
  if (showCreateProjectDialog && !ImGui::IsPopupOpen("Create Project")) {
    ImGui::OpenPopup("Create Project");
  }
  if (showCreateShaderDialog && !ImGui::IsPopupOpen("Create Shader")) {
    ImGui::OpenPopup("Create Shader");
  }

  // Render dialogs
  renderCreateProjectDialog();
  renderCreateShaderDialog();
}
