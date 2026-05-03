#include "SceneView.hpp"
#include "src/scene/Scene.hpp"
#include "src/scene/Mesh.hpp"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <array>

#include <glm/gtc/matrix_transform.hpp>
#include "ImGuizmo.h"

SceneView::~SceneView() {
  cleanup();
}

void SceneView::init(VkDevice device, VkPhysicalDevice physicalDevice,
                     VkQueue graphicsQueue, uint32_t queueFamily,
                     const std::string& vertSpvPath, const std::string& fragSpvPath) {
  this->device = device;
  this->physicalDevice = physicalDevice;
  this->graphicsQueue = graphicsQueue;
  this->queueFamily = queueFamily;

  // Command pool for buffer uploads
  VkCommandPoolCreateInfo poolInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
    .queueFamilyIndex = queueFamily,
  };
  vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);

  createRenderPass();
  createColorTarget();
  createFramebuffer();
  registerImGuiTexture();
  createPipeline(vertSpvPath, fragSpvPath);

  // Upload default cube mesh
  Mesh cube = Mesh::createCube();
  uploadMesh(cube);

  initialized = true;
  std::cerr << "[SceneView] Initialized " << width << "x" << height << std::endl;
}

void SceneView::cleanup() {
  if (!device) return;
  vkDeviceWaitIdle(device);

  if (indexBuffer) { vkDestroyBuffer(device, indexBuffer, nullptr); indexBuffer = VK_NULL_HANDLE; }
  if (indexMemory) { vkFreeMemory(device, indexMemory, nullptr); indexMemory = VK_NULL_HANDLE; }
  if (vertexBuffer) { vkDestroyBuffer(device, vertexBuffer, nullptr); vertexBuffer = VK_NULL_HANDLE; }
  if (vertexMemory) { vkFreeMemory(device, vertexMemory, nullptr); vertexMemory = VK_NULL_HANDLE; }
  if (pipeline) { vkDestroyPipeline(device, pipeline, nullptr); pipeline = VK_NULL_HANDLE; }
  if (pipelineLayout) { vkDestroyPipelineLayout(device, pipelineLayout, nullptr); pipelineLayout = VK_NULL_HANDLE; }
  if (commandPool) { vkDestroyCommandPool(device, commandPool, nullptr); commandPool = VK_NULL_HANDLE; }
  if (imguiTextureId) { ImGui_ImplVulkan_RemoveTexture(imguiTextureId); imguiTextureId = VK_NULL_HANDLE; }
  if (framebuffer) { vkDestroyFramebuffer(device, framebuffer, nullptr); framebuffer = VK_NULL_HANDLE; }
  if (colorImageView) { vkDestroyImageView(device, colorImageView, nullptr); colorImageView = VK_NULL_HANDLE; }
  if (colorImage) { vkDestroyImage(device, colorImage, nullptr); colorImage = VK_NULL_HANDLE; }
  if (colorMemory) { vkFreeMemory(device, colorMemory, nullptr); colorMemory = VK_NULL_HANDLE; }
  if (renderPass) { vkDestroyRenderPass(device, renderPass, nullptr); renderPass = VK_NULL_HANDLE; }

  initialized = false;
}

// ---- Render target ----

