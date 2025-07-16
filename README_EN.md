# ETSlime Game Engine Development Notes (Complete English Edition)

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
11. [Enemy AI System](#enemy-ai-system)
12. [Asynchronous Architecture System](#asynchronous-architecture-system)
13. [UI System (animation & scene integration)](#ui-system-animation--scene-integration)
14. [Game Central Control](#game-central-control)

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
- Octree structure for fast triangle queries on terrain  
- Each node contains AABB, child pointers, and triangle indices  
- Supports **dynamic construction and destruction**  
- Implements **asynchronous building** via multi-threading to avoid main thread stalls

### 🧊 Voxel Grid Spatial Partitioning
- Sparse voxel grid using a hash table and Morton encoding for 3D spatial indexing  
- `VoxelKey` (integer coordinates) → `MortonCode64` used as key  
- Each `VoxelEntry` holds triangle pointers with thread-safe access (`SpinLock`)  
- Flattened for GPU access:  
  - `IndexTripletPageHeader`: per-page metadata (Morton code, triangle count, index offset)  
  - All triangles uploaded to `StructuredBuffer<Triangle>`  
- GPU query flow:  
  - Compute Morton code from position → binary search for page  
  - Perform ray-triangle intersection per triangle in the page  
- ✅ **SPH Particle System Integration**:  
  - Each particle predicts its next position in `UpdateCS`  
  - Using Voxel Grid, nearby terrain triangles are queried on GPU  
  - Distance and normal of each triangle are computed to simulate water-surface collisions  
  - Entirely GPU-based, with no CPU-side interaction or data copying 

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
- Organized by `ShaderGroup` enum to group related effect resources.
- Supports batch rendering and unified resource binding.
- `ParticleEffectRendererBase` abstracts common particle pipeline behavior:
  - Manages `BufferSet` creation: encapsulates `StructuredBuffer`, `SRV`, and `UAV` for particle simulation.
  - In `Update()` stage, dispatches compute shader to simulate particles.
  - Handles automatic binding/unbinding of structured buffers for drawing.
  - Provides `DrawParticles()` as a virtual function for subclasses to implement specific rendering (e.g. billboard, flipbook).

---

## Water Particle Rendering

The water simulation system is implemented in a particle-driven manner, where all water surface visuals are constructed by aggregating fluid particles. It supports realistic behaviors such as flow, collision, and accumulation. Unlike transient effects like fire or smoke, water particles have a continuous lifespan and support large-scale simulation with full scene interaction.

### 🌊 Water Particle Emission Logic (Emitter)
- Water particles are emitted per frame via the `Emit()` interface, controlled by the emitter.
- Particles are **persistent physical entities** that do not disappear quickly.
- Emission areas support shapes such as Cone and Box, assigning initial position and velocity on creation.
- `AliveList` is used to manage active particles, rendered efficiently via `DrawIndirect` on the GPU.

### 💡 SPH Fluid System
The simulation uses SPH (Smoothed Particle Hydrodynamics). Each particle acts as a fluid unit and moves, accumulates, and collides under forces like gravity, viscosity, and pressure.

Two types of spatial indexing structures are supported:

#### ✅ Dense Grid
- Uniform voxel grid
- Used in early testing or for small particle counts
- Accessed via `FlattenIndex(x, y, z)`

#### ✅ Sparse Grid
- Optimized for large-scale simulation with sparsely distributed particles
- Particles are spatially hashed and sorted using Morton encoding
- Entirely GPU-based particle lookup and interaction

#### 🔁 SPH Sparse Grid Construction Flow

In each frame, the system constructs a sparse voxel grid based on the alive particles from the previous frame to accelerate neighbor queries:

1. The CPU retrieves alive particles from the previous frame via ReadBack.
2. For each particle, its voxel coordinate is computed and `gridMin` is updated to avoid negative coordinates.
3. Morton code is computed for each voxel, and particle IDs are accumulated into the `mortonMap`.
4. `mortonMap` entries are converted into `TempTriplet` structures (holding Morton code and particle ID), and sorted.
5. From the sorted list, `SPHIndexTriplet` is constructed to group particles sharing the same Morton code.
6. `PagedIndexTripletBuffer` is then generated from `SPHIndexTriplet` and uploaded to the GPU.

> From the next frame onward, the GPU performs efficient neighbor queries and SPH force calculations using this structure:

7. Each particle uses its `prevPosition` to determine its previous voxel and nearby Morton codes.
8. Neighboring pages are queried using `PagedIndexTripletBuffer`.
9. Using `g_SortedIndicesSRV`, neighbor indices are obtained and used with `g_ParticlesUAV` to compute SPH interactions like density, pressure, and viscosity.

#### 🔬 SPH Force Formulas (Pressure / Viscosity)

Every frame, the GPU performs a 3×3×3 neighbor search and evaluates SPH forces:

- **Wendland C2 kernel** is used to calculate interaction weights between particles
- Density is estimated by accumulating mass from neighboring particles using the kernel
- Pressure force is derived from density differences and the kernel gradient (`∇W`)
- Viscosity force is smoothed using velocity differences and `∇W`
- Final forces:
  - `pressureForce += -mass * (p_i + p_j) / 2ρ_j * ∇W`
  - `viscosityForce += viscosity * velDiff * mass / ρ_j * ∇W`
- **XSPH correction** is added to unify particle velocities:
  - `velocity += ε * Σ (neighbor.velocity - self.velocity) * W`

All computations are handled by Compute Shader, iterating nearby Morton-coded voxel pages.

#### 🧱 Voxel Grid + Triangle Collision Detection

Fluid particles interact with static terrain using Voxel Grid accelerated triangle collision detection:

- Each frame builds or reuses a `VoxelGrid` (sparse Morton-encoded triangle map)
- Each particle emits a ray from its `predictedPosition`
- `RayIntersectsTriangle()` is used to test against triangles in nearby voxels
- On collision:
  - **Penetration prevention**: reflect or slide the particle along the normal
  - **Surface sliding**: retain tangential velocity, remove vertical component
- Enables natural downhill flow, edge accumulation, container flowback, etc.

Executed in parallel with SPH force evaluation for coherent simulation.

### 🧊 Surface Reconstruction with Marching Cubes

Since water particles are discrete, Marching Cubes is used to reconstruct a continuous surface mesh for rendering.

#### 📐 Scalar Field Generation

Two methods supported for building scalar fields:

##### ✅ Dense Grid
- Fixed-size 3D grid allocation
- Cleared and rebuilt each frame
- Indexed using `FlattenIndex(x, y, z)`
- Pros: Simple structure, easy to debug
- Cons: High memory usage, not suitable for large areas

##### ✅ Sparse Grid
- Builds only active voxels near particles
- Each particle visits affected voxels within influence radius
- Uses **64-bit Morton encoding** to support large spatial range
- Uses **Voxel-Centric (pull-based)** construction:
  - Each voxel queries nearby particles to accumulate weight
  - Weight is distance-attenuated using **Wendland C2**
- Result is uploaded as `StructuredBuffer<float>`, supporting concurrent GPU writes via `InterlockedAdd`

Follows the same paging system and Morton infrastructure as `SPHSystem`.

#### 🧱 Marching Cubes Surface Mesh Generation

Once scalar fields are built, Marching Cubes is run on the GPU:

1. For each voxel, check if the scalar values of its 8 corners cross the isosurface threshold
2. Use `g_TriTableSRV` to get triangle topology
3. Calculate vertex positions and normals along edges, write to `VertexBuffer` and `IndexBuffer` via `InterlockedAdd`
4. Render using `DrawIndexedInstancedIndirect` with optional cluster parallelism

An **Edge Vertex Cache** is used to avoid duplicate vertex creation.

#### 💡 Thickness Map-based Surface Shading

To enhance volume and realism, surface shading uses a screen-space **thickness map**:

- Fluid particles write depth to a thickness texture
- Surface mesh samples the map during shading to enhance darkening and absorption
- Enables:
  - Soft translucent edges
  - Darker color in deeper areas

Managed by the `FluidThicknessRenderer` module.

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
- Provides ImGui interface to:
  - Show AABBs
  - Adjust CSM parameters
  - Toggle debug options

---
