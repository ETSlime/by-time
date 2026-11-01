# ETSlime Game Engine Development Notes (Complete English Edition)

> This document records the entire implementation and architecture of ETSlime's DirectX 11-based game engine in module-by-module detail.  
> Tech Stack: C++ / DirectX 11 / HLSL / Win32 / ImGui / Compute Shader / Async Task System

---

##  Table of Contents

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

###  Forward Rendering Architecture
- Uses Forward rendering (non-deferred).
- Multiple rendering passes:
  - Main Pass
  - Shadow Pass (up to 4 cascades for CSM)
- Supported object types:
  - Static Mesh
  - Skinned Mesh
  - Instanced Objects

###  Cascaded Shadow Map (CSM)
- Supports up to 4 shadow cascades.
- Automatically computes LightViewProj matrices.
- Snap-to-texel precision alignment.
- Debug frustum visualization available.
- ShadowMapRenderer handles layout/shader selection for static/skinned/instanced models.
- Uses object AABB to determine cascade level; supports cross-layer upload.

---

## Instanced Rendering

###  Supported Object Types
- Grass (with wind sway)
- Trees (no wind sway)
- Rocks, flowers, bushes (planned)

###  Technical Details
- Each object type has its own `InstanceBuffer` and `InputLayout`.
- Grass uses `GrassVertex` with per-vertex `Weight` for wind influence.
- `InstanceData` includes transform matrix, scale, and rotation.
- Updated per-frame using `D3D11_USAGE_DYNAMIC + Map/Unmap`.
- Culled instances are stored in `SimpleArray<InstanceData>* visibleInstances`.

###  LOD Support (Structure Reserved, Not Yet Implemented)
- Planned: multiple LOD buffers per object type.
- Will update based on camera distance per frame.
- Will call `DrawIndexedInstanced()` per LOD level.

---

## Animation System

###  State Machine Structure
- Implemented using `AnimationStateMachine<State>` template.
- Uses enum type `State` for efficient lookup.
- Each state stores the current animation, previous animation, blend time, transition condition, and finish callback.

###  Transition Conditions
- Uses member function pointers: `bool (Character::*)() const`.
- Called through `GetNextState(ISkinnedMeshModelChar*)` to evaluate conditions.

###  Animation Blending
- Position: `XMVectorLerp`
- Rotation: `XMQuaternionSlerp`
- Bone matrix blending handled inside state machine.

---

## Particle System

###  Fully GPU-Driven Architecture
- All particle logic handled via Compute Shader.
- Four stages:
  - `EmitCS`: Spawn particles
  - `UpdateCS`: Update particle position/velocity/life
  - `SortCS`: GPU sorting for alpha transparency
  - `DrawIndirect`: Indirect draw for performance

###  Management Structure
- FreeList structure reuses dead particle slots.
- Allocation via `g_FreeListConsumeUAV.Consume()`.
- `spawnRate` defines emission frequency.

###  Features
- **Texture Sheet Animation (Flipbook)**
- **Cone-shaped emission**
- **Lifetime-based scale/color control**
- **SoftParticles for fade-out near camera**
- **Efficient rendering using DrawIndirect + GPU sort**

---

## Terrain & Collision

###  Octree Acceleration Structure
- Uses an Octree to spatially partition terrain triangles and accelerate triangle queries within specified regions
- Each node contains an AABB, child nodes, and corresponding triangle indices
- Supports dynamic, deferred, and asynchronous construction to reduce main-thread stalls during large-scene initialization
- Can be used for CPU-side spatial queries such as terrain-region searches, ray casting, and object placement

###  Sparse Voxel Grid
- Provides a **Sparse Voxel Grid** spatial index for scene triangles, storing only active voxels that actually contain geometry
- Uses **64-bit Morton encoding** to organize spatial data, combined with sorting, paged indexing, and binary search to efficiently locate candidate triangles
- Scene triangles can be uploaded to `StructuredBuffer`, allowing Compute Shaders to perform spatial queries and precise collision detection directly on the GPU
- Designed as a general-purpose scene geometry query structure that can be reused by higher-level systems such as particle simulation, Ray / Triangle collision, trajectory prediction, and laser collision detection
- For details of Voxel Grid collision in the fluid simulation system, see the dedicated **Water Simulation** technical documentation

