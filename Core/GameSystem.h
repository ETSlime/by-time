//=============================================================================
//
// ゲーム進行・描画・システム状態を統括管理する中枢クラス [GameSystem.h]
// Author : 
// タイトル／ゲーム／リザルト各モードの状態遷移・更新・描画制御を行い、
// プレイヤー／敵／UI／カメラ／エフェクトなど全主要サブシステムを統括する
//
//=============================================================================
#pragma once
#include "main.h"
#include "Utility/SingletonBase.h"
#include "UI/UIManager.h"
#include "Core/Graphics/Renderer.h"
#include "Scene/Player.h"
#include "Scene/EnemyManager.h"
#include "Scene/Ground.h"
#include "Effects/Skybox.h"
#include "Core/Camera.h"
#include "Core/Graphics/ShadowMapRenderer.h"
#include "Core/CursorManager.h"
#include "Effects/EffectSystem.h"
#include "Utility/InputManager.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
// ゲームモードの列挙
enum class GameMode : int		// ゲームモード
{
    TITLE = 0,			        // タイトル画面
    TUTORIAL,			        // ゲーム説明画面
    GAME,				        // ゲーム画面
    RESULT,				        // リザルト画面
    MAX
};

// ゲーム全体の状態を管理するクラス
class GameSystem : public SingletonBase<GameSystem>
{
public:

    // 初期化と更新
    void Init(void);
    void Shutdown(void);
    void Update(void);
    void Draw(void);
    void RenderShadowPass(void);

    // ゲームの一時停止/再開
    void PauseGame(void);
    void ResumeGame(void);
    bool IsPaused(void) const { return m_isPaused; }
    bool IsAltDown(void) const { return m_isAltDown; } // Altキーが押されているかどうか

    // 会話モード切替
    void EnterDialogue(void);
    void ExitDialogue(void);
    bool IsInDialogue(void) const { return m_inDialogue; }

    // ミニマップ表示切替
    void ToggleMiniMap(void);
    bool IsMiniMapVisible(void) const { return m_showMiniMap; }

    // モード切り替え
    void SetMode(GameMode mode);
    GameMode GetMode() const { return m_mode; }

protected:
    // プライベートコンストラクタ
    GameSystem()
        : SingletonBase()
        , m_renderer(Renderer::get_instance())
        , m_lightManager(LightManager::get_instance())
		, m_inputManager(InputManager::get_instance())
        , m_uiManager(UIManager::get_instance())
        , m_enemyManager(EnemyManager::get_instance())
        , m_camera(Camera::get_instance())
        , m_scene(Scene::get_instance())
        , m_shadowMapRenderer(ShadowMapRenderer::get_instance())
        , m_cursorManager(CursorManager::get_instance())
        , m_effectSystem(EffectSystem::get_instance())
        , m_shaderManager(ShaderManager::get_instance()) {}

    // この行が絶対に必要！！
    // SingletonBase<GameSystem> が GameSystem の protected コンストラクタにアクセスできるようにするための特別な許可
    // テンプレートクラスは「自動的にフレンド」にはならないため、明示的に friend 宣言が必要
    friend class SingletonBase<GameSystem>;


private:
    // 内部状態
    bool m_isPaused = false;
    bool m_inDialogue = false;
    bool m_showMiniMap = true;
    bool m_isAltDown = false; // Altキーが押されているかどうか

    void HandleInput(void);
    void HandlePauseInput(void);

    GameMode m_mode = GameMode::GAME;

    Player* m_player = nullptr;
    Ground* m_ground = nullptr;
    Skybox* m_skybox = nullptr;

    UIManager& m_uiManager;
    EnemyManager& m_enemyManager;
    Renderer& m_renderer;
    LightManager& m_lightManager;
    Camera& m_camera;
    Scene& m_scene;
    ShadowMapRenderer& m_shadowMapRenderer;
    CursorManager& m_cursorManager;
    EffectSystem& m_effectSystem;
    ShaderManager& m_shaderManager;
    InputManager& m_inputManager;

    // モードごとの更新と描画関数
    void UpdateTitle(void);
    void UpdateGame(void);
    void UpdateResult(void);

    void DrawTitle(void);
    void DrawGame(void);
    void DrawResult(void);
};