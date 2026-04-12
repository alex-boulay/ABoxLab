#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

class FileTree {
public:
  FileTree();
  ~FileTree() = default;

  void render();
  void setWorkspacePath(const std::string& path);
  const std::string& getWorkspacePath() const { return workspacePath; }
  const std::string& getSelectedFile() const { return selectedFile; }

  void toggleOpen() { isOpen = !isOpen; }
  bool isWorkspaceOpen() const { return isOpen; }

  // Callback when a file is clicked
  void setOnFileClickedCallback(std::function<void(const std::string&)> callback) {
    onFileClicked = callback;
  }

private:
  std::string workspacePath;
  std::string selectedFile;
  bool isOpen = true;
  float sidebarWidth = 250.0f;

  std::function<void(const std::string&)> onFileClicked;

  void renderDirectory(const fs::path& path, int depth = 0);
  bool isHiddenFile(const fs::path& path) const;
};
