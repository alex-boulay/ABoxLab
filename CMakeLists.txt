cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PREFIX_PATH /usr/local ${CMAKE_PREFIX_PATH})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ABoxLab)

# Add ABox as a subdirectory to build the library
add_subdirectory(ABox)

add_executable(ABoxLab
  main.cpp
  ABoxLabApp.cpp
  ABoxLabApp.hpp
)

target_compile_definitions(ABoxLab PRIVATE SPIRV_REFLECT_USE_SYSTEM_SPIRV_H)

# Link against ABoxLib - it already provides all dependencies and includes
target_link_libraries(
  ABoxLab
  PRIVATE
  ABoxLib
)


set_target_properties(ABoxLab PROPERTIES INSTALL_RPATH "$VULKAN_SDK/lib")
install(TARGETS ABoxLab RUNTIME DESTINATION bin)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_BINARY_DIR}/compile_commands.json"
          "${CMAKE_SOURCE_DIR}/compile_commands.json"
)
