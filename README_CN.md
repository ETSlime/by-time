#  ETSlime 游戏引擎开发笔记（中文完整版）

> 本文档详细记录了 ETSlime 使用 DirectX11 开发游戏引擎的全部模块与实现方式  
> 技术栈：C++ / DirectX 11 / HLSL / Win32 / ImGui / Compute Shader / Async Task System

---

##  目录

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

###  Forward 渲染结构
- 使用 Forward 渲染方式（非延迟渲染）
- 渲染流程分为多个 Pass：
  - Main Pass 主渲染通道
  - Shadow Pass 阴影渲染通道（最多支持 4 层级 CSM）
- 支持渲染对象：
  - 静态模型
  - 骨骼动画模型
  - 实例化对象

###  CSM 阴影系统
- 实现最大 4 层级的级联阴影贴图
- 自动计算 LightViewProj 阵
- 支持 Texel 对齐精度处理（Snap to Texel）
- 可视化调试 Frustum 区域
- 使用 ShadowMapRenderer 区分静态/骨骼/实例化模型着色器资源
- 利用 AABB 判断每个物体所属层级，并支持跨层冗余上传

---

## 实例化渲染

###  支持的对象类型
- 草（带风摆动效果）
- 树（不摆动）
- 岩石、花、灌木等未来扩展对象

###  技术细节
- 每种对象拥有独立的 InstanceBuffer 和 InputLayout
- 草使用 `GrassVertex`，包含每个顶点的 Weight 控制风力影响
- 每个实例的数据包含位移、缩放、旋转矩阵等
- 使用 `D3D11_USAGE_DYNAMIC` + `Map/Unmap` 实现动态上传
- 剔除后实例数据保存在 `SimpleArray<InstanceData>* visibleInstances` 中

###  支持 LOD
- 为每种对象提供多个 LOD InstanceBuffer
- 每帧根据相机距离切换 LOD
- 每个 LOD 层分别调用 `DrawIndexedInstanced()`

---


## 动画系统

###  FBXLoader 的实现
- 自制 `FBXLoader` 支持解析 ASCII 格式的 `.fbx` 文件
- 可提取数据：
  - 顶点属性（位置、法线、UV、切线）
  - 索引列表
  - 骨骼信息（节点层级与父子关系）
  - 动画关键帧数据（位置、旋转、缩放）
- 支持多个动画片段（AnimClip），每段包含时长、是否循环、插值策略
- 所有动画数据导入后储存至动画状态机中统一管理

###  状态迁移设计
- 使用 `AnimationStateMachine<State>` 模板类管理动画状态切换
- 每个状态包含：
  - 当前动画
  - 上一个动画
  - 混合时间
  - 可设置播放结束回调函数（使用函数指针）
- 状态间切换由状态机在 Update 时自动判断并执行

###  迁移条件的实现
- 每个状态可以指定一个条件函数（成员函数指针）
  - 函数类型：`bool (Character::*)() const`
- 动画状态机在更新时调用条件函数进行切换判断
- 如满足条件，自动进入目标状态并启动混合过程

###  补间与混合实现
- 使用 `DirectX::XMVectorLerp` 插值位置向量
- 使用 `DirectX::XMQuaternionSlerp` 插值旋转四元数
- 骨骼矩阵的混合由状态机内部统一控制
- 支持逐帧平滑过渡，确保动画衔接自然


## 粒子系统

###  完全 GPU 驱动架构
- 所有粒子生命周期逻辑使用 Compute Shader 实现
- 分阶段处理：
  - `EmitCS`：生成新粒子
  - `UpdateCS`：更新粒子位置、速度、寿命
  - `SortCS`：GPU排序解决 AlphaBlend 闪烁
  - `DrawIndirect`：通过间接绘制提升性能

###  粒子管理结构
- 使用 FreeList 结构回收死亡粒子，并通过 `Consume()` 分配新粒子槽位
- `spawnRate` 控制每秒生成速率，非每个粒子自己复活

###  特性支持
- **帧动画（Texture Sheet / Flipbook）**
- **圆锥形（Cone）发射方向控制**
- **生命周期控制缩放和颜色变化**
- **SoftParticle 支持：根据距离渐隐**
- **DrawIndirect + GPU 排序，适配海量粒子渲染**

