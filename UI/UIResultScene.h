#pragma once
#include "UI/Base/UIElement.h"
#include "UI/Base/UIAnimatedSprite.h"
#include "Core/TextureMgr.h"
#include "Utility/InputManager.h"
#include "UI/Base/ResultType.h"
#include "UI/Base/UIManager.h"

// ëOï˚êÈåæÇÃÇ›
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

    // UIResultScene.h Ç…
    bool IsRequestChangeMode() const { return m_fadeOutStarted && UIManager::get_instance().GetFader().IsFadeOutComplete(); }

private:
    ResultType m_resultType;
    UIAnimatedSprite* m_resultSprite = nullptr;
    bool m_fadeOutStarted = false;

    InputManager& m_input = InputManager::get_instance();

    DebugProc& m_debugProc = DebugProc::get_instance();
};
