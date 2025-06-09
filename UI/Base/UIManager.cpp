//=============================================================================
//
// UIElement処理 [UIElement.cpp]
// Author : 
//
//=============================================================================
#include "UI/Base/UIManager.h"
#include "UI/Base/UISpriteRenderer.h"
#include "UI/Base/ScreenFader.h"

void UIManager::Init(void)
{
    HRESULT hr = S_OK;

    // 頂点データを作成
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(UIVertex) * 4;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_renderer.GetDevice()->CreateBuffer(&bd, nullptr, &m_spriteVB);
    assert(SUCCEEDED(hr));

    m_overlay.Initialize(m_spriteVB);
    m_screenFader.Initialize();
}

void UIManager::Update(void)
{
    if (m_modal)
    {
        m_modal->Update(); // モーダルだけ更新
    }
    else
    {
        for (auto& e : m_elements)
            e->Update();
    }

    m_screenFader.Update();
}

void UIManager::Draw(void)
{
    // Z順でソート（必要なら）
    UINT numUIElements = m_elements.getSize();
    for (UINT i = 1; i < numUIElements; ++i)
    {
        auto key = m_elements[i];
        int j = static_cast<int>(i) - 1;

        while (j >= 0 && m_elements[j]->GetZOrder() > key->GetZOrder())
        {
            m_elements[j + 1] = m_elements[j];
            --j;
        }

        m_elements[j + 1] = key;
    }

    // UI要素を描画
    for (auto& e : m_elements)
        e->Draw();

    if (m_modal)
    {
        if (m_modal->HasCustomOverlay())
        {
            // カスタムオーバーレイを描画
            m_modal->DrawCustomOverlay();
        }
        else if (m_modal->NeedsOverlay())
        {
            // 半透明の黒い背景
            DrawOverlay(SCREEN_CENTER_X, SCREEN_CENTER_Y, SCREEN_WIDTH, SCREEN_HEIGHT, { 0, 0, 0, 0.5f });
        }

        m_modal->Draw();
    }

    m_screenFader.Draw();
}

void UIManager::AddElement(UIElement* element)
{
    m_elements.push_back(element);
}

void UIManager::SetModal(UIElement* modal)
{
    // 既存のモーダルを削除
    SAFE_DELETE(m_modal);
    // 新しいモーダルを設定
    m_modal = modal;
}

bool UIManager::IsModalActive() const
{
    return m_modal != nullptr;
}

void UIManager::ClearModal(void)
{
    SAFE_DELETE(m_modal);
}

void UIManager::DrawOverlay(float x, float y, float w, float h, XMFLOAT4 color)
{
    m_overlay.Draw(m_renderer.GetDeviceContext(), x, y, w, h, color);
}

void UIManager::Clear(void)
{
    for (UINT i = 0; i < m_elements.getSize(); ++i)
    {
        SAFE_DELETE(m_elements[i]); // 明示的解放
    }

    m_elements.clear();
}
