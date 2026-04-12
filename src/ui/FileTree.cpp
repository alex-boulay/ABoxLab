#include "FileTree.hpp"
#include "imgui.h"
#include <algorithm>

FileTree::FileTree() : workspacePath(""), selectedFile("") {}

void FileTree::setWorkspacePath(const std::string& path) {
  if (fs::exists(path) && fs::is_directory(path)) {
    workspacePath = path;
  }
}

bool FileTree::isHiddenFile(const fs::path& path) const {
  std::string filename = path.filename().string();
  // Skip hidden files (starting with .), build directories, etc.
  return filename[0] == '.' ||
         filename == "build" ||
         filename == "cmake-build-debug" ||
         filename == "cmake-build-release";
}

void FileTree::renderDirectory(const fs::path& path, int depth) {
  if (!fs::exists(path) || !fs::is_directory(path)) return;

  std::vector<fs::directory_entry> entries;
  for (const auto& entry : fs::directory_iterator(path)) {
    if (!isHiddenFile(entry.path())) {
      entries.push_back(entry);
    }
  }

  // Sort: directories first, then files, alphabetically
  std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
    if (a.is_directory() != b.is_directory()) {
      return a.is_directory();
    }
    return a.path().filename().string() < b.path().filename().string();
  });

  for (const auto& entry : entries) {
    const auto& entryPath = entry.path();
    std::string filename = entryPath.filename().string();

    if (entry.is_directory()) {
      // Use standard TreeNode for directories - this is the common approach
      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                 ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                 ImGuiTreeNodeFlags_SpanFullWidth;

      bool nodeOpen = ImGui::TreeNodeEx(filename.c_str(), flags);

      if (nodeOpen) {
        renderDirectory(entryPath, depth + 1);
        ImGui::TreePop();
      }
    } else {
      // Use Selectable for files - industry standard for proper alignment
      ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf |
                                      ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                      ImGuiTreeNodeFlags_SpanFullWidth;

      bool isSelected = (entryPath.string() == selectedFile);

      // TreeNodeEx just for the leaf icon, then Selectable for interaction
      ImGui::TreeNodeEx(filename.c_str(), node_flags);

      if (ImGui::IsItemClicked()) {
        selectedFile = entryPath.string();
        if (onFileClicked) {
          onFileClicked(selectedFile);
        }
      }
    }
  }
}

void FileTree::render() {
  ImGuiViewport* viewport = ImGui::GetMainViewport();

  if (!isOpen) {
    // Render collapsed tab on the left
    float buttonWidth = 30.0f;
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(buttonWidth, viewport->Size.y - ImGui::GetFrameHeight()));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("##WorkspaceCollapsed", nullptr, flags);

    // Square arrow button
    if (ImGui::Button(">", ImVec2(20, 20))) {
      isOpen = true;
    }

    ImGui::End();
    return;
  }

  // Fixed left sidebar
  ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + ImGui::GetFrameHeight())); // Below menu bar
  ImGui::SetNextWindowSize(ImVec2(sidebarWidth, viewport->Size.y - ImGui::GetFrameHeight()));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoTitleBar;

  ImGui::Begin("##Workspace", nullptr, flags);

  // Header with text on left, collapse button on right
  ImGui::Text("Workspace");

  // Right-align button using ImGui's layout system
  ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 20.0f);
  if (ImGui::Button("<", ImVec2(20, 20))) {
    isOpen = false;
  }
  ImGui::Separator();

  if (workspacePath.empty()) {
    ImGui::TextDisabled("No project opened");
    ImGui::TextWrapped("Create or open a project from the File menu");
  } else {
    // Collapsible header for the workspace root
    std::string workspaceName = fs::path(workspacePath).filename().string();
    if (workspaceName.empty()) {
      workspaceName = workspacePath; // Use full path if no filename
    }

    if (ImGui::CollapsingHeader(workspaceName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
      renderDirectory(workspacePath);
    }
  }

  ImGui::End();
}
