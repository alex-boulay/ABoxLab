#include "ABoxLabApp.hpp"
#include "utils/Logger.hpp"
#include "graphics/ShaderHandler.hpp"
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstring>

void ABoxLabApp::run() {
  while (!wm.shouldClose()) {
    wm.pollEvents();
    rs.drawFrame();
    if (wm.consumeFramebufferResized()) {
      rs.waitIdle();
      LOG_INFO("App") << "recreating swapchain";
      LOG_INFO("App") << "value: "
                      << static_cast<int32_t>(rs.reCreateSwapchain(
                             wm.getWidth(), wm.getHeight()));
    }
  }
  rs.waitIdle();
}

ABoxLabApp::ABoxLabApp() {
  // TODO Ressource Manager should just ask for a GraphicsPipeline or Compute
  // and behave accordingly
  rs.getDeviceHandler()->listPhysicalDevices();
  LOG_INFO("App") << "\n --Physical Device Listed --";
  wm.createSurface(rs);
  LOG_INFO("App") << "\n -- Application Display Created --";
  rs.addLogicalDevice();
  LOG_INFO("App") << "\n -- Logical Device added --";
  rs.createSwapchain(wm.getWidth(), wm.getHeight());
  LOG_INFO("App") << "\n -- Swapchain Created --";
  rs.addGraphicsPipeline(shaderHandler.getShaderHandlers());
  LOG_INFO("App") << "\n -- Graphics Pipeline added --";
  rs.createFramebuffers();
  LOG_INFO("App") << "\n -- Frame Buffers Created --";
}

ABoxLabApp::~ABoxLabApp() { glfwTerminate(); };
