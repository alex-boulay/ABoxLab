#include "ABoxLabApp.hpp"
#include "utils/Logger.hpp"
#include "graphics/ShaderHandler.hpp"
#include <vulkan/vulkan_core.h>
#include "ABox/src/utils/Logger.hpp"
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
      .MinImageCount = 2,
      .ImageCount = static_cast<uint32_t>(dbe->swapchains.front().getImagesCount()),
      .PipelineInfoMain = {
          .RenderPass = dbe->rpm.front().get(),
          .Subpass = 0,
          .MSAASamples = VK_SAMPLE_COUNT_1_BIT
      }
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
  auto *dbe = rs.getMainDevice();
  uint32_t imageIndex;

  // Wait for previous frame to complete
  VkFence inFlightFence = dbe->getFrameSyncArray()->getFrameSyncObject()->inFlight;
  vkWaitForFences(dbe->getDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);
  vkResetFences(dbe->getDevice(), 1, &inFlightFence);

  // Acquire next swapchain image
  VkResult result = vkAcquireNextImageKHR(
      dbe->getDevice(),
      dbe->swapchains.front().getSwapchain(),
      UINT64_MAX,
      dbe->getFrameSyncArray()->getFrameSyncObject()->imageOk.get(),
      VK_NULL_HANDLE,
      &imageIndex
  );

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    LOG_WARN("Vulkan") << "Swapchain out of date";
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed to acquire swap chain image!");
  }

  uint32_t frameIndex = dbe->getFrameSyncArray()->getFrameIndex();
  VkCommandBuffer cmdBuffer = dbe->getCommandHandler()->top().getCommandBuffer(frameIndex);

  // Reset and begin command buffer
  vkResetCommandBuffer(cmdBuffer, 0);
  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pInheritanceInfo = nullptr
  };
  vkBeginCommandBuffer(cmdBuffer, &beginInfo);

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

  // End command buffer
  vkEndCommandBuffer(cmdBuffer);

  // Submit to graphics queue
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = dbe->getFrameSyncArray()->getFrameSyncObject()->imageOk.ptr(),
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmdBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = dbe->getFrameSyncArray()->getFrameSyncObject()->renderEnd.ptr()
  };

  VkResult submitResult = vkQueueSubmit(
      dbe->graphicsQueue,
      1,
      &submitInfo,
      dbe->getFrameSyncArray()->getFrameSyncObject()->inFlight
  );

  if (submitResult != VK_SUCCESS) {
    throw std::runtime_error("Failed to submit command buffer!");
  }

  // Present
  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = dbe->getFrameSyncArray()->getFrameSyncObject()->renderEnd.ptr(),
      .swapchainCount = 1,
      .pSwapchains = dbe->swapchains.front().swapchainPtr(),
      .pImageIndices = &imageIndex,
      .pResults = nullptr
  };

  vkQueuePresentKHR(dbe->presentQueue, &presentInfo);

  // Increment frame index for next frame
  dbe->getFrameSyncArray()->incrementFrameIndex();
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

    // Calculate editor offset based on workspace state
    float editorOffset = fileTree.isWorkspaceOpen() ? 250.0f : 30.0f;
    codeEditor.render(editorOffset);

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
  menuBar.setProjectManager(&projectManager);
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

  // Set up recent projects
  menuBar.setRecentProjects(&projectManager.getRecentProjects());
  menuBar.setOnOpenRecentProjectCallback([this](const std::string& projectFilePath) {
    if (projectManager.openProject(projectFilePath)) {
      fileTree.setWorkspacePath(projectManager.getActiveProject().path);
      LOG_INFO("App") << "Opened recent project: " << projectManager.getActiveProject().name;
    } else {
      LOG_INFO("App") << "Failed to open recent project at: " << projectFilePath;
    }
  });

  menuBar.setOnToggleWorkspaceCallback([this]() {
    fileTree.toggleOpen();
  });

  menuBar.setOnCreateShaderCallback([this](const std::string& name, const std::string& type) {
    if (!projectManager.hasActiveProject()) {
      LOG_INFO("App") << "No active project to create shader in";
      return;
    }

    std::string extension = FileTemplates::getExtensionForShaderType(type);
    std::string filename = name + extension;
    std::string content = FileTemplates::getTemplateForShaderType(type);

    if (projectManager.createFile(filename, content)) {
      LOG_INFO("App") << "Created shader: " << filename;
    } else {
      LOG_INFO("App") << "Failed to create shader: " << filename;
    }
  });

  fileTree.setOnFileClickedCallback([this](const std::string& filePath) {
    codeEditor.openFile(filePath);
    LOG_INFO("App") << "Opened file: " << filePath;
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