###  Grass Distribution
- Grass is generated in randomized clusters rather than being uniformly distributed across the entire terrain
- Each Cluster defines a randomized region and queries candidate terrain triangles through the Octree
- Ray tests are used to determine the exact placement position of each grass instance
- Grass orientation is automatically adjusted according to the triangle surface normal
- Terrain slope is evaluated to prevent grass generation on excessively steep surfaces

---

## Model Import & Animation Management

### Custom FBXLoader
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

###  Light Control
- Player holds a dynamic directional light (rotates with time).
- Torch holds a local point light.

###  TextureSlotManager
- Unified slot definitions:
  - g_Texture → t0
  - g_ShadowMap[x] → t1
  - g_NormalMap → t9, etc.
- Avoids redundant binding by tracking current slot state.

---

## Shader Management

###  ShaderManager Architecture
- Uses `ShaderID` enum as the key.
- Manages ShaderSet, ShadowShaderSet, ComputeShaderSet.
- Supports hot reload via `ReloadAllIfChanged()`.

###  Hot Reload Mechanism
- Uses `(path, entry)` as key.
- Compares compiled blob hash to detect changes.
- Prevents double release via `outputShaderPtr` tracking.

---

## Effect System

### FireEffectRenderer (Fire + Smoke)
- Plays fire animations using a texture sheet
- Renders smoke through a particle system, independently or together with fire
- Exposes unified `Update()` / `Draw()` interfaces
- Supports blend modes such as Additive and AlphaBlend
- Supports soft particles and flipbook animation blending

### Particle–Terrain Collision (Falling.hlsl)
- Performs particle emission, velocity integration, and position updates on the GPU using compute shaders, with AliveList / FreeList managing active particles and reusable slots
- Transforms predicted positions into world space and generates Morton codes from voxel coordinates to look up associated triangle indices, narrowing the collision search
- Constructs a world-space ray from the current frame's displacement and limits intersection tests to that movement distance
- Corrects the particle position on impact with an offset along the surface normal and clears its velocity, providing contact response against terrain surfaces
- Expands particles into camera-facing billboards in the geometry shader, using circular clipping and an opacity gradient for a soft appearance

### Water Surface Rendering (LakeSurface.hlsl)
- Combines two normal maps scrolling in different directions and at different speeds to produce animated surface detail
- Samples the normal maps in the vertex shader to displace vertices, combining geometric motion with normal perturbation in the pixel shader
- Uses `g_WaveStrength` to control both displacement amplitude and shading-normal perturbation; setting it to 0 disables displacement and uses a flat surface normal for a calm surface state
- Computes a Fresnel weight from the viewing direction and surface normal to blend the base water color with a highlight color and adjust opacity
- Exposes water color, wave strength, Fresnel bias, and Fresnel power through a constant buffer

### Procedural Laser Beam (Laser.hlsl)
- Receives world-space ribbon vertices from the CPU and uses longitudinal and transverse coordinates to construct the beam appearance in the pixel shader
- Combines Core, Body, and Rim intensity profiles to represent a bright central core, beam body, and edge halo
- Modulates longitudinal intensity with a time-dependent sine function to create a flowing pattern without texture sampling
- Applies smooth attenuation at both ends to soften the beam's endpoints
- Calculates color intensity and opacity weights separately, with a minimum opacity contribution to retain the visible silhouette; exposes HDR base color, flow speed, and core sharpness controls

