#pragma once
//=============================================================================
//
// 一時停止の魔法をかけるモーダルちゃん [PauseModal.h]
// Author : 
// ポーズ中に画面全体をふんわりオーバーレイで包んでくれる可愛いUIクラスですっ
// 背景をやさしく遮って、まるで時間が止まったみたいな演出をしてくれるの
// 
//=============================================================================
#include "UI/Base/UIElement.h"
#include "UI/Base/UIManager.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define SAVE_COMPLETE_MESSAGE_TEXTURE_PATH		"data/TEXTURE/UI/gamesave_.png"
#define SAVE_COMPLETE_MESSAGE_TEXTURE_POS_X		SCREEN_CENTER_X
#define SAVE_COMPLETE_MESSAGE_TEXTURE_POS_Y		SCREEN_CENTER_Y * 0.75f // 画面中央の少し上に配置
#define SAVE_COMPLETE_MESSAGE_TEXTURE_SIZE_X	2177.0f
#define SAVE_COMPLETE_MESSAGE_TEXTURE_SIZE_Y	387.0f

#define SAVE_COMPLETE_BTN_TEXTURE_PATH			"data/TEXTURE/UI/gamesaveok.png"
#define SAVE_COMPLETE_BTN_TEXTURE_POS_X			SCREEN_CENTER_X
#define SAVE_COMPLETE_BTN_TEXTURE_POS_Y			SCREEN_CENTER_Y * 1.3f // 画面中央の少し下に配置
#define SAVE_COMPLETE_BTN_TEXTURE_SIZE_X		457.0f
#define SAVE_COMPLETE_BTN_TEXTURE_SIZE_Y		275.0f

class SaveCompleteModal : public UIElement
{
public:
    SaveCompleteModal()
    {
		m_completeMessage = new UISprite(
			UIManager::get_instance().GetSpriteVertexBuffer(),
			TextureMgr::get_instance().CreateTexture(SAVE_COMPLETE_MESSAGE_TEXTURE_PATH));
		m_completeMessage->SetPosition(SAVE_COMPLETE_MESSAGE_TEXTURE_POS_X, SAVE_COMPLETE_MESSAGE_TEXTURE_POS_Y);
		m_completeMessage->SetSize(SAVE_COMPLETE_MESSAGE_TEXTURE_SIZE_X, SAVE_COMPLETE_MESSAGE_TEXTURE_SIZE_Y);
		m_completeMessage->SetScale(0.35f, 0.35f);

		m_closeButton = new UIButton(
			{ SAVE_COMPLETE_BTN_TEXTURE_POS_X, SAVE_COMPLETE_BTN_TEXTURE_POS_Y },
			{ SAVE_COMPLETE_BTN_TEXTURE_SIZE_X, SAVE_COMPLETE_BTN_TEXTURE_SIZE_Y }, "OK");
		m_closeButton->InitWithSingleTexture(
			UIManager::get_instance().GetSpriteVertexBuffer(),
			SAVE_COMPLETE_BTN_TEXTURE_PATH,
			XMFLOAT4(1, 1, 1, 1), // Normal
			XMFLOAT4(0.8f, 0.8f, 0.8f, 1), // Hover
			XMFLOAT4(0.6f, 0.6f, 0.6f, 1), // Pressed
			XMFLOAT4(0.5f, 0.5f, 0.5f, 1) // Disabled
		);
		m_closeButton->SetScale(0.4f, 0.4f);
		m_closeButton->SetOnClick(OnCloseClicked, nullptr);

    }

	~SaveCompleteModal()
	{
		SAFE_DELETE(m_completeMessage);
		SAFE_DELETE(m_closeButton);
	}

    void Update(void) override 
	{
		if (m_closeButton) m_closeButton->Update();
	}

    void Draw(void) override 
	{
		if (m_completeMessage) m_completeMessage->Draw();
		if (m_closeButton) m_closeButton->Draw();
	}

	bool NeedsOverlay(void) const override { return true; }

private:
	static void OnCloseClicked(void* userData)
	{
		UIManager::get_instance().PopModal();
	}

	UISprite* m_completeMessage = nullptr; // 背景スプライト
	UIButton* m_closeButton = nullptr; // 閉じるボタン

};

