cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PREFIX_PATH /usr/local ${CMAKE_PREFIX_PATH})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ABoxLab)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/_deps)

# Fetch slang first (it includes glslang as a dependency)
# This must be before add_subdirectory(ABox) so ABox can use glslang targets
FetchContent_Declare(
  slang
  GIT_REPOSITORY https://github.com/shader-slang/slang.git
  GIT_TAG v0.27.0
)
FetchContent_MakeAvailable(slang)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG master
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
  ImGuiColorTextEdit
  GIT_REPOSITORY https://github.com/goossens/ImGuiColorTextEdit.git
  GIT_TAG master
)
FetchContent_MakeAvailable(ImGuiColorTextEdit)
FetchContent_GetProperties(ImGuiColorTextEdit SOURCE_DIR ImGuiColorTextEdit_SOURCE_DIR)

FetchContent_Declare(
  imnodes
  GIT_REPOSITORY https://github.com/Nelarius/imnodes.git
  GIT_TAG master
)
FetchContent_Populate(imnodes)
FetchContent_GetProperties(imnodes SOURCE_DIR imnodes_SOURCE_DIR)

add_library(imgui_lib
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui_lib PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC
  Vulkan::Vulkan
  glfw
)

add_subdirectory(ABox)

# UI Module
add_library(ui
  src/ui/MenuBar.cpp
  src/ui/MenuBar.hpp
  src/ui/FileTree.cpp
  src/ui/FileTree.hpp
  src/ui/FileDialog.cpp
  src/ui/FileDialog.hpp
  src/ui/CodeEditor.cpp
  src/ui/CodeEditor.hpp
  src/ui/ShaderCompiler.cpp
  src/ui/ShaderCompiler.hpp
  src/ui/ShaderGraph.cpp
  src/ui/ShaderGraph.hpp
  "${ImGuiColorTextEdit_SOURCE_DIR}/TextEditor.cpp"
  "${imnodes_SOURCE_DIR}/imnodes.cpp"
)
target_include_directories(ui PUBLIC ${CMAKE_SOURCE_DIR} ${imgui_SOURCE_DIR} ${slang_SOURCE_DIR}/include "${ImGuiColorTextEdit_SOURCE_DIR}" "${imnodes_SOURCE_DIR}")
target_link_libraries(ui PUBLIC imgui_lib glfw slang)

# Project Module
add_library(project
  src/project/ProjectManager.cpp
  src/project/ProjectManager.hpp
  src/project/FileTemplates.cpp
  src/project/FileTemplates.hpp
)
target_include_directories(project PUBLIC ${CMAKE_SOURCE_DIR})

add_executable(ABoxLab
  main.cpp
  ABoxLabApp.cpp
  ABoxLabApp.hpp
)

target_compile_definitions(ABoxLab PRIVATE SPIRV_REFLECT_USE_SYSTEM_SPIRV_H)

# Add sanitizer support for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Use ASAN without leak detection (GLFW/Wayland have cleanup issues with LSAN)
  target_compile_options(ABoxLab PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(ABoxLab PRIVATE -fsanitize=address)

  target_compile_options(imgui PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(imgui PRIVATE -fsanitize=address)

  target_compile_options(ui PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(ui PRIVATE -fsanitize=address)

  target_compile_options(project PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(project PRIVATE -fsanitize=address)
endif()

target_link_libraries(
  ABoxLab
  PRIVATE
  ABoxLib
  imgui_lib
  ui
  project
)


set_target_properties(ABoxLab PROPERTIES INSTALL_RPATH "$VULKAN_SDK/lib")
install(TARGETS ABoxLab RUNTIME DESTINATION bin)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_BINARY_DIR}/compile_commands.json"
          "${CMAKE_SOURCE_DIR}/compile_commands.json"
)
