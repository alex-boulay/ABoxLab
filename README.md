# ABoxLab

A shader testing and 3D development tool built with Vulkan and ImGui — inspired by RenderMonkey.

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
     - Node Graph - Wire up meshes, shaders, and textures
     - Viewport - Real-time 3D render preview

4. **Node Graph**
   - Visual node editor for assembling render configurations
   - Node types: Mesh, Shader, Texture, Float, Vec3, Color, Material Output
   - Connect shader stages + mesh + textures to a Material Output node
   - Bindings auto-apply to the 3D viewport when connected

5. **Viewport (SceneView)**
   - Offscreen Vulkan render target displayed in ImGui
   - Orbit camera (left-drag rotate, middle-drag pan, scroll zoom)
   - ImGuizmo orientation gizmo
   - MVP push constants for vertex shaders
   - Hot-reload: pipeline and mesh swap at runtime from node graph

6. **Object Management** (planned)
   - Full scene composition view (separate from node graph)
   - Conditional rendering (show/hide objects)
   - Transform objects (translate, rotate, scale)
   - Material assignment and shader binding

7. **Shader Compilation System**
   - SPIR-V compiler integration via Slang
   - GLSL and HLSL support
   - Real-time linting with debounce while typing
   - Error reporting with line/column markers
   - Automatic SPIR-V introspection (inputs/outputs/descriptors)

## Current Status

### Implemented
- Vulkan rendering with manual frame synchronization
- ImGui master branch with custom rendering integration
- Project management with recent projects tracking
- File tree view with project browsing
- Code editor with syntax highlighting (GLSL/HLSL)
- Real-time shader linting with error markers
- Shader compilation to SPIR-V using Slang compiler
- Shader introspection via SPIR-V Reflect
- **Node Graph** — visual node editor (imnodes) with typed pins and multiple node types
- **SceneView** — offscreen Vulkan renderer with orbit camera, push constants, ImGuizmo
- **Scene module** — Mesh primitives (Quad, Cube, Sphere), Scene/SceneObject data structures
- **Graph-to-viewport binding** — connecting nodes to the Output node auto-applies to the 3D view
- Hot-reload of pipeline (shader swap) and mesh from the node graph, including on recompile
- Node graph link and node deletion (Ctrl+Click or right-click context menu)
- Shader recompile preserves existing node graph connections
- Tab-based UI switching between Code Editor, Node Graph, and Viewport

### In Progress
- Texture node loading and binding
- Full scene composition view (multi-object)

### Planned
- Image/texture loading and GPU upload
- Multiple render passes / multi-object scenes
- Transform gizmos (translate/rotate/scale) on scene objects
- Material properties panel
- Export/import of node graph configurations

## Architecture

### Design Philosophy

ABoxLab follows the modular architecture pattern used by large open-source projects like Blender and GIMP:

- **Separation of Concerns**: Each subsystem is self-contained in its own module
- **User Workspace Independence**: User projects live anywhere on the filesystem; the app only stores references
- **Manager Pattern**: Focused manager classes coordinate specific domains

### Module Structure

```
ABoxLab/
├── src/
│   ├── ui/              # UI components (MenuBar, FileTree, CodeEditor, NodeGraph)
│   ├── renderer/        # SceneView — offscreen Vulkan rendering + orbit camera
│   ├── scene/           # Mesh primitives, Scene/SceneObject data structures
│   └── project/         # Project management and file tracking
├── resources/
│   └── shaders/         # Default vertex/fragment shaders
├── ABox/                # Custom Vulkan library (submodule)
├── ABoxLabApp.cpp       # Application orchestration
└── main.cpp
```

### User Data Management

User projects are **not** stored within the application directory. Instead:

- **Project Files**: User chooses location (e.g., `~/Documents/MyShaderProject/`)
- **App Config**: Stored in standard OS locations:
  - Linux: `~/.config/aboxlab/`
  - Windows: `%APPDATA%/aboxlab/`
  - macOS: `~/Library/Application Support/aboxlab/`
- **Recent Projects**: Tracked via JSON file storing file paths, not copying files

### Core Technologies

- **ABox**: Custom Vulkan rendering library for resource management
- **ImGui**: UI framework (master branch)
- **ImGuiColorTextEdit**: Syntax-highlighted code editor
- **imnodes**: Visual node editor
- **ImGuizmo**: 3D orientation gizmo (ViewManipulate)
- **GLM**: Math library (matrices, vectors, transforms)
- **GLFW**: Window and input handling
- **Vulkan**: Graphics API
- **Slang**: High-level shader compiler
- **SPIR-V Reflect**: Shader introspection

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
- GLM
- CMake >= 3.25
- C++20 compatible compiler
- SPIR-V Reflect
