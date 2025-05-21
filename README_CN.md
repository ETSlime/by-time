# 🧠 ETSlime 游戏引擎开发笔记（中文完整版）

> 本文档详细记录了 ETSlime 使用 DirectX11 开发游戏引擎的全部模块与实现方式  
> 技术栈：C++ / DirectX 11 / HLSL / Win32 / ImGui / Compute Shader

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
9. [UI 系统](#ui-系统)
10. [特效系统](#特效系统)
11. [敌人 AI 系统](#敌人-ai-系统)
12. [游戏中枢控制](#游戏中枢控制)

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

### 🎭 状态机结构
- 使用 `AnimationStateMachine<State>` 模板
- 以枚举类型 State 管理所有状态，提高效率
- 每个状态保存当前动画、上一个动画、混合时间与切换条件、回调函数等信息

### 🔄 状态切换条件
- 通过成员函数指针 `bool (Character::*)() const` 实现条件判断
- `GetNextState(ISkinnedMeshModelChar*)` 中传入角色指针用于动态调用逻辑判断

### 💫 动画混合
- 使用 `XMVectorLerp` 插值位置
- 使用 `XMQuaternionSlerp` 插值旋转
- 动画状态机自动管理骨骼混合矩阵的生成

---

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

### 📁 自制 FBXLoader
- 支持解析 ASCII 格式的 `.fbx` 文件
- 解析数据：
  - 顶点：位置、法线、UV、Tangent
  - 索引信息
  - 骨骼结构：节点层级与父子关系
  - 动画数据：每根骨骼的关键帧信息（位置、旋转、缩放）
- 所有动画按片段保存到 `AnimClip`，包含：
  - 动画长度
  - 是否循环
  - 插值方式
- 每个 AnimClip 可供状态机控制，切换播放并自动混合

---

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

## UI 系统

### 🖼 UI 管理器结构
- 使用 UIManager 管理所有 UIElement（图像、文字、进度条等）
- 提供统一的 SetSprite 函数族
- 支持界面显示/隐藏、更新控制、调试绘制等功能

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

## 游戏中枢控制

### 🧠 GameSystem 类
- 管理游戏模式切换：标题 → 教程 → 游戏本体
- 调用并协调 Renderer、UI、Effect、Camera、敌人等各子系统
- 提供 ImGui 调试界面：显示包围盒、调整 CSM 参数等

---
