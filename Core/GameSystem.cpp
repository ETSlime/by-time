//=============================================================================
//
// GameSystem処理 [GameSystem.cpp]
// Author : 
//
//=============================================================================
#include "Core/GameSystem.h"
#include "UI/PauseModal.h"
#include "Utility/Debugproc.h"
#include "Effects/EffectSystem.h"

void GameSystem::Init(void)
{
    m_effectSystem.Initialize(m_renderer.GetDevice(), m_renderer.GetDeviceContext()); // エフェクトシステムの初期化

    m_player = new Player(); // プレイヤーの初期化

    m_ground = new Ground(); // 地面の初期化

    m_skybox = new Skybox(); // Skyboxの初期化

    m_uiManager.Init(); // UIManagerの初期化

    m_enemyManager.Init(m_player); // エネミーの初期化

    m_cursorManager.Init(); // カーソルマネージャの初期化
}

void GameSystem::Shutdown(void)
{
    SAFE_DELETE(m_player); // プレイヤーの解放

    SAFE_DELETE(m_ground); // 地面の解放

    SAFE_DELETE(m_skybox); // Skyboxの解放

    m_uiManager.Uninit(); // UIManagerの解放

    m_effectSystem.Shutdown();
}

void GameSystem::Update(void)
{
    HandleInput();// 入力処理

    bool isPaused = IsPaused(); // ゲームが一時停止中かどうかを確認

    // ゲームが一時停止中の場合、モーダルの更新処理を行う
    if (!isPaused)
    {
        // モードによって処理を分ける
        switch (m_mode)
        {
        case GameMode::TITLE:
            UpdateTitle(); // タイトル画面の更新処理
            break;
        case GameMode::GAME:
            UpdateGame(); // ゲームの更新処理
            break;
        case GameMode::RESULT:
            UpdateResult(); // リザルト画面の更新処理
            break;
        }
    }

    m_uiManager.Update();
}

void GameSystem::Draw(void)
{
    // メインパスのビューポートを設定
    m_renderer.SetMainPassViewport();

    // モードによって処理を分ける
    switch (m_mode)
    {
    case GameMode::TITLE:
        DrawTitle(); // タイトル画面の描画処理
        break;
    case GameMode::GAME:
        DrawGame(); // ゲームの描画処理
        break;
    case GameMode::RESULT:
        DrawResult(); // リザルト画面の描画処理
        break;
    default:
        break;
    }

    // UIの描画
    //m_lightManager.SetLightEnable(FALSE); // ライティングを無効
    m_renderer.SetCullingMode(CULL_MODE_NONE); // カリングモードを無効に
    m_renderer.SetUIInputLayout(); // モデルの入力レイアウトを設定
    m_renderer.SetRenderUI(); // UIの描画を設定
    // 深度テストを無効に
    m_renderer.SetDepthMode(DepthMode::Disable);
    m_uiManager.Draw();
    // ライティングを有効に
    //m_lightManager.SetLightEnable(TRUE);
}

void GameSystem::RenderShadowPass(void)
{
    if (m_mode != GameMode::GAME)
        return;

    // シャドウマップのレンダリング
    const auto& lightList = m_lightManager.GetLightList();
    const auto& sceneObjects = m_scene.GetAllRenderableObjects();

    int lightIdx = 0;
    for (const auto& light : lightList)
    {
        // ライトの有効性を確認
        if (!light->GetLightData().Enable || lightIdx >= LIGHT_MAX) continue;

        // ディレクショナルライトの場合
        if (light->GetType() == LIGHT_TYPE::DIRECTIONAL)
        {
            m_shadowMapRenderer.RenderCSMForLight(static_cast<DirectionalLight*>(light), lightIdx, sceneObjects);
        }

        lightIdx++;
    }
}

void GameSystem::PauseGame(void)
{
    m_isPaused = true;    
    // カーソル位置を復元し、その後UI専用状態に切り替える
    m_cursorManager.RestoreCursorAndEnterUIExclusive(); 
    // ポーズモーダルを表示
    m_uiManager.SetModalIfNotExist<PauseModal>(); 

}

void GameSystem::ResumeGame(void)
{
    m_isPaused = false;
    m_cursorManager.EnterUIHidden(); // UI非表示モードに入る
    m_cursorManager.RememberCursorPosition(); // カーソル位置を保存
    m_inputManager.SetMouseRecentered(true); // カメラ用：次のフレームでマウス位置リセットを要求
}

void GameSystem::EnterDialogue(void)
{
    m_inDialogue = true;
}

