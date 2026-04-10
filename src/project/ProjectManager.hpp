#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

struct Project {
  std::string name;
  std::string path;
  std::string configPath; // Path to .aboxlab project file
};

class ProjectManager {
public:
  ProjectManager();
  ~ProjectManager();

  // Project operations
  bool createProject(const std::string& name, const std::string& path);
  bool openProject(const std::string& projectPath);
  void closeProject();

  // Getters
  bool hasActiveProject() const { return activeProject.has_value(); }
  const Project& getActiveProject() const { return activeProject.value(); }
  const std::vector<std::string>& getRecentProjects() const { return recentProjects; }

  // Config directory
  static std::string getConfigDir();
  static std::string getRecentProjectsFile();

private:
  std::optional<Project> activeProject;
  std::vector<std::string> recentProjects;

  void loadRecentProjects();
  void saveRecentProjects();
  void addToRecentProjects(const std::string& projectPath);

  bool saveProjectFile(const Project& project);
  bool loadProjectFile(const std::string& path, Project& project);
};
