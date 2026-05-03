#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <cstdint>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "imgui.h"
#include "imgui_impl_vulkan.h"

class Scene;
struct Mesh;

class SceneView {
public:
  SceneView() = default;
  ~SceneView();

  void init(VkDevice device, VkPhysicalDevice physicalDevice,
            VkQueue graphicsQueue, uint32_t queueFamily,
            const std::string& vertSpvPath, const std::string& fragSpvPath);
  void cleanup();

  void setScene(Scene* scene) { this->scene = scene; }

  // Hot-reload: swap pipeline shaders or mesh at runtime
  void reloadPipeline(const std::string& vertSpvPath, const std::string& fragSpvPath);
  void setMesh(int primitiveType); // 0=Quad, 1=Cube, 2=Sphere

  // Record offscreen render pass into the given command buffer
  void recordCommands(VkCommandBuffer cmdBuffer);

  // Display the viewport texture in ImGui (handles camera input)
  void renderImGui();

  bool isInitialized() const { return initialized; }

private:
  bool initialized = false;

  VkDevice device = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  uint32_t queueFamily = 0;

  Scene* scene = nullptr;

  // Offscreen render target
  VkImage colorImage = VK_NULL_HANDLE;
  VkDeviceMemory colorMemory = VK_NULL_HANDLE;
  VkImageView colorImageView = VK_NULL_HANDLE;
  VkRenderPass renderPass = VK_NULL_HANDLE;
  VkFramebuffer framebuffer = VK_NULL_HANDLE;

  // ImGui texture handle
  VkDescriptorSet imguiTextureId = VK_NULL_HANDLE;

  uint32_t width = 800;
  uint32_t height = 600;

  // Graphics pipeline
  VkPipeline pipeline = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

  // Mesh GPU buffers
  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexMemory = VK_NULL_HANDLE;
  uint32_t indexCount = 0;

  // Orbit camera
  float orbitYaw = 30.0f;
  float orbitPitch = 20.0f;
  float orbitDistance = 4.0f;
  glm::vec3 panOffset = glm::vec3(0.0f);

  // Cached matrices
  glm::mat4 viewMatrix = glm::mat4(1.0f);
  glm::mat4 projMatrix = glm::mat4(1.0f);

  // Command pool for one-shot uploads
  VkCommandPool commandPool = VK_NULL_HANDLE;

  void createRenderPass();
  void createColorTarget();
  void createFramebuffer();
  void registerImGuiTexture();
  void createPipeline(const std::string& vertSpvPath, const std::string& fragSpvPath);
  void uploadMesh(const Mesh& mesh);
  void updateCamera();
  void handleCameraInput();

  VkShaderModule createShaderModule(const std::vector<uint32_t>& spirv);
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer& buffer, VkDeviceMemory& memory);
  void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
