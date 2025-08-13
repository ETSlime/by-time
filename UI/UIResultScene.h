#pragma once
#include "UI/Base/UIElement.h"
#include "UI/Base/UIAnimatedSprite.h"
#include "Core/TextureMgr.h"
#include "Utility/InputManager.h"
#include "UI/Base/ResultType.h"
#include "UI/Base/UIManager.h"

// 前方宣言のみ
class GameSystem;
enum class GameMode;

class UIResultScene : public UIElement
{
public:
    UIResultScene(ResultType type);

    void Init();
    ~UIResultScene();

    void Update() override;
    void Draw() override;

    // フェードアウト開始状態を取得
    bool IsFadeOutStarted() const { return m_fadeOutStarted; }

private:
    ResultType m_resultType;
    UIAnimatedSprite* m_resultSprite = nullptr;
    bool m_fadeOutStarted = false;

    InputManager& m_input = InputManager::get_instance();

    DebugProc& m_debugProc = DebugProc::get_instance();
};