void SceneView::createRenderPass() {
  VkAttachmentDescription colorAttachment{
    .format = VK_FORMAT_R8G8B8A8_UNORM,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };

  VkAttachmentReference colorRef{
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass{
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorRef,
  };

  VkSubpassDependency dependency{
    .srcSubpass = 0,
    .dstSubpass = VK_SUBPASS_EXTERNAL,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
  };

  VkRenderPassCreateInfo rpInfo{
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &colorAttachment,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency,
  };

  if (vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create offscreen render pass");
  }
}

void SceneView::createColorTarget() {
  VkImageCreateInfo imageInfo{
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = VK_FORMAT_R8G8B8A8_UNORM,
    .extent = { width, height, 1 },
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  if (vkCreateImage(device, &imageInfo, nullptr, &colorImage) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create offscreen color image");
  }

  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(device, colorImage, &memReqs);

  VkMemoryAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memReqs.size,
    .memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
  };

  if (vkAllocateMemory(device, &allocInfo, nullptr, &colorMemory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate offscreen image memory");
  }

  vkBindImageMemory(device, colorImage, colorMemory, 0);

  VkImageViewCreateInfo viewInfo{
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = colorImage,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = VK_FORMAT_R8G8B8A8_UNORM,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
  };

  if (vkCreateImageView(device, &viewInfo, nullptr, &colorImageView) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create offscreen image view");
  }
}

void SceneView::createFramebuffer() {
  VkFramebufferCreateInfo fbInfo{
    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
    .renderPass = renderPass,
    .attachmentCount = 1,
    .pAttachments = &colorImageView,
    .width = width,
    .height = height,
    .layers = 1,
  };

  if (vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create offscreen framebuffer");
  }
}

void SceneView::registerImGuiTexture() {
  imguiTextureId = ImGui_ImplVulkan_AddTexture(
      colorImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

// ---- Pipeline ----

VkShaderModule SceneView::createShaderModule(const std::vector<uint32_t>& spirv) {
  VkShaderModuleCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = spirv.size() * sizeof(uint32_t),
    .pCode = spirv.data(),
  };

  VkShaderModule module;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader module");
  }
  return module;
}

static std::vector<uint32_t> loadSpirv(const std::string& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open SPIR-V file: " + path);
  }
  size_t size = file.tellg();
  file.seekg(0);
  std::vector<uint32_t> data(size / sizeof(uint32_t));
  file.read(reinterpret_cast<char*>(data.data()), size);
  return data;
}

void SceneView::createPipeline(const std::string& vertSpvPath, const std::string& fragSpvPath) {
  auto vertSpirv = loadSpirv(vertSpvPath);
  auto fragSpirv = loadSpirv(fragSpvPath);

  VkShaderModule vertModule = createShaderModule(vertSpirv);
  VkShaderModule fragModule = createShaderModule(fragSpirv);

  std::array<VkPipelineShaderStageCreateInfo, 2> stages = {{
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertModule,
      .pName = "main",
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragModule,
      .pName = "main",
    },
  }};

  // Vertex input: matches Vertex struct (pos vec3, normal vec3, uv vec2)
  VkVertexInputBindingDescription binding{
    .binding = 0,
    .stride = sizeof(float) * 8, // 3+3+2
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  std::array<VkVertexInputAttributeDescription, 3> attributes = {{
    { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT,    .offset = 0 },
    { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT,    .offset = sizeof(float) * 3 },
    { .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT,       .offset = sizeof(float) * 6 },
  }};

  VkPipelineVertexInputStateCreateInfo vertexInput{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &binding,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size()),
    .pVertexAttributeDescriptions = attributes.data(),
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  VkViewport viewport{
    .x = 0, .y = 0,
    .width = static_cast<float>(width),
    .height = static_cast<float>(height),
    .minDepth = 0.0f, .maxDepth = 1.0f,
  };

  VkRect2D scissor{ .offset = {0, 0}, .extent = { width, height } };

  VkPipelineViewportStateCreateInfo viewportState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };

  VkPipelineRasterizationStateCreateInfo rasterizer{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .lineWidth = 1.0f,
  };

  VkPipelineMultisampleStateCreateInfo multisampling{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = VK_FALSE,
  };

  VkPipelineColorBlendAttachmentState colorBlendAttachment{
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  VkPipelineColorBlendStateCreateInfo colorBlend{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
  };

  // Push constant for MVP matrix
  VkPushConstantRange pushConstantRange{
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .offset = 0,
    .size = sizeof(glm::mat4),
  };

  VkPipelineLayoutCreateInfo layoutInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges = &pushConstantRange,
  };

  if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = static_cast<uint32_t>(stages.size()),
    .pStages = stages.data(),
    .pVertexInputState = &vertexInput,
    .pInputAssemblyState = &inputAssembly,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pColorBlendState = &colorBlend,
    .layout = pipelineLayout,
    .renderPass = renderPass,
    .subpass = 0,
  };

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                 nullptr, &pipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create graphics pipeline");
  }

  std::cerr << "[SceneView] Pipeline created successfully" << std::endl;

  vkDestroyShaderModule(device, vertModule, nullptr);
  vkDestroyShaderModule(device, fragModule, nullptr);
}

// ---- GPU Buffers ----

void SceneView::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags properties,
                             VkBuffer& buffer, VkDeviceMemory& memory) {
  VkBufferCreateInfo bufferInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create buffer");
  }

  VkMemoryRequirements memReqs;
  vkGetBufferMemoryRequirements(device, buffer, &memReqs);

  VkMemoryAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memReqs.size,
    .memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, properties),
  };

  if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate buffer memory");
  }

  vkBindBufferMemory(device, buffer, memory, 0);
}