---

## 地形与碰撞

###  八叉树加速结构
- 使用 Octree 对场景地形三角形进行空间划分，加速指定区域内的三角形查询
- 每个节点包含 AABB、子节点与对应的三角面索引
- 支持动态、延迟及异步构建，减少大型场景初始化时对主线程的阻塞
- 可用于地形区域查询、Ray Casting、物体放置等 CPU 侧空间检索

###  Sparse Voxel Grid
- 提供基于 **Sparse Voxel Grid** 的场景三角形空间索引，仅存储实际包含几何体的活跃体素
- 使用 **64 位 Morton 编码**组织空间数据，并通过排序、分页索引与二分查找快速定位候选三角形
- 场景三角形可上传至 `StructuredBuffer`，支持 Compute Shader 在 GPU 上直接执行空间查询与精确碰撞检测
- 作为通用场景几何查询结构，可供粒子模拟、Ray / Triangle 碰撞、弹道预测、激光检测等上层系统复用
- 水体模拟中的具体 Voxel Grid 碰撞流程参见独立的 **Water Simulation** 技术文档

###  草分布算法
- 草簇（Cluster）随机生成，非全图均匀分布
- 每个 Cluster 生成随机范围，并通过 Octree 查询范围内的候选地形三角形
- 使用 Ray 测试获得草实例的准确落地位置
- 根据三角形法线自动调整草的朝向
- 对地形坡度进行判断，斜率过大的区域不生成草

---


## 模型导入与动画管理

###  FBXLoader 的实现
- 支持解析 ASCII 格式的 `.fbx` 文件
- 解析内容包括：
  - 顶点信息：位置、法线、UV、切线
  - 索引列表
  - 骨骼结构：层级、绑定关系、节点名称
  - 动画数据：每根骨骼的关键帧动画（位置 / 旋转 / 缩放）
- 动画按片段导入为 `AnimClip`，并接入动画状态机进行混合播放

###  模型管理器结构
- 使用 `ModelManager` / `SkinnedModelManager` 统一管理所有模型资源
- 通过路径 + 模型名哈希索引，避免重复加载
- 所有 GameObject 中的模型引用皆由此分发获取，确保资源唯一性

### 模型缓存系统
- 第一次加载 `.obj` 或 `.fbx` 时将解析结果缓存为 `.bin` 二进制文件
- 后续读取 `.bin` 文件可大幅提升加载速度
- 缓存机制支持纹理信息、索引数据、骨骼动画等完整数据序列化


## 光照与贴图槽位管理

###  光源控制
- 玩家：绑定可移动方向光，支持旋转
- 火把：绑定点光源，跟随位置变换

###  TextureSlotManager
- 管理所有贴图资源的绑定槽位：
  - g_Texture → t0
  - g_ShadowMap[x] → t1
  - g_NormalMap → t9 等
- 宏定义槽位名 + 状态记录，避免重复绑定

---

## Shader 管理系统

###  ShaderManager 架构
- 使用 ShaderID 枚举管理资源
- 同时管理 ShaderSet、ShadowShaderSet、ComputeShaderSet
- 支持热重载（Hot Reload）

###  热重载实现
- 使用 `(path, entry)` 作为组合键
- 对比 Shader Blob 哈希判断是否重新编译
- 使用 outputShaderPtr 避免双重释放

---

## 特效系统

### FireEffectRenderer（火焰 + 烟雾）
- 使用帧动画贴图（Texture Sheet）播放火焰效果
- 烟雾通过粒子系统进行渲染，可独立开启或与火焰同时显示
- 对外统一提供 `Update()` / `Draw()` 接口
- 支持 Additive、AlphaBlend 等混合模式
- 支持 SoftParticle 与 Flipbook 帧动画混合

