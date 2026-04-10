#pragma once

#include <string>
#include <vector>
#include <filesystem>

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

private:
  std::string workspacePath;
  std::string selectedFile;
  bool isOpen = true;
  float sidebarWidth = 250.0f;

  void renderDirectory(const fs::path& path, int depth = 0);
  bool isHiddenFile(const fs::path& path) const;
};
