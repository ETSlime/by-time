#pragma once
//=============================================================================
//
// すべてのUIちゃんを統括する管理お姉さま [UIManager.h]
// Author : 
// UI要素をまとめて管理・描画・更新し、モーダルの制御やオーバーレイ描画もしてくれる頼れるクラスですっ！
// UI界の女王さまみたいな存在で、みんながスムーズに動けるように支えてくれてるの~
//
//=============================================================================
#include "main.h"
#include "Utility/SimpleArray.h"
#include "Utility/SingletonBase.h"
#include "UI/Base/UIElement.h"
#include "UI/Base/UIOverlayRenderer.h"
#include "UI/Base/ScreenFader.h"

class UIManager : public SingletonBase<UIManager>
{
public:
    UIManager() : m_modal(nullptr), m_spriteVB(nullptr) {}

    void Init(void);
    void Uninit(void) {Clear();}
    void Update(void);
    void Draw(void);
    void Clear(void);
    void AddElement(UIElement* element);

    template<typename T>
    void SetModalIfNotExist()
    {
        if (dynamic_cast<T*>(m_modal) == nullptr)
        {
            if (m_modal)
                delete m_modal;
            m_modal = new T();
        }
    }

    void SetModal(UIElement* modal);
    bool IsModalActive(void) const;
    void ClearModal(void);

    void DrawOverlay(float x, float y, float w, float h, XMFLOAT4 color);

    ID3D11Buffer* GetSpriteVertexBuffer(void) const { return m_spriteVB; }
    ScreenFader& GetFader(void) { return m_screenFader; }

private:
    SimpleArray<UIElement*> m_elements; // UI要素の配列
    UIElement* m_modal; // モーダルUIの保持
    UIOverlayRenderer m_overlay; // UIオーバーレイ描画
    ScreenFader m_screenFader;
    ID3D11Buffer* m_spriteVB;
    Renderer& m_renderer = Renderer::get_instance();
};