### 粒子与地形碰撞（Falling.hlsl）
- 使用 Compute Shader 在 GPU 上完成粒子发射、速度积分与位置更新，通过 AliveList / FreeList 管理有效粒子和可复用槽位
- 将预测位置转换到世界空间，根据体素坐标生成 Morton 编码，查询体素关联的三角形索引，缩小碰撞检测范围
- 以当前帧的位移构造世界空间射线，并将检测距离限制在本帧移动范围内，对候选三角形执行相交测试
- 命中后沿表面法线添加偏移以修正粒子位置，并将速度清零，实现粒子与地形表面的接触响应
- 使用 Geometry Shader 将粒子展开为面向摄像机的 Billboard，结合圆形裁剪与透明度渐变绘制柔和的粒子外观

### 水面渲染（LakeSurface.hlsl）
- 叠加两张沿不同方向和速度滚动的法线贴图，生成动态水面细节
- 在 Vertex Shader 中采样法线贴图并对顶点施加位移，结合 Pixel Shader 的法线扰动表现水面起伏
- 通过 `g_WaveStrength` 统一控制顶点位移幅度与着色法线的扰动程度；设为 0 时关闭位移，并使用平面法线，形成平静表面状态
- 根据视线方向与表面法线计算 Fresnel 权重，用于混合基础水色与高亮色，并调整透明度
- 通过常量缓冲区配置水色、波动强度、Fresnel 偏置与指数等参数

### 程序化激光光束（Laser.hlsl）
- 接收 CPU 提供的世界空间带状顶点，以长度方向和横向坐标在 Pixel Shader 中构造光束外观
- 组合 Core、Body 与 Rim 三种亮度分布，表现中心亮芯、主体与边缘光晕
- 使用随时间变化的正弦函数调制轴向亮度，产生沿光束流动的动态纹理，无需采样纹理
- 对光束首尾应用平滑衰减，减轻端点的硬边
- 分别计算颜色亮度与透明度权重，并提供最低透明度以保留可见轮廓；支持 HDR 基础颜色、流动速度和中心亮芯锐度调节

### 贴花系统（Decal.hlsl）
- 使用实例数据配置贴花的位置、尺寸、表面法线、旋转角度和生命周期，支持弹痕、爆炸痕迹与光束接触痕迹等表面效果
- 根据表面法线构建局部坐标基，将平面贴花定向到接触表面
- 根据法线与视线方向施加位置偏移，减轻贴花与表面重叠造成的 Z-fighting
- 使用 4×2 Texture Atlas，通过各实例的 UV 偏移选择不同的贴花图案
- 根据经过时间与生命周期计算透明度衰减，使痕迹逐渐淡出

### 弹道与作用范围预览（ProjectilePreview.hlsl）
- 通过 `g_RenderType` 在同一 Shader 内切换弹道轨迹与半球形范围的描画
- 接收外部提供的轨迹顶点，并利用顶点法线的 Y 分量作为透明度调制参数，形成轨迹的透明度渐变
- 以传入的命中位置为中心，对半球网格进行缩放，显示碰撞位置附近的预期作用范围
- 使用随时间变化的正弦函数控制半球尺寸与透明度，形成周期性的呼吸动画
- 根据片元到命中位置的距离计算透明度衰减，并使衰减范围随半球缩放同步变化
- 通过常量缓冲区统一配置命中位置、范围尺寸、基础颜色、时间与绘制类型

### ShaderGroup 与粒子特效架构
- 使用 `ShaderGroup` 枚举对相关特效 Shader 与资源进行分组管理
- 支持批处理渲染与统一的资源绑定流程
- 使用 `ParticleEffectRendererBase` 抽象粒子特效的公共 GPU Pipeline，为不同粒子效果提供统一的模拟与渲染框架：
  - 封装 `BufferSet` 的创建与管理，统一处理粒子 `StructuredBuffer`、`SRV` 与 `UAV` 等 GPU 资源
  - 在 `Update()` 阶段统一绑定资源并 Dispatch Compute Shader，执行粒子的 Emit / Update 等 GPU 模拟流程
  - 封装不同 Pipeline 阶段下 `StructuredBuffer`、`SRV`、`UAV` 的绑定与解绑，避免资源读写冲突
  - 为绘制阶段准备粒子 Buffer，并统一管理 Compute Shader 与 Graphics Pipeline 之间的资源切换
  - 抽象 `DrawParticles()` 接口，由派生类实现具体的粒子绘制方式，例如 Billboard、Flipbook 等
