#pragma once

#include <string>
#include <vector>

class CodeEditor {
public:
  CodeEditor();
  ~CodeEditor() = default;

  void render(float offsetX);
  void openFile(const std::string& filePath);
  void saveFile();
  void closeFile();

  bool hasOpenFile() const { return !currentFilePath.empty(); }
  const std::string& getCurrentFilePath() const { return currentFilePath; }
  bool isModified() const { return modified; }

private:
  std::string currentFilePath;
  std::vector<char> textBuffer;
  bool modified = false;
  static const size_t MAX_FILE_SIZE = 1024 * 1024; // 1MB max

  void loadFileContent(const std::string& filePath);
};
