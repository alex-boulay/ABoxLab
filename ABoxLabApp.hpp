#ifndef ABOXLABAPP_HPP
#define ABOXLABAPP_HPP

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include "core/ResourcesManager.hpp"
#include "graphics/ShaderHandler.hpp"
#include "window/WindowManager.hpp"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "src/ui/MenuBar.hpp"
#include "src/ui/FileTree.hpp"
#include "src/ui/CodeEditor.hpp"
#include "src/project/ProjectManager.hpp"
#include "src/project/FileTemplates.hpp"
#include "src/renderer/SceneView.hpp"

// #include "ShaderHandler.hpp"
/**
 * @class ABoxApp
 * @brief Vulkan Loader application
 *
 */
class ABoxLabApp {
  static constexpr VkExtent2D baseWindowDimention = {.width = 800u,
                                                     .height = 600u};

  // IMPORTANT: Member order determines destruction order (reverse of declaration)
  // wm must be destroyed LAST because rs needs the GLFW/Wayland connection
  // to properly destroy swapchains
  WindowManager wm{baseWindowDimention, "ABoxLab"};
  ShaderHandler shaderHandler;
  ResourcesManager rs;

  VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
  VkDevice imguiDevice = VK_NULL_HANDLE;
  MenuBar menuBar;
  FileTree fileTree;
  CodeEditor codeEditor;
  SceneView sceneView;
  ProjectManager projectManager;

  bool layoutRefreshed = false;

  void initImGui();
  void cleanupImGui();
  void renderFrame();
  void triggerLayoutRefresh();

public:
  ABoxLabApp();
  ~ABoxLabApp();
  void run();
};

#endif // ! ABOXAPP_HPP
