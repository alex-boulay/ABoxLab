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

  // Store device for cleanup (avoids accessing rs during destruction)
  imguiDevice = device;

  LOG_INFO("App") << "ImGui initialized successfully";
}

void ABoxLabApp::cleanupImGui() {
  if (imguiDevice == VK_NULL_HANDLE) {
    return; // Not initialized
  }

  // Wait for device to finish before cleanup
  vkDeviceWaitIdle(imguiDevice);

  // Shutdown ImGui backends first (before destroying descriptor pool)
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  // Destroy ImGui's descriptor pool
  if (imguiDescriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(imguiDevice, imguiDescriptorPool, nullptr);
    imguiDescriptorPool = VK_NULL_HANDLE;
  }

  imguiDevice = VK_NULL_HANDLE;
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

void ABoxLabApp::triggerLayoutRefresh() {
  int width, height;
  glfwGetWindowSize(wm.getWindow(), &width, &height);
  glfwSetWindowSize(wm.getWindow(), width + 1, height);
  glfwSetWindowSize(wm.getWindow(), width, height);
}

void ABoxLabApp::run() {
  while (!wm.shouldClose()) {
    wm.pollEvents();

    // Fix ImGui layout on first frame (one-shot logic)
    if (!layoutRefreshed && (layoutRefreshed = true)) {
      triggerLayoutRefresh();
    }

    // Start ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Render UI components
    menuBar.render(wm.getWindow());
    fileTree.render();

    // Handle keyboard shortcuts after rendering (but before ImGui::Render)
    // This ensures shortcuts work regardless of which window has focus
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard || !io.WantTextInput) {
      if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_B, false)) {
        fileTree.toggleOpen();
      }
    }

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

  // Set up menu callbacks
  menuBar.setOnCreateProjectCallback([this](const std::string& name, const std::string& path) {
    if (projectManager.createProject(name, path)) {
      fileTree.setWorkspacePath(path);
      LOG_INFO("App") << "Created project: " << name << " at " << path;
    } else {
      LOG_INFO("App") << "Failed to create project: " << name;
    }
  });

  menuBar.setOnOpenProjectCallback([this](const std::string& path) {
    if (projectManager.openProject(path)) {
      fileTree.setWorkspacePath(projectManager.getActiveProject().path);
      LOG_INFO("App") << "Opened project: " << projectManager.getActiveProject().name;
    } else {
      LOG_INFO("App") << "Failed to open project at: " << path;
    }
  });

  menuBar.setOnToggleWorkspaceCallback([this]() {
    fileTree.toggleOpen();
  });
}

ABoxLabApp::~ABoxLabApp() {
  // Clean up ImGui first, before ABox resources are destroyed
  cleanupImGui();

  // NOTE: glfwTerminate() is NOT called here!
  // The Wayland/X11 connection must remain valid while Vulkan
  // swapchains are being destroyed (during rs destructor).
  // GLFW will clean up automatically at process exit.
};
