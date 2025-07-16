# 🧠 ETSlime 游戏引擎开发笔记（中文完整版）

> 本文档详细记录了 ETSlime 使用 DirectX11 开发游戏引擎的全部模块与实现方式  
> 技术栈：C++ / DirectX 11 / HLSL / Win32 / ImGui / Compute Shader / Async Task System

---

## 📚 目录

1. [渲染系统](#渲染系统)
2. [实例化渲染](#实例化渲染)
3. [动画系统](#动画系统)
4. [粒子系统](#粒子系统)
5. [地形与碰撞](#地形与碰撞)
6. [模型导入与动画管理](#模型导入与动画管理)
7. [光照与贴图槽位管理](#光照与贴图槽位管理)
8. [Shader 管理系统](#shader-管理系统)
9. [特效系统](#特效系统)
10. [敌人 AI 系统](#敌人-ai-系统)
11. [异步架构系统](#异步架构系统)
12. [ui 系统动画与场景集成](#ui-系统动画与场景集成)
13. [游戏中枢控制](#游戏中枢控制)

---

## 渲染系统

### 🎨 Forward 渲染结构
- 使用 Forward 渲染方式（非延迟渲染）
- 渲染流程分为多个 Pass：
  - Main Pass 主渲染通道
  - Shadow Pass 阴影渲染通道（最多支持 4 层级 CSM）
- 支持渲染对象：
  - 静态模型
  - 骨骼动画模型
  - 实例化对象

### 🌘 CSM 阴影系统
- 实现最大 4 层级的级联阴影贴图
- 自动计算 LightViewProj 阵
- 支持 Texel 对齐精度处理（Snap to Texel）
- 可视化调试 Frustum 区域
- 使用 ShadowMapRenderer 区分静态/骨骼/实例化模型着色器资源
- 利用 AABB 判断每个物体所属层级，并支持跨层冗余上传

---

## 实例化渲染

### 🌱 支持的对象类型
- 草（带风摆动效果）
- 树（不摆动）
- 岩石、花、灌木等未来扩展对象

### 🧩 技术细节
- 每种对象拥有独立的 InstanceBuffer 和 InputLayout
- 草使用 `GrassVertex`，包含每个顶点的 Weight 控制风力影响
- 每个实例的数据包含位移、缩放、旋转矩阵等
- 使用 `D3D11_USAGE_DYNAMIC` + `Map/Unmap` 实现动态上传
- 剔除后实例数据保存在 `SimpleArray<InstanceData>* visibleInstances` 中

### 🌀 支持 LOD
- 为每种对象提供多个 LOD InstanceBuffer
- 每帧根据相机距离切换 LOD
- 每个 LOD 层分别调用 `DrawIndexedInstanced()`

---


## 动画系统

### 📁 FBXLoader 的实现
- 自制 `FBXLoader` 支持解析 ASCII 格式的 `.fbx` 文件
- 可提取数据：
  - 顶点属性（位置、法线、UV、切线）
  - 索引列表
  - 骨骼信息（节点层级与父子关系）
  - 动画关键帧数据（位置、旋转、缩放）
- 支持多个动画片段（AnimClip），每段包含时长、是否循环、插值策略
- 所有动画数据导入后储存至动画状态机中统一管理

### 🎭 状态迁移设计
- 使用 `AnimationStateMachine<State>` 模板类管理动画状态切换
- 每个状态包含：
  - 当前动画
  - 上一个动画
  - 混合时间
  - 可设置播放结束回调函数（使用函数指针）
- 状态间切换由状态机在 Update 时自动判断并执行

### 🔄 迁移条件的实现
- 每个状态可以指定一个条件函数（成员函数指针）
  - 函数类型：`bool (Character::*)() const`
- 动画状态机在更新时调用条件函数进行切换判断
- 如满足条件，自动进入目标状态并启动混合过程

### 💫 补间与混合实现
- 使用 `DirectX::XMVectorLerp` 插值位置向量
- 使用 `DirectX::XMQuaternionSlerp` 插值旋转四元数
- 骨骼矩阵的混合由状态机内部统一控制
- 支持逐帧平滑过渡，确保动画衔接自然


## 粒子系统

### 🔥 完全 GPU 驱动架构
- 所有粒子生命周期逻辑使用 Compute Shader 实现
- 分阶段处理：
  - `EmitCS`：生成新粒子
  - `UpdateCS`：更新粒子位置、速度、寿命
  - `SortCS`：GPU排序解决 AlphaBlend 闪烁
  - `DrawIndirect`：通过间接绘制提升性能

### 🧠 粒子管理结构
- 使用 FreeList 结构回收死亡粒子，并通过 `Consume()` 分配新粒子槽位
- `spawnRate` 控制每秒生成速率，非每个粒子自己复活

### 🌈 特性支持
- **帧动画（Texture Sheet / Flipbook）**
- **圆锥形（Cone）发射方向控制**
- **生命周期控制缩放和颜色变化**
- **SoftParticle 支持：根据距离渐隐**
- **DrawIndirect + GPU 排序，适配海量粒子渲染**

---

## 地形与碰撞

### 🧱 八叉树加速结构
- 使用 Octree 加速地形三角形查询
- 每个节点包含 AABB 与其子节点指针和三角面索引

### 🌾 草分布算法
- 草簇（Cluster）随机生成，非全图均匀分布
- 每个簇生成一个随机范围，在 Octree 中查询包含三角形
- 使用 Ray 测试获得草的准确落地位置
- 草会根据三角形法线自动旋转
- 斜率过陡则不生成草

---


## 模型导入与动画管理

### 📁 FBXLoader 的实现
- 支持解析 ASCII 格式的 `.fbx` 文件
- 解析内容包括：
  - 顶点信息：位置、法线、UV、切线
  - 索引列表
  - 骨骼结构：层级、绑定关系、节点名称
  - 动画数据：每根骨骼的关键帧动画（位置 / 旋转 / 缩放）
- 动画按片段导入为 `AnimClip`，并接入动画状态机进行混合播放

### 📦 模型管理器结构
- 使用 `ModelManager` / `SkinnedModelManager` 统一管理所有模型资源
- 通过路径 + 模型名哈希索引，避免重复加载
- 所有 GameObject 中的模型引用皆由此分发获取，确保资源唯一性

### 🚀 模型缓存系统
- 第一次加载 `.obj` 或 `.fbx` 时将解析结果缓存为 `.bin` 二进制文件
- 后续读取 `.bin` 文件可大幅提升加载速度
- 缓存机制支持纹理信息、索引数据、骨骼动画等完整数据序列化


## 光照与贴图槽位管理

### 🌞 光源控制
- 玩家：绑定可移动方向光，支持旋转
- 火把：绑定点光源，跟随位置变换

### 🎯 TextureSlotManager
- 管理所有贴图资源的绑定槽位：
  - g_Texture → t0
  - g_ShadowMap[x] → t1
  - g_NormalMap → t9 等
- 宏定义槽位名 + 状态记录，避免重复绑定

---

## Shader 管理系统

### 🎮 ShaderManager 架构
- 使用 ShaderID 枚举管理资源
- 同时管理 ShaderSet、ShadowShaderSet、ComputeShaderSet
- 支持热重载（Hot Reload）

### 🔁 热重载实现
- 使用 `(path, entry)` 作为组合键
- 对比 Shader Blob 哈希判断是否重新编译
- 使用 outputShaderPtr 避免双重释放

---

## 特效系统

### 🔥 FireEffectRenderer（火焰 + 烟雾）
- 使用火焰帧动画贴图播放火焰效果（Texture Sheet）
- 烟雾粒子可单独开关或与火焰同时显示
- 模块封装 `Update()` / `Draw()` 接口
- 支持 Additive、AlphaBlend 混合模式
- SoftParticle 烟雾效果与 Flipbook 支持

### 💡 特效架构规划
- 使用 ShaderGroup 枚举组织特效资源组
- 支持批处理渲染、统一绑定流程

---

## 敌人 AI 系统

### 👾 EnemyManager 管理
- 所有敌人由 EnemyManager 集中管理
- 负责生成、更新、移除、渲染等流程

### 🧠 行为树控制（Behavior Tree）
- 每个敌人使用一棵独立行为树
- 节点类型：
  - Selector：选择第一个成功的子节点执行
  - Sequence：按顺序执行子节点，失败即中断
- 示例节点组合：
  - 发现玩家 → 追击 → 攻击
  - 巡逻 → 玩家进入视野 → 切换攻击状态
- 条件判断包括距离、视野角度、障碍物检测等
- 动画与状态联动，自动切换移动、攻击、待机动画

---



## 异步架构系统

### 📦 异步任务与系统管理
- 所有异步任务由统一异步系统管理，支持模型与资源的异步加载。
- 提供两种任务绑定方式：
  - `DeferredTaskBinder<T>`：绑定类成员函数的结构化任务接口。
  - `RawDeferredTask`：灵活绑定任意函数指针或 lambda。

### 🔁 线程池机制
- 使用自制 `ThreadPool` 管理线程复用。
- 使用 Win32 原生 `CreateEvent` 实现线程任务唤醒与完成等待。
- 支持任务调度优先级、自定义 TaskOptions、延迟执行等机制。

### 🔒 线程安全支持
- 实现了轻量级自旋锁 `SpinLock` 和互斥锁 `LightweightMutex`。
- 给八叉树构造过程添加了锁支持，实现线程安全的多线程构建。

### 🧵 模型加载的异步化
- 模型加载任务分为多个阶段：
  1. 异步读取 `.obj` / `.fbx` 并解析结构体（顶点、索引、动画等）
  2. 创建资源缓存数据，等待主线程调度上传 GPU 资源
  3. 通过 DeferredTask 完成后的回调激活模型初始化流程

### 🖼 异步贴图加载
- 分离贴图加载为两阶段：
  - 子线程异步读取贴图数据并放入 `TextureUploadQueue`
  - 主线程调用 `WaitForSingleObject` 等待读取完成后创建 SRV
- 使用上传队列管理 `TextureUploadTask`，每个任务包含：
  - 路径、数据指针、输出 SRV 指针、完成回调函数
- 完成后通过回调更新贴图加载计数，支持资源热重载与分批上传。


---

## UI 系统（动画与场景集成）

### 🎞 UIAnimatedSprite 动画播放
- 新增 `UIAnimatedSprite` 用于播放帧动画 UI，例如加载动画、闪烁提示等。
- 支持 Texture Sheet、帧率控制、循环播放与自动淡出。

### 🌗 渐入渐出管理（ScreenFader）
- 将渐入/渐出效果从 UI 单体抽离成独立系统 `ScreenFader` 管理。
- 使用 `UIOverlayRenderer` 遮罩 + `alpha` 控制实现黑幕过渡。
- 提供自动淡入/淡出 API，可与任何场景切换联动。

### ⏳ 加载场景系统（UILoadingScene）
- 专用 UI 场景用于资源加载过程展示。
- 拥有内部状态机，根据加载进度与时长控制动画播放流程：
  - 初始状态：播放进入动画
  - 加载状态：显示 Loading + 进度动画
  - 结束状态：等待加载完成触发离开动画

### 🏠 标题场景与切换（UITitleScene）
- 实现完整的标题场景 UI（Logo、标题动画、按钮等）
- 与 `GameSystem` 整合，通过回车键触发淡出并切换模式。

### 🔁 Scene 与 GameMode 切换
- 改造 `Scene` 类以支持按 `SceneID` 管理模型加载与卸载。
- 切换 GameMode（如标题 → 游戏）时自动卸载旧场景模型并加载目标场景资源。
- 使用统一接口注册/销毁 GameObject 实例，避免内存泄露与重复资源。


## 游戏中枢控制

### 🧠 GameSystem 类与资源加载
- 管理游戏模式切换：标题 → 教程 → 游戏本体
- 调用并协调 Renderer、UI、Effect、Camera、敌人等各子系统
- 提供 ImGui 调试界面：显示包围盒、调整 CSM 参数等

- 支持按不同 GameMode（如标题/加载/游戏）进行资源预加载和卸载。
- 每个 GameMode 拥有对应的资源集（模型、贴图、UI），通过异步加载系统分阶段调度。
- 进入模式时触发异步加载任务，资源准备完毕后通知主线程切换场景。
- 退出模式时自动回收该模式注册的全部 GameObject 资源，避免占用。

---

---