void SceneView::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
  VkCommandBufferAllocateInfo allocInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };

  VkCommandBuffer cmdBuf;
  vkAllocateCommandBuffers(device, &allocInfo, &cmdBuf);

  VkCommandBufferBeginInfo beginInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(cmdBuf, &beginInfo);

  VkBufferCopy copyRegion{ .size = size };
  vkCmdCopyBuffer(cmdBuf, src, dst, 1, &copyRegion);

  vkEndCommandBuffer(cmdBuf);

  VkSubmitInfo submitInfo{
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &cmdBuf,
  };
  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  vkFreeCommandBuffers(device, commandPool, 1, &cmdBuf);
}

void SceneView::uploadMesh(const Mesh& mesh) {
  // Clean up old buffers
  if (vertexBuffer) { vkDestroyBuffer(device, vertexBuffer, nullptr); vertexBuffer = VK_NULL_HANDLE; }
  if (vertexMemory) { vkFreeMemory(device, vertexMemory, nullptr); vertexMemory = VK_NULL_HANDLE; }
  if (indexBuffer) { vkDestroyBuffer(device, indexBuffer, nullptr); indexBuffer = VK_NULL_HANDLE; }
  if (indexMemory) { vkFreeMemory(device, indexMemory, nullptr); indexMemory = VK_NULL_HANDLE; }

  indexCount = static_cast<uint32_t>(mesh.indices.size());
  VkDeviceSize vertexSize = mesh.vertices.size() * sizeof(Vertex);
  VkDeviceSize indexSize = mesh.indices.size() * sizeof(uint32_t);

  // Staging buffer for vertices
  VkBuffer stagingBuf;
  VkDeviceMemory stagingMem;
  createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuf, stagingMem);

  void* data;
  vkMapMemory(device, stagingMem, 0, vertexSize, 0, &data);
  memcpy(data, mesh.vertices.data(), vertexSize);
  vkUnmapMemory(device, stagingMem);

  createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexMemory);
  copyBuffer(stagingBuf, vertexBuffer, vertexSize);

  vkDestroyBuffer(device, stagingBuf, nullptr);
  vkFreeMemory(device, stagingMem, nullptr);

  // Staging buffer for indices
  createBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuf, stagingMem);

  vkMapMemory(device, stagingMem, 0, indexSize, 0, &data);
  memcpy(data, mesh.indices.data(), indexSize);
  vkUnmapMemory(device, stagingMem);

  createBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexMemory);
  copyBuffer(stagingBuf, indexBuffer, indexSize);

  vkDestroyBuffer(device, stagingBuf, nullptr);
  vkFreeMemory(device, stagingMem, nullptr);

  std::cerr << "[SceneView] Uploaded mesh: " << mesh.vertices.size()
            << " vertices, " << mesh.indices.size() << " indices" << std::endl;
}

// ---- Hot-reload ----

void SceneView::reloadPipeline(const std::string& vertSpvPath, const std::string& fragSpvPath) {
  if (!initialized) return;
  vkDeviceWaitIdle(device);

  if (pipeline) { vkDestroyPipeline(device, pipeline, nullptr); pipeline = VK_NULL_HANDLE; }
  if (pipelineLayout) { vkDestroyPipelineLayout(device, pipelineLayout, nullptr); pipelineLayout = VK_NULL_HANDLE; }

  createPipeline(vertSpvPath, fragSpvPath);
  std::cerr << "[SceneView] Pipeline reloaded" << std::endl;
}

void SceneView::setMesh(int primitiveType) {
  if (!initialized) return;
  vkDeviceWaitIdle(device);

  Mesh mesh;
  switch (primitiveType) {
    case 0: mesh = Mesh::createQuad(); break;
    case 1: mesh = Mesh::createCube(); break;
    case 2: mesh = Mesh::createSphere(); break;
    default: mesh = Mesh::createCube(); break;
  }
  uploadMesh(mesh);
}

// ---- Rendering ----

