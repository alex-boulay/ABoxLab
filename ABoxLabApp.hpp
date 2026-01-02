#ifndef ABOXLABAPP_HPP
#define ABOXLABAPP_HPP

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include "core/ResourcesManager.hpp"
#include "graphics/ShaderHandler.hpp"
#include "window/WindowManager.hpp"
#include <GLFW/glfw3.h>

// #include "ShaderHandler.hpp"
/**
 * @class ABoxApp
 * @brief Vulkan Loader application
 *
 */
class ABoxLabApp {
  static constexpr VkExtent2D baseWindowDimention = {.width = 800u,
                                                     .height = 600u};

  WindowManager wm{baseWindowDimention};
  ResourcesManager rs;
  ShaderHandler shaderHandler;

public:
  ABoxLabApp();
  ~ABoxLabApp();
  void run();
};

#endif // ! ABOXAPP_HPP