- 不同粒子特效可复用相同的底层 Buffer 与 Compute Shader 调度框架，仅需实现自身的模拟参数和绘制逻辑

---

## 敌人 AI 系统

###  EnemyManager 管理
- 所有敌人由 EnemyManager 集中管理
- 负责生成、更新、移除、渲染等流程

###  行为树控制（Behavior Tree）
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

###  异步任务与系统管理
- 所有异步任务由统一异步系统管理，支持模型与资源的异步加载。
- 提供两种任务绑定方式：
  - `DeferredTaskBinder<T>`：绑定类成员函数的结构化任务接口。
  - `RawDeferredTask`：灵活绑定任意函数指针或 lambda。

###  线程池机制
- 使用自制 `ThreadPool` 管理线程复用。
- 使用 Win32 原生 `CreateEvent` 实现线程任务唤醒与完成等待。
- 支持任务调度优先级、自定义 TaskOptions、延迟执行等机制。

###  线程安全支持
- 实现了轻量级自旋锁 `SpinLock` 和互斥锁 `LightweightMutex`。
- 给八叉树构造过程添加了锁支持，实现线程安全的多线程构建。

###  模型加载的异步化
- 模型加载任务分为多个阶段：
  1. 异步读取 `.obj` / `.fbx` 并解析结构体（顶点、索引、动画等）
  2. 创建资源缓存数据，等待主线程调度上传 GPU 资源
  3. 通过 DeferredTask 完成后的回调激活模型初始化流程

###  异步贴图加载
- 分离贴图加载为两阶段：
  - 子线程异步读取贴图数据并放入 `TextureUploadQueue`
  - 主线程调用 `WaitForSingleObject` 等待读取完成后创建 SRV
- 使用上传队列管理 `TextureUploadTask`，每个任务包含：
  - 路径、数据指针、输出 SRV 指针、完成回调函数
- 完成后通过回调更新贴图加载计数，支持资源热重载与分批上传。


---

## UI 系统（动画与场景集成）

###  UIAnimatedSprite 动画播放
- 新增 `UIAnimatedSprite` 用于播放帧动画 UI，例如加载动画、闪烁提示等。
- 支持 Texture Sheet、帧率控制、循环播放与自动淡出。

###  渐入渐出管理（ScreenFader）
- 将渐入/渐出效果从 UI 单体抽离成独立系统 `ScreenFader` 管理。
- 使用 `UIOverlayRenderer` 遮罩 + `alpha` 控制实现黑幕过渡。
- 提供自动淡入/淡出 API，可与任何场景切换联动。

###  加载场景系统（UILoadingScene）
- 专用 UI 场景用于资源加载过程展示。
- 拥有内部状态机，根据加载进度与时长控制动画播放流程：
  - 初始状态：播放进入动画
  - 加载状态：显示 Loading + 进度动画
  - 结束状态：等待加载完成触发离开动画

###  标题场景与切换（UITitleScene）
- 实现完整的标题场景 UI（Logo、标题动画、按钮等）
- 与 `GameSystem` 整合，通过回车键触发淡出并切换模式。

###  Scene 与 GameMode 切换
- 改造 `Scene` 类以支持按 `SceneID` 管理模型加载与卸载。
- 切换 GameMode（如标题 → 游戏）时自动卸载旧场景模型并加载目标场景资源。
- 使用统一接口注册/销毁 GameObject 实例，避免内存泄露与重复资源。


## 游戏中枢控制

###  GameSystem 类与资源加载
- 管理游戏模式切换：标题 → 教程 → 游戏本体
- 调用并协调 Renderer、UI、Effect、Camera、敌人等各子系统
- 提供 ImGui 调试界面：显示包围盒、调整 CSM 参数等

- 支持按不同 GameMode（如标题/加载/游戏）进行资源预加载和卸载。
- 每个 GameMode 拥有对应的资源集（模型、贴图、UI），通过异步加载系统分阶段调度。
- 进入模式时触发异步加载任务，资源准备完毕后通知主线程切换场景。
- 退出模式时自动回收该模式注册的全部 GameObject 资源，避免占用。

---

---