void GameSystem::ExitDialogue(void)
{
    m_inDialogue = false;
}

void GameSystem::ToggleMiniMap()
{
    m_showMiniMap = !m_showMiniMap;
}

void GameSystem::SetMode(GameMode mode)
{
    m_mode = mode;
}

void GameSystem::HandleInput(void)
{
    HandlePauseInput();
}

void GameSystem::HandlePauseInput(void)
{
    if (m_inputManager.GetKeyboardTrigger(KEY_PAUSE) && m_mode == GameMode::GAME && !m_inDialogue)
    {
        // ゲームが一時停止中かどうかを確認
        if (IsPaused())
        {
            // ゲームを再開
            ResumeGame();
            m_uiManager.ClearModal();
        }
        else
        {
            // ゲームを一時停止
            PauseGame();
        }
    }
}

// 各モードの更新
void GameSystem::UpdateTitle(void)
{
    // タイトル画面のロジック
}

void GameSystem::UpdateGame(void)
{
    if (m_isPaused)
        return;

    m_isAltDown = m_inputManager.GetKeyboardPress(DIK_LALT) || m_inputManager.GetKeyboardPress(DIK_RALT);

    if (m_isAltDown)
        m_cursorManager.OnEnterVisibleTemp();
    else
        m_cursorManager.OnExitVisibleTemp();

    m_player->Update(); // プレイヤーの更新処理
    m_ground->Update(); // 地面の更新処理
    m_skybox->Update(); // Skyboxの更新処理
    m_enemyManager.Update(); // エネミーの更新処理
    m_effectSystem.Update(); // エフェクトの更新処理

#ifdef _DEBUG
    m_shaderManager.CollectAndReloadShaders(m_renderer.GetDevice());
#endif // !_ DEBUG
}

void GameSystem::UpdateResult(void)
{
    // リザルト画面のロジック
}

// 各モードの描画
void GameSystem::DrawTitle(void)
{
    // タイトル画面の描画
}

void GameSystem::DrawGame(void)
{
    // スカイボックスの描画
    m_skybox->Draw(XMLoadFloat4x4(&m_camera.GetViewMatrix()), XMLoadFloat4x4(&m_camera.GetProjMatrix()));

    m_renderer.SetDepthMode(DepthMode::Enable); // 深度テストを有効に
    m_renderer.SetCullingMode(CULL_MODE_BACK); // カリングモードを有効に

    m_renderer.SetStaticModelInputLayout(); // モデルの入力レイアウトを設定
    m_renderer.SetRenderObject(); // モデルの描画を設定

    // ライティングを無効
    m_lightManager.SetLightEnable(FALSE);
    m_renderer.SetRenderLayer(RenderLayer::LAYER_1); // UIの描画レイヤーを設定
    m_enemyManager.DrawUI(EnemyUIType::HPGauge); // HPゲージの描画
    m_renderer.SetRenderLayer(RenderLayer::DEFAULT);
    m_enemyManager.DrawUI(EnemyUIType::HPGaugeCover); // HPゲージカバーの描画
    // ライティングを有効に
    m_lightManager.SetLightEnable(TRUE);

    m_ground->Draw(); // 地面の描画

    
    m_renderer.SetSkinnedMeshInputLayout(); // スキニングメッシュの入力レイアウトを設定
    m_renderer.SetRenderSkinnedMeshModel(); // スキニングメッシュモデルの描画を設定
    m_player->Draw(); // プレイヤーの描画
    m_enemyManager.Draw(); // エネミーの描画
    m_ground->Draw(); // 地面の描画

    m_renderer.SetRenderInstance(); // インスタンスの描画を設定
    m_ground->Draw(); // 地面の描画

    m_renderer.SetVFXInputLayout(); // VFXの入力レイアウトを設定
    m_renderer.SetRenderVFX(); // VFXの描画を設定
    m_player->DrawEffect(); // プレイヤーのエフェクト描画

    m_renderer.SetDepthMode(DepthMode::Particle); // パーティクルの深度設定
    m_effectSystem.Draw(m_camera.GetViewProjMtx()); // エフェクトの描画
    m_renderer.SetBlendState(BLEND_MODE_ALPHABLEND); // ブレンドステートを設定

    //m_renderer.SetCullingMode(CULL_MODE_BACK);

    //SetOffScreenRender();
//DrawScene();

//SetLightEnable(FALSE);
//DrawOffScreenRender();
//SetLightEnable(FALSE);
}

void GameSystem::DrawResult(void)
{
    // リザルト画面の描画
}