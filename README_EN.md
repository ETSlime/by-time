# 🧠 ETSlime Game Engine Development Notes (Complete English Edition)

> This document records the entire implementation and architecture of ETSlime's DirectX 11-based game engine in module-by-module detail.  
> Tech Stack: C++ / DirectX 11 / HLSL / Win32 / ImGui / Compute Shader / Async Task System

---

## 📚 Table of Contents

1. [Rendering System](#rendering-system)
2. [Instanced Rendering](#instanced-rendering)
3. [Animation System](#animation-system)
4. [Particle System](#particle-system)
5. [Terrain & Collision](#terrain--collision)
6. [Model Import & Animation Management](#model-import--animation-management)
7. [Lighting & Texture Slot Management](#lighting--texture-slot-management)
8. [Shader Management](#shader-management)
9. [Effect System](#effect-system)
10. [Enemy AI System](#enemy-ai-system)
11. [Asynchronous Architecture System](#asynchronous-architecture-system)
12. [UI System (animation & scene integration)](#ui-system-animation--scene-integration)
13. [Game Central Control](#game-central-control)

---

## Rendering System

### 🎨 Forward Rendering Architecture
- Uses Forward rendering (non-deferred).
- Multiple rendering passes:
  - Main Pass
  - Shadow Pass (up to 4 cascades for CSM)
- Supported object types:
  - Static Mesh
  - Skinned Mesh
  - Instanced Objects

### 🌘 Cascaded Shadow Map (CSM)
- Supports up to 4 shadow cascades.
- Automatically computes LightViewProj matrices.
- Snap-to-texel precision alignment.
- Debug frustum visualization available.
- ShadowMapRenderer handles layout/shader selection for static/skinned/instanced models.
- Uses object AABB to determine cascade level; supports cross-layer upload.

---

## Instanced Rendering

### 🌱 Supported Object Types
- Grass (with wind sway)
- Trees (no wind sway)
- Rocks, flowers, bushes (planned)

### 🧩 Technical Details
- Each object type has its own `InstanceBuffer` and `InputLayout`.
- Grass uses `GrassVertex` with per-vertex `Weight` for wind influence.
- `InstanceData` includes transform matrix, scale, and rotation.
- Updated per-frame using `D3D11_USAGE_DYNAMIC + Map/Unmap`.
- Culled instances are stored in `SimpleArray<InstanceData>* visibleInstances`.

### 🌀 LOD Support (Structure Reserved, Not Yet Implemented)
- Planned: multiple LOD buffers per object type.
- Will update based on camera distance per frame.
- Will call `DrawIndexedInstanced()` per LOD level.

---

## Animation System

### 🎭 State Machine Structure
- Implemented using `AnimationStateMachine<State>` template.
- Uses enum type `State` for efficient lookup.
- Each state stores the current animation, previous animation, blend time, transition condition, and finish callback.

### 🔄 Transition Conditions
- Uses member function pointers: `bool (Character::*)() const`.
- Called through `GetNextState(ISkinnedMeshModelChar*)` to evaluate conditions.

### 💫 Animation Blending
- Position: `XMVectorLerp`
- Rotation: `XMQuaternionSlerp`
- Bone matrix blending handled inside state machine.

---

## Particle System

### 🔥 Fully GPU-Driven Architecture
- All particle logic handled via Compute Shader.
- Four stages:
  - `EmitCS`: Spawn particles
  - `UpdateCS`: Update particle position/velocity/life
  - `SortCS`: GPU sorting for alpha transparency
  - `DrawIndirect`: Indirect draw for performance

### 🧠 Management Structure
- FreeList structure reuses dead particle slots.
- Allocation via `g_FreeListConsumeUAV.Consume()`.
- `spawnRate` defines emission frequency.

### 🌈 Features
- **Texture Sheet Animation (Flipbook)**
- **Cone-shaped emission**
- **Lifetime-based scale/color control**
- **SoftParticles for fade-out near camera**
- **Efficient rendering using DrawIndirect + GPU sort**

---

## Terrain & Collision

### 🧱 Octree Acceleration
- Uses Octree to accelerate terrain triangle queries.
- Each node holds an AABB and references to triangles or child nodes.

### 🌾 Grass Distribution Logic
- Cluster-based generation instead of full-random.
- For each cluster:
  - Random center + radius
  - AABB → Octree → triangle query
  - Raycasting to determine ground height
- Grass rotates based on triangle normal.
- Too-steep surfaces are skipped.

---

## Model Import & Animation Management

### 📁 Custom FBXLoader
- Parses ASCII `.fbx` format directly.
- Extracted data:
  - Vertices: position, normal, UV, tangent
  - Indices
  - Skeleton hierarchy: bone parent-child structure
  - Animation keyframes for each bone
- Stores multiple clips in `AnimClip`:
  - Length, looping, interpolation mode
- Each `AnimClip` is connected to the animation state machine for blending transitions.

---

## Lighting & Texture Slot Management

### 🌞 Light Control
- Player holds a dynamic directional light (rotates with time).
- Torch holds a local point light.

### 🎯 TextureSlotManager
- Unified slot definitions:
  - g_Texture → t0
  - g_ShadowMap[x] → t1
  - g_NormalMap → t9, etc.
- Avoids redundant binding by tracking current slot state.

---

## Shader Management

### 🎮 ShaderManager Architecture
- Uses `ShaderID` enum as the key.
- Manages ShaderSet, ShadowShaderSet, ComputeShaderSet.
- Supports hot reload via `ReloadAllIfChanged()`.

### 🔁 Hot Reload Mechanism
- Uses `(path, entry)` as key.
- Compares compiled blob hash to detect changes.
- Prevents double release via `outputShaderPtr` tracking.

---

## Effect System

### 🔥 FireEffectRenderer (Fire + Smoke)
- Plays fire animation using texture sheet.
- Smoke rendered via particle system (toggleable).
- Exposed `Update()` and `Draw()` interface.
- Supports Additive and AlphaBlend modes.
- Includes SoftParticle and Flipbook blending.

### 💡 ShaderGroup Architecture
- Planned: unify effects under ShaderGroup enum.
- Enable efficient batch rendering and binding.

---

## Enemy AI System

### 👾 Managed by EnemyManager
- Centralized control for all enemies.
- Handles creation, update, rendering, and destruction.

### 🧠 Behavior Tree Design
- Each enemy has its own behavior tree.
- Node types:
  - Selector: runs first successful child
  - Sequence: runs all children in order until failure
- Example logic:
  - Patrol → Detect Player → Chase → Attack
  - Field of view check → Turn → Attack
- Conditions based on distance, angle, occlusion, etc.
- Linked to animation system to trigger movement, idle, attack animations.

---

## Asynchronous Architecture System

### 📦 Task Management
- Unified async system manages model and resource loading.
- Supports:
  - `DeferredTaskBinder<T>`: Structured task bound to class methods.
  - `RawDeferredTask`: Raw function or lambda-based task execution.

### 🔁 Thread Pool
- Custom `ThreadPool` with Win32 `CreateEvent` for task wake/wait.
- TaskOptions support: priorities, delayed tasks, etc.

### 🔒 Thread Safety
- Implements `SpinLock` and `LightweightMutex`.
- Octree construction uses internal locks for multithread safety.

### 🧵 Async Model Loading
- Model load split into phases:
  1. Sub-thread reads/parses `.obj` / `.fbx`
  2. Main thread creates GPU resources
  3. Callback notifies completion and model is initialized

### 🖼 Async Texture Upload
- Read and upload split:
  - Sub-thread reads file and pushes to `TextureUploadQueue`
  - Main thread waits via `WaitForSingleObject` and creates SRV
- Callback on completion updates progress counter
- Lock-protected access for upload task pool

---

## UI System (Animation & Scene Integration)

### 🎞 UIAnimatedSprite
- Plays frame-based UI animations using sprite sheets.
- Supports looping, framerate control, fade out.

### 🌗 ScreenFader
- `ScreenFader` handles fade-in/out transitions globally.
- Uses `UIOverlayRenderer` and alpha control.
- Exposed API integrates with all scene transitions.

### ⏳ UILoadingScene
- Loading screen scene with internal state machine:
  - Entry → Loading → Completion → Exit animation transitions
- Duration and animation timing controlled automatically.

### 🏠 UITitleScene
- Dedicated scene for the title screen (logo, buttons, intro animations).
- Press Enter triggers fade-out and transitions to main game.

### 🔁 Scene & GameMode Management
- `Scene` extended to support model load/unload by SceneID.
- Switching GameMode unloads old resources, async loads target models.
- GameObject registration/destruction unified via central API.

---

## Game Central Control

### 🧠 GameSystem & Async Resource Loading
- Manages resources per GameMode (Title / Loading / Game).
- Each mode loads models, textures, UI asynchronously via task system.
- Scene transition triggered after load complete.
- All resources of previous mode are automatically unloaded to save memory.
- Supports preloading and unloading resources by different GameModes (e.g., Title, Loading, In-Game).
- Each GameMode has its own set of resources (models, textures, UI) managed through asynchronous loading.
- When entering a mode, async tasks are dispatched, and the scene switches once loading completes.
- Upon exiting a mode, all registered GameObjects for that mode are automatically released to save memory.

---
