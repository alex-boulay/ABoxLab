#include "ProjectManager.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>

ProjectManager::ProjectManager() {
  // Ensure config directory exists
  fs::path configDir = getConfigDir();
  if (!fs::exists(configDir)) {
    fs::create_directories(configDir);
  }

  loadRecentProjects();
}

ProjectManager::~ProjectManager() {
  saveRecentProjects();
}

std::string ProjectManager::getConfigDir() {
#ifdef _WIN32
  const char* appdata = std::getenv("APPDATA");
  if (appdata) {
    return std::string(appdata) + "/aboxlab";
  }
  return "";
#elif __APPLE__
  const char* home = std::getenv("HOME");
  if (home) {
    return std::string(home) + "/Library/Application Support/aboxlab";
  }
  return "";
#else
  // Linux/Unix
  const char* configHome = std::getenv("XDG_CONFIG_HOME");
  if (configHome) {
    return std::string(configHome) + "/aboxlab";
  }
  const char* home = std::getenv("HOME");
  if (home) {
    return std::string(home) + "/.config/aboxlab";
  }
  return "";
#endif
}

std::string ProjectManager::getRecentProjectsFile() {
  return getConfigDir() + "/recent_projects.txt";
}

bool ProjectManager::createProject(const std::string& name, const std::string& path) {
  Project project;
  project.name = name;
  project.path = path;
  project.configPath = path + "/.aboxlab";

  // Create project directory if it doesn't exist
  if (!fs::exists(path)) {
    fs::create_directories(path);
  }

  // Save project file
  if (!saveProjectFile(project)) {
    return false;
  }

  activeProject = project;
  addToRecentProjects(project.configPath);
  return true;
}

bool ProjectManager::openProject(const std::string& projectPath) {
  Project project;

  // If projectPath is a directory, look for .aboxlab file
  std::string configPath = projectPath;
  if (fs::is_directory(projectPath)) {
    configPath = projectPath + "/.aboxlab";
  }

  if (!fs::exists(configPath)) {
    return false;
  }

  if (!loadProjectFile(configPath, project)) {
    return false;
  }

  activeProject = project;
  addToRecentProjects(configPath);
  return true;
}

void ProjectManager::closeProject() {
  activeProject.reset();
}

bool ProjectManager::saveProjectFile(const Project& project) {
  std::ofstream file(project.configPath);
  if (!file) {
    return false;
  }

  file << "name=" << project.name << "\n";
  file << "path=" << project.path << "\n";
  file.close();
  return true;
}

bool ProjectManager::loadProjectFile(const std::string& path, Project& project) {
  std::ifstream file(path);
  if (!file) {
    return false;
  }

  project.configPath = path;

  std::string line;
  while (std::getline(file, line)) {
    size_t pos = line.find('=');
    if (pos != std::string::npos) {
      std::string key = line.substr(0, pos);
      std::string value = line.substr(pos + 1);

      if (key == "name") {
        project.name = value;
      } else if (key == "path") {
        project.path = value;
      }
    }
  }

  file.close();
  return !project.name.empty() && !project.path.empty();
}

void ProjectManager::loadRecentProjects() {
  std::string recentFile = getRecentProjectsFile();
  if (!fs::exists(recentFile)) {
    return;
  }

  std::ifstream file(recentFile);
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty() && fs::exists(line)) {
      recentProjects.push_back(line);
    }
  }
  file.close();
}

void ProjectManager::saveRecentProjects() {
  std::ofstream file(getRecentProjectsFile());
  for (const auto& path : recentProjects) {
    file << path << "\n";
  }
  file.close();
}

void ProjectManager::addToRecentProjects(const std::string& projectPath) {
  // Remove if already exists
  auto it = std::find(recentProjects.begin(), recentProjects.end(), projectPath);
  if (it != recentProjects.end()) {
    recentProjects.erase(it);
  }

  // Add to front
  recentProjects.insert(recentProjects.begin(), projectPath);

  // Keep only last 10
  if (recentProjects.size() > 10) {
    recentProjects.resize(10);
  }

  saveRecentProjects();
}