### Decal System (Decal.hlsl)
- Uses per-instance data to configure decal position, size, surface normal, rotation, and lifetime, supporting surface effects such as impact marks, blast marks, and beam contact marks
- Constructs a local basis from the surface normal to orient planar decals against the contact surface
- Applies a position offset based on the surface normal and viewing direction to reduce Z-fighting
- Uses a 4×2 texture atlas, with per-instance UV offsets selecting different decal patterns
- Calculates opacity attenuation from elapsed time and lifetime to fade marks out gradually

### Trajectory and Area Preview (ProjectilePreview.hlsl)
- Switches between trajectory and hemispherical area rendering within the same shader using `g_RenderType`
- Receives externally generated trajectory vertices and uses the Y component of their normals as an opacity modulation parameter to produce an opacity gradient
- Positions and scales a hemisphere mesh around the supplied hit position to visualize the expected area of effect near the contact point
- Uses time-dependent sine functions to animate hemisphere scale and opacity, producing a periodic pulsing effect
- Attenuates opacity based on distance from the hit position, with the attenuation range following the hemisphere's animated scale
- Exposes hit position, area scale, base color, time, and render type through a shared constant buffer

### ShaderGroup and Particle Effect Architecture
- Groups related effect shaders and resources using the `ShaderGroup` enum
- Supports batch rendering and unified resource binding
- `ParticleEffectRendererBase` abstracts the common GPU pipeline for particle simulation and rendering:
  - Encapsulates `BufferSet` creation and management, including particle `StructuredBuffer`, `SRV`, and `UAV` resources
  - Binds resources and dispatches compute shaders during `Update()` for GPU simulation stages such as Emit / Update
  - Encapsulates resource binding and unbinding across pipeline stages to avoid read/write conflicts
  - Prepares particle buffers for drawing and manages resource transitions between compute shaders and the graphics pipeline
  - Provides an abstract `DrawParticles()` interface for derived classes to implement rendering methods such as billboards and flipbooks
- Particle effects reuse the underlying buffer management and compute dispatch framework while supplying their own simulation parameters and drawing logic

---

## Enemy AI System

###  Managed by EnemyManager
- Centralized control for all enemies.
- Handles creation, update, rendering, and destruction.

###  Behavior Tree Design
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

###  Task Management
- Unified async system manages model and resource loading.
- Supports:
  - `DeferredTaskBinder<T>`: Structured task bound to class methods.
  - `RawDeferredTask`: Raw function or lambda-based task execution.

###  Thread Pool
- Custom `ThreadPool` with Win32 `CreateEvent` for task wake/wait.
- TaskOptions support: priorities, delayed tasks, etc.

###  Thread Safety
- Implements `SpinLock` and `LightweightMutex`.
- Octree construction uses internal locks for multithread safety.

###  Async Model Loading
- Model load split into phases:
  1. Sub-thread reads/parses `.obj` / `.fbx`
  2. Main thread creates GPU resources
  3. Callback notifies completion and model is initialized

###  Async Texture Upload
- Read and upload split:
  - Sub-thread reads file and pushes to `TextureUploadQueue`
  - Main thread waits via `WaitForSingleObject` and creates SRV
- Callback on completion updates progress counter
- Lock-protected access for upload task pool

---

## UI System (Animation & Scene Integration)

###  UIAnimatedSprite
- Plays frame-based UI animations using sprite sheets.
- Supports looping, framerate control, fade out.

###  ScreenFader
- `ScreenFader` handles fade-in/out transitions globally.
- Uses `UIOverlayRenderer` and alpha control.
- Exposed API integrates with all scene transitions.

###  UILoadingScene
- Loading screen scene with internal state machine:
  - Entry → Loading → Completion → Exit animation transitions
- Duration and animation timing controlled automatically.

###  UITitleScene
- Dedicated scene for the title screen (logo, buttons, intro animations).
- Press Enter triggers fade-out and transitions to main game.

###  Scene & GameMode Management
- `Scene` extended to support model load/unload by SceneID.
- Switching GameMode unloads old resources, async loads target models.
- GameObject registration/destruction unified via central API.

---

## Game Central Control

###  GameSystem & Async Resource Loading
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