void SceneView::recordCommands(VkCommandBuffer cmdBuffer) {
  if (!initialized || !pipeline || !vertexBuffer) return;

  VkClearValue clearColor = {.color = {.float32 = {0.1f, 0.1f, 0.12f, 1.0f}}};

  VkRenderPassBeginInfo rpBegin{
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = renderPass,
    .framebuffer = framebuffer,
    .renderArea = { .offset = {0, 0}, .extent = { width, height } },
    .clearValueCount = 1,
    .pClearValues = &clearColor,
  };

  vkCmdBeginRenderPass(cmdBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  // Push MVP matrix
  updateCamera();
  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 mvp = projMatrix * viewMatrix * model;
  vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                     0, sizeof(glm::mat4), &mvp);

  VkBuffer buffers[] = { vertexBuffer };
  VkDeviceSize offsets[] = { 0 };
  vkCmdBindVertexBuffers(cmdBuffer, 0, 1, buffers, offsets);
  vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);

  vkCmdEndRenderPass(cmdBuffer);
}

void SceneView::updateCamera() {
  float yawRad = glm::radians(orbitYaw);
  float pitchRad = glm::radians(orbitPitch);

  glm::vec3 eye;
  eye.x = orbitDistance * cos(pitchRad) * sin(yawRad);
  eye.y = orbitDistance * sin(pitchRad);
  eye.z = orbitDistance * cos(pitchRad) * cos(yawRad);
  eye += panOffset;

  glm::vec3 target = panOffset;
  glm::vec3 up(0.0f, 1.0f, 0.0f);

  viewMatrix = glm::lookAt(eye, target, up);
  float aspect = static_cast<float>(width) / static_cast<float>(height);
  projMatrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
  // Vulkan clip space: flip Y
  projMatrix[1][1] *= -1.0f;
}

void SceneView::handleCameraInput() {
  ImGuiIO& io = ImGui::GetIO();

  if (!ImGui::IsItemHovered()) return;

  // Left drag: orbit
  if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    ImVec2 delta = io.MouseDelta;
    orbitYaw -= delta.x * 0.5f;
    orbitPitch += delta.y * 0.5f;
    orbitPitch = glm::clamp(orbitPitch, -89.0f, 89.0f);
  }

  // Middle drag: pan
  if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
    ImVec2 delta = io.MouseDelta;
    float panSpeed = orbitDistance * 0.003f;
    float yawRad = glm::radians(orbitYaw);
    glm::vec3 right(cos(yawRad), 0.0f, -sin(yawRad));
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    panOffset -= right * delta.x * panSpeed;
    panOffset += up * delta.y * panSpeed;
  }

  // Scroll: zoom
  if (io.MouseWheel != 0.0f) {
    orbitDistance -= io.MouseWheel * orbitDistance * 0.1f;
    orbitDistance = glm::clamp(orbitDistance, 0.5f, 50.0f);
  }
}

void SceneView::renderImGui() {
  if (!initialized || !imguiTextureId) return;

  ImVec2 avail = ImGui::GetContentRegionAvail();
  if (avail.x <= 0 || avail.y <= 0) return;

  ImVec2 cursorPos = ImGui::GetCursorScreenPos();

  ImGui::Image(imguiTextureId, avail);
  handleCameraInput();

  // ImGuizmo orientation gizmo (view cube) in top-right corner
  ImGuizmo::SetDrawlist();
  ImGuizmo::SetRect(cursorPos.x, cursorPos.y, avail.x, avail.y);

  glm::mat4 identity = glm::mat4(1.0f);
  float viewManipSize = 128.0f;
  glm::mat4 viewCopy = viewMatrix;
  ImGuizmo::ViewManipulate(
      &viewCopy[0][0],
      orbitDistance,
      ImVec2(cursorPos.x + avail.x - viewManipSize, cursorPos.y),
      ImVec2(viewManipSize, viewManipSize),
      0x10101010
  );

  // If the user clicked the view cube, extract new orbit angles from the manipulated view matrix
  if (viewCopy != viewMatrix) {
    // Extract eye position from inverse view matrix
    glm::mat4 invView = glm::inverse(viewCopy);
    glm::vec3 eye = glm::vec3(invView[3]);
    glm::vec3 dir = eye - panOffset;
    orbitDistance = glm::length(dir);
    if (orbitDistance > 0.001f) {
      dir /= orbitDistance;
      orbitPitch = glm::degrees(asin(glm::clamp(dir.y, -1.0f, 1.0f)));
      orbitYaw = glm::degrees(atan2(dir.x, dir.z));
    }
  }
}

uint32_t SceneView::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProps;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

  for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable memory type");
}
