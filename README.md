# ABoxLab

A shader testing and 3D development tool built with Vulkan and ImGui.

## Project Goals

### Core Interface Components

1. **Menu Bar**
   - File menu (New, Open, Save, Close, etc.)
   - Edit menu (Undo, Redo, Cut, Copy, Paste, etc.)
   - View menu (toggle panels, layouts)
   - Additional menus as needed

2. **Left Workspace - File Tree View**
   - Hierarchical file browser showing project assets
   - Support for common 3D tool file types:
     - Code files (shaders, scripts)
     - Images/Textures (PNG, JPG, etc.)
     - 3D models (OBJ, glTF, FBX, etc.)
     - Materials
     - Scenes

3. **Main Window - Multi-purpose Viewer**
   - Tabbed interface supporting multiple file types:
     - Code Editor - Edit shaders and scripts
     - Image Viewer - Preview textures and images
     - 3D Object Viewer - View and manipulate 3D models
     - Render View - Real-time shader preview

4. **Render View**
   - Programmable scene composition
   - Add/remove objects dynamically
   - Assign shaders to objects
   - Adjust materials and properties
   - Real-time preview of shader effects
   - Switch between edit mode and render view

5. **Object Management**
   - Conditional rendering (show/hide objects)
   - Transform objects (translate, rotate, scale)
   - Material assignment
   - Shader binding

6. **Shader Compilation System**
   - SPIR-V compiler integration
   - Slang shader compiler support
   - GLSL compilation
   - HLSL compilation
   - Real-time shader compilation and hot-reload
   - Error reporting and validation

## Current Status

- Basic Vulkan rendering setup with ABox
- ImGui integration for UI
- Window management and swapchain
- Render pass and framebuffer management

Planned:
- UI layout
- File tree view
- Code editor
- 3D viewport
- Shader compilation system
- Object management

## Architecture

- **ABox**: Custom Vulkan rendering library for resource management
- **ImGui**: UI framework for interface elements
- **GLFW**: Window and input handling
- **Vulkan**: Graphics API
- **Slang**: High-level shader compiler

## Building

```bash
make release    # Build optimized version
make debug      # Build debug version
make clean      # Clean build artifacts
make install    # Install to system (requires sudo)
```

## Dependencies

- Vulkan SDK
- GLFW3
- CMake >= 3.25
- C++20 compatible compiler
- Slang compiler (for shader compilation)
