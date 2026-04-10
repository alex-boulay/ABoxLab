#include "FileDialog.hpp"
#include <array>
#include <memory>
#include <cstdio>
#include <algorithm>

bool FileDialog::commandExists(const std::string& command) {
  std::string checkCmd = "command -v " + command + " > /dev/null 2>&1";
  return system(checkCmd.c_str()) == 0;
}

std::optional<std::string> FileDialog::executeCommand(const std::string& command) {
  std::array<char, 256> buffer;
  std::string result;

  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
  if (!pipe) {
    return std::nullopt;
  }

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  // Remove trailing newline
  if (!result.empty() && result.back() == '\n') {
    result.pop_back();
  }

  return result.empty() ? std::nullopt : std::optional<std::string>(result);
}

std::optional<std::string> FileDialog::selectFolder(const std::string& title) {
#ifdef __linux__
  // Try zenity (GTK) first
  if (commandExists("zenity")) {
    std::string cmd = "zenity --file-selection --directory --title=\"" + title + "\" 2>/dev/null";
    return executeCommand(cmd);
  }

  // Try kdialog (KDE) second
  if (commandExists("kdialog")) {
    std::string cmd = "kdialog --getexistingdirectory . --title \"" + title + "\" 2>/dev/null";
    return executeCommand(cmd);
  }

  // Fallback: use yad if available
  if (commandExists("yad")) {
    std::string cmd = "yad --file --directory --title=\"" + title + "\" 2>/dev/null";
    return executeCommand(cmd);
  }
#elif _WIN32
  // Windows file dialog would go here
  // Could use Windows API or PowerShell
#elif __APPLE__
  // macOS file dialog
  std::string cmd = "osascript -e 'POSIX path of (choose folder with prompt \"" + title + "\")'";
  return executeCommand(cmd);
#endif

  return std::nullopt;
}

std::optional<std::string> FileDialog::selectFile(const std::string& title) {
#ifdef __linux__
  // Try zenity (GTK) first
  if (commandExists("zenity")) {
    std::string cmd = "zenity --file-selection --title=\"" + title + "\" 2>/dev/null";
    return executeCommand(cmd);
  }

  // Try kdialog (KDE) second
  if (commandExists("kdialog")) {
    std::string cmd = "kdialog --getopenfilename . --title \"" + title + "\" 2>/dev/null";
    return executeCommand(cmd);
  }
#elif _WIN32
  // Windows file dialog
#elif __APPLE__
  // macOS file dialog
  std::string cmd = "osascript -e 'POSIX path of (choose file with prompt \"" + title + "\")'";
  return executeCommand(cmd);
#endif

  return std::nullopt;
}
