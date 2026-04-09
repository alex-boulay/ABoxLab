#include "ABoxLabApp.hpp"
#include "utils/Logger.hpp"
#include "graphics/ShaderHandler.hpp"
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstring>
#include <array>

void ABoxLabApp::initImGui() {
  auto *dbe = rs.getMainDevice();
  VkDevice device = dbe->getDevice().get();

  // Create descriptor pool for ImGui
  std::array<VkDescriptorPoolSize, 11> poolSizes = {{
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
  }};

  VkDescriptorPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = 1000,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()
  };

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create ImGui descriptor pool!");
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigWindowsMoveFromTitleBarOnly = true;  // Only move from title bar

  // Setup ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForVulkan(wm.getWindow(), true);

  ImGui_ImplVulkan_InitInfo initInfo{
      .Instance = rs.getInstance(),
      .PhysicalDevice = dbe->getPhysicalDevice(),
      .Device = device,
      .QueueFamily = dbe->getFamilyQueueIndices().at(QueueRole::Graphics),
      .Queue = dbe->graphicsQueue,
      .DescriptorPool = imguiDescriptorPool,
      .RenderPass = dbe->rpm.front().get(),
      .MinImageCount = 2,
      .ImageCount = static_cast<uint32_t>(dbe->swapchains.front().getImagesCount()),
      .MSAASamples = VK_SAMPLE_COUNT_1_BIT
  };

  ImGui_ImplVulkan_Init(&initInfo);

  LOG_INFO("App") << "ImGui initialized successfully";
}

void ABoxLabApp::cleanupImGui() {
  // Make sure device is idle before cleanup
  if (imguiDescriptorPool != VK_NULL_HANDLE) {
    auto *dbe = rs.getMainDevice();
    VkDevice device = dbe->getDevice().get();

    vkDeviceWaitIdle(device);

    // Shutdown ImGui backends
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Destroy ImGui's descriptor pool
    vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);
    imguiDescriptorPool = VK_NULL_HANDLE;
  }
}

void ABoxLabApp::renderFrame() {
  uint32_t imageIndex;

  // Begin frame using ABox - get command buffer
  VkCommandBuffer cmdBuffer = rs.beginFrame(&imageIndex);

  if (cmdBuffer == VK_NULL_HANDLE) {
    // Swapchain out of date, skip this frame
    return;
  }

  auto *dbe = rs.getMainDevice();

  // Begin render pass
  VkClearValue clearColor = {.color = {.float32 = {0.1f, 0.1f, 0.1f, 1.0f}}};

  VkRenderPassBeginInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = dbe->rpm.front().get(),
      .framebuffer = dbe->fbb.getFrameBuffers(
          dbe->swapchains.front().getSwapchain(),
          dbe->rpm.front().get()
      )->at(imageIndex).get(),
      .renderArea = {
          .offset = {0, 0},
          .extent = dbe->swapchains.front().getExtent()
      },
      .clearValueCount = 1,
      .pClearValues = &clearColor
  };

  vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  // Record ImGui draw data
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);

  // End render pass
  vkCmdEndRenderPass(cmdBuffer);

  // Submit and present using ABox
  rs.endFrame(imageIndex, cmdBuffer);
}

void ABoxLabApp::run() {
  while (!wm.shouldClose()) {
    wm.pollEvents();

    // Start ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Example ImGui UI
    // Keep window inside viewport
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImGui::GetIO().DisplaySize);

    ImGui::Begin("ABoxLab");
    ImGui::Text("Shader Testing Tool");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    // Clamp window position to stay in viewport
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    if (windowPos.x < 0) ImGui::SetWindowPos(ImVec2(0, windowPos.y));
    if (windowPos.y < 0) ImGui::SetWindowPos(ImVec2(windowPos.x, 0));
    if (windowPos.x + windowSize.x > displaySize.x)
        ImGui::SetWindowPos(ImVec2(displaySize.x - windowSize.x, windowPos.y));
    if (windowPos.y + windowSize.y > displaySize.y)
        ImGui::SetWindowPos(ImVec2(windowPos.x, displaySize.y - windowSize.y));

    ImGui::End();

    // Render
    ImGui::Render();
    renderFrame();

    if (wm.consumeFramebufferResized()) {
      rs.waitIdle();
      LOG_INFO("App") << "recreating swapchain";
      rs.reCreateSwapchain(wm.getWidth(), wm.getHeight());
    }
  }
  rs.waitIdle();
}

ABoxLabApp::ABoxLabApp() {
  rs.getDeviceHandler()->listPhysicalDevices();
  LOG_INFO("App") << "\n --Physical Device Listed --";
  wm.createSurface(rs);
  LOG_INFO("App") << "\n -- Application Display Created --";
  rs.addLogicalDevice();
  LOG_INFO("App") << "\n -- Logical Device added --";
  rs.createSwapchain(wm.getWidth(), wm.getHeight());
  LOG_INFO("App") << "\n -- Swapchain Created --";

  // Create render pass for ImGui
  rs.createRenderPass();
  LOG_INFO("App") << "\n -- Render Pass Created --";

  // Create framebuffers (without pipeline - just for render pass)
  auto *dbe = rs.getMainDevice();
  dbe->fbb.createFramebuffers(
      dbe->getDevice(),
      dbe->rpm.front().get(),
      &dbe->swapchains.front()
  );
  LOG_INFO("App") << "\n -- Frame Buffers Created --";

  // Initialize ImGui
  initImGui();
  LOG_INFO("App") << "\n -- ImGui Initialized --";
}

ABoxLabApp::~ABoxLabApp() {
  rs.waitIdle();
  cleanupImGui();
  glfwTerminate();
};
