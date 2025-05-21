# 🎮 bytime Game Engine

Welcome to the repository for ETSlime's custom DirectX 11 game engine, which is used to develop the game, bytime.  
This engine demonstrates advanced real-time rendering, animation blending, particle systems, behavior trees, and more.

---

## 📚 Multi-language Documentation

This project provides full technical documentation in three languages. Click below to view the full breakdown of all systems and implementations:

- 🇨🇳 [中文版开发笔记](./README_CN.md)  
  包含渲染系统、实例化、动画混合、粒子系统、地形碰撞、FBX模型导入、光照系统、Shader管理、UI系统、特效系统与敌人AI（行为树）等详细实现。

- 🇯🇵 [日本語版開発ノート](./README_JP.md)  
  DirectX11ベースのゲームエンジン構成と全モジュールの詳細実装を日本語で解説。全システムにわたる技術仕様を網羅。

- 🇺🇸 [English Development Notes](./README_EN.md)  
  A complete breakdown of the game engine architecture and implementation details in English, covering rendering, instancing, animation, terrain, effects, particles, AI and more.

---

## ✨ Features

- Forward rendering pipeline with CSM (Cascaded Shadow Maps)
- GPU-driven particle system (Emit, Update, Sort, DrawIndirect)
- Skinned animation with blending and state machine
- Behavior Tree AI system with Selector & Sequence nodes
- Instancing system with LOD support (planned)
- Custom FBX model loader and binary caching
- Resource managers: TextureSlotManager, ShaderManager, UIManager
- ImGui debug panel for real-time control and diagnostics

---
