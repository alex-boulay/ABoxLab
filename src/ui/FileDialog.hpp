#pragma once

#include <string>
#include <optional>

class FileDialog {
public:
  // Open a directory selection dialog
  // Returns the selected directory path, or empty if cancelled
  static std::optional<std::string> selectFolder(const std::string& title = "Select Folder");

  // Open a file selection dialog
  // Returns the selected file path, or empty if cancelled
  static std::optional<std::string> selectFile(const std::string& title = "Select File");

private:
  static bool commandExists(const std::string& command);
  static std::optional<std::string> executeCommand(const std::string& command);
};
