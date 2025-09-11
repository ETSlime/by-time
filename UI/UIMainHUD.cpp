//=============================================================================
//
// [UIMainHUD.h]
// Author : 
// 
//=============================================================================
#include "UI/UIMainHUD.h"
#include "UI/Base/UIManager.h"
#include "Core/TextureMgr.h"
#include "Core/GameSystem.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define HUD_EQUIPMENT_SLOT_TEXTURE_PATH			"data/TEXTURE/UI/ui_weapon.png"
#define HUD_EQUIPMENT_SLOT_TEXTURE_POS_X		SCREEN_CENTER_X * 1.75f
#define HUD_EQUIPMENT_SLOT_TEXTURE_POS_Y		SCREEN_CENTER_Y * 1.5f
#define HUD_EQUIPMENT_SLOT_TEXTURE_SIZE_X		544.0f
#define HUD_EQUIPMENT_SLOT_TEXTURE_SIZE_Y		435.0f

#define HUD_WEAPON_ICON_TEXTURE_PATH			"data/TEXTURE/UI/weapon.png"
#define HUD_MAIN_WEAPON_ICON_TEXTURE_POS_X		HUD_EQUIPMENT_SLOT_TEXTURE_POS_X * 1.04f
#define HUD_MAIN_WEAPON_ICON_TEXTURE_POS_Y		HUD_EQUIPMENT_SLOT_TEXTURE_POS_Y * 1.05f
#define HUD_MAIN_WEAPON_ICON_TEXTURE_SCALE		0.1f
#define HUD_SUB_WEAPON_ICON_TEXTURE_POS_X		HUD_EQUIPMENT_SLOT_TEXTURE_POS_X * 1.08f
#define HUD_SUB_WEAPON_ICON_TEXTURE_POS_Y		HUD_EQUIPMENT_SLOT_TEXTURE_POS_Y * 0.85f
#define HUD_SUB_WEAPON_ICON_TEXTURE_SCALE		0.04f
#define HUD_WEAPON_ICON_TEXTURE_SIZE_X			2186.0f
#define HUD_WEAPON_ICON_TEXTURE_SIZE_Y			798.0f

#define HUD_GUN_ICON_TEXTURE_PATH				"data/TEXTURE/UI/gun.png"
#define HUD_MAIN_GUN_ICON_TEXTURE_POS_X			HUD_EQUIPMENT_SLOT_TEXTURE_POS_X * 1.04f
#define HUD_MAIN_GUN_ICON_TEXTURE_POS_Y			HUD_EQUIPMENT_SLOT_TEXTURE_POS_Y * 1.05f
#define HUD_MAIN_GUN_ICON_TEXTURE_SCALE			0.17f
#define HUD_SUB_GUN_ICON_TEXTURE_POS_X			HUD_EQUIPMENT_SLOT_TEXTURE_POS_X * 1.08f
#define HUD_SUB_GUN_ICON_TEXTURE_POS_Y			HUD_EQUIPMENT_SLOT_TEXTURE_POS_Y * 0.85f
#define HUD_SUB_GUN_ICON_TEXTURE_SCALE			0.08f
#define HUD_GUN_ICON_TEXTURE_SIZE_X				1197.0f
#define HUD_GUN_ICON_TEXTURE_SIZE_Y				658.0f

#define HUD_SEED_ICON_TEXTURE_PATH				"data/TEXTURE/UI/seed.png"
#define HUD_SEED_ICON_TEXTURE_POS_X				HUD_EQUIPMENT_SLOT_TEXTURE_POS_X * 0.917f
#define HUD_SEED_ICON_TEXTURE_POS_Y				HUD_EQUIPMENT_SLOT_TEXTURE_POS_Y * 1.13f
#define HUD_SEED_ICON_TEXTURE_SIZE_X			1128.0f
#define HUD_SEED_ICON_TEXTURE_SIZE_Y			1394.0f

#define HUD_BULLET_ICON_TEXTURE_PATH			"data/TEXTURE/UI/bullet.png"
#define HUD_BULLET_ICON_TEXTURE_POS_X			HUD_EQUIPMENT_SLOT_TEXTURE_POS_X * 0.917f
#define HUD_BULLET_ICON_TEXTURE_POS_Y			HUD_EQUIPMENT_SLOT_TEXTURE_POS_Y * 1.0f
#define HUD_BULLET_ICON_TEXTURE_SIZE_X			1497.0f
#define HUD_BULLET_ICON_TEXTURE_SIZE_Y			1364.0f

#define HUD_CROSSHAIR_TEXTURE_PATH				"data/TEXTURE/UI/crosshair.png"
#define HUD_CROSSHAIR_TEXTURE_SIZE_X			48.0f
#define HUD_CROSSHAIR_TEXTURE_SIZE_Y			48.0f

#define HUD_ITEM_COUNT_TEXTURE_PATH				"data/TEXTURE/UI/number.png"
#define HUD_ITEM_COUNT_SIGN_TEXTURE_PATH		"data/TEXTURE/UI/numberSign.png"
#define HUD_ITEM_COUNT_TEXTURE_SIZE_X			16.0f
#define HUD_ITEM_COUNT_TEXTURE_SIZE_Y			24.0f
#define HUD_SEED_COUNT_TEXTURE_POS_X			HUD_SEED_ICON_TEXTURE_POS_X * 1.03f
#define HUD_SEED_COUNT_TEXTURE_POS_Y			HUD_SEED_ICON_TEXTURE_POS_Y * 0.96f
#define HUD_BULLET_COUNT_TEXTURE_POS_X			HUD_BULLET_ICON_TEXTURE_POS_X * 1.03f
#define HUD_BULLET_COUNT_TEXTURE_POS_Y			HUD_BULLET_ICON_TEXTURE_POS_Y * 0.96f
#define ITEM_COUNT_DIGIT						2			// 桁数

UIMainHUD::~UIMainHUD()
{
	for (UINT i = 0; i < m_sprites.getSize(); ++i)
	{
		if (m_sprites[i])
		{
			SAFE_DELETE(m_sprites[i]);
		}
	}
}

void UIMainHUD::Initialize(void)
{
	m_sprites.reserve(12);

	m_player = GameSystem::get_instance().GetPlayer();

	m_equipSlot = new UISprite(
		UIManager::get_instance().GetSpriteVertexBuffer(),
		TextureMgr::get_instance().CreateTexture(HUD_EQUIPMENT_SLOT_TEXTURE_PATH)
	);
	m_equipSlot->SetSize(HUD_EQUIPMENT_SLOT_TEXTURE_SIZE_X, HUD_EQUIPMENT_SLOT_TEXTURE_SIZE_Y);
	m_equipSlot->SetPosition(HUD_EQUIPMENT_SLOT_TEXTURE_POS_X, HUD_EQUIPMENT_SLOT_TEXTURE_POS_Y);
	m_equipSlot->SetScale(0.82f, 0.82f);
	m_equipSlot->SetColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)); // 半透明に設定
	m_sprites.push_back(m_equipSlot);

	m_weaponIcon = new UISprite(
		UIManager::get_instance().GetSpriteVertexBuffer(),
		TextureMgr::get_instance().CreateTexture(HUD_WEAPON_ICON_TEXTURE_PATH)
	);
	m_weaponIcon->SetSize(HUD_WEAPON_ICON_TEXTURE_SIZE_X, HUD_WEAPON_ICON_TEXTURE_SIZE_Y);
	m_weaponIcon->SetPosition(HUD_MAIN_WEAPON_ICON_TEXTURE_POS_X, HUD_MAIN_WEAPON_ICON_TEXTURE_POS_Y);
	m_weaponIcon->SetScale(HUD_MAIN_WEAPON_ICON_TEXTURE_SCALE, HUD_MAIN_WEAPON_ICON_TEXTURE_SCALE);
	m_sprites.push_back(m_weaponIcon);

	m_gunIcon = new UISprite(
		UIManager::get_instance().GetSpriteVertexBuffer(),
		TextureMgr::get_instance().CreateTexture(HUD_GUN_ICON_TEXTURE_PATH)
	);
	m_gunIcon->SetSize(HUD_GUN_ICON_TEXTURE_SIZE_X, HUD_GUN_ICON_TEXTURE_SIZE_Y);
	m_gunIcon->SetPosition(HUD_SUB_GUN_ICON_TEXTURE_POS_X, HUD_SUB_GUN_ICON_TEXTURE_POS_Y);
	m_gunIcon->SetScale(HUD_SUB_GUN_ICON_TEXTURE_SCALE, HUD_SUB_GUN_ICON_TEXTURE_SCALE);
	m_sprites.push_back(m_gunIcon);

	m_seedIcon = new UISprite(
		UIManager::get_instance().GetSpriteVertexBuffer(),
		TextureMgr::get_instance().CreateTexture(HUD_SEED_ICON_TEXTURE_PATH)
	);
	m_seedIcon->SetSize(HUD_SEED_ICON_TEXTURE_SIZE_X, HUD_SEED_ICON_TEXTURE_SIZE_Y);
	m_seedIcon->SetPosition(HUD_SEED_ICON_TEXTURE_POS_X, HUD_SEED_ICON_TEXTURE_POS_Y);
	m_seedIcon->SetRotation(0.3f);
	m_seedIcon->SetScale(0.056f, 0.056f);
	//m_seedIcon->SetDrawMode(SpriteDrawMode::CenterWithRotation);
	m_sprites.push_back(m_seedIcon);

	m_bulletIcon = new UISprite(
		UIManager::get_instance().GetSpriteVertexBuffer(),
		TextureMgr::get_instance().CreateTexture(HUD_BULLET_ICON_TEXTURE_PATH)
	);
	m_bulletIcon->SetSize(HUD_BULLET_ICON_TEXTURE_SIZE_X, HUD_BULLET_ICON_TEXTURE_SIZE_Y);
	m_bulletIcon->SetPosition(HUD_BULLET_ICON_TEXTURE_POS_X, HUD_BULLET_ICON_TEXTURE_POS_Y);
	m_bulletIcon->SetScale(0.056f, 0.056f);
	m_sprites.push_back(m_bulletIcon);

	m_crosshair = new UISprite(
		UIManager::get_instance().GetSpriteVertexBuffer(),
		TextureMgr::get_instance().CreateTexture(HUD_CROSSHAIR_TEXTURE_PATH)
	);
	m_crosshair->SetSize(HUD_CROSSHAIR_TEXTURE_SIZE_X, HUD_CROSSHAIR_TEXTURE_SIZE_Y);
	m_crosshair->SetPosition(SCREEN_CENTER_X, SCREEN_CENTER_Y);
	m_crosshair->SetScale(1.0f, 1.0f);
	m_sprites.push_back(m_crosshair);

	m_itemCount = new UISprite(
		UIManager::get_instance().GetSpriteVertexBuffer(),
		TextureMgr::get_instance().CreateTexture(HUD_ITEM_COUNT_TEXTURE_PATH)
	);
	m_itemCount->SetSize(HUD_ITEM_COUNT_TEXTURE_SIZE_X, HUD_ITEM_COUNT_TEXTURE_SIZE_Y);
	m_itemCount->SetScale(1.0f, 1.0f);

	m_itemCountSign = new UISprite(
		UIManager::get_instance().GetSpriteVertexBuffer(),
		TextureMgr::get_instance().CreateTexture(HUD_ITEM_COUNT_SIGN_TEXTURE_PATH)
	);
	m_itemCountSign->SetSize(HUD_ITEM_COUNT_TEXTURE_SIZE_X, HUD_ITEM_COUNT_TEXTURE_SIZE_Y);
	m_itemCountSign->SetScale(0.5f, 0.5f);
}

void UIMainHUD::Update(void)
{
	if (m_player == nullptr)
	{
		m_player = GameSystem::get_instance().GetPlayer();
		if (m_player == nullptr) return; // プレイヤーがまだ初期化されていない場合は何もしない
	}

	// アイコンの表示状態を更新
	m_seedIcon->SetVisible(m_player->HasSeed());
	m_bulletIcon->SetVisible(m_player->HasBullet());

	m_crosshair->SetVisible(m_player->GetPlayerAttributes().aimMode);

	// アイコンの位置を更新
	if (m_player->GetMeleeMode())
	{
		m_weaponIcon->SetPosition(HUD_MAIN_WEAPON_ICON_TEXTURE_POS_X, HUD_MAIN_WEAPON_ICON_TEXTURE_POS_Y);
		m_weaponIcon->SetScale(HUD_MAIN_WEAPON_ICON_TEXTURE_SCALE, HUD_MAIN_WEAPON_ICON_TEXTURE_SCALE);
		m_gunIcon->SetPosition(HUD_SUB_GUN_ICON_TEXTURE_POS_X, HUD_SUB_GUN_ICON_TEXTURE_POS_Y);
		m_gunIcon->SetScale(HUD_SUB_GUN_ICON_TEXTURE_SCALE, HUD_SUB_GUN_ICON_TEXTURE_SCALE);
	}
	else
	{
		m_weaponIcon->SetPosition(HUD_SUB_WEAPON_ICON_TEXTURE_POS_X, HUD_SUB_WEAPON_ICON_TEXTURE_POS_Y);
		m_weaponIcon->SetScale(HUD_SUB_WEAPON_ICON_TEXTURE_SCALE, HUD_SUB_WEAPON_ICON_TEXTURE_SCALE);
		m_gunIcon->SetPosition(HUD_MAIN_GUN_ICON_TEXTURE_POS_X, HUD_MAIN_GUN_ICON_TEXTURE_POS_Y);
		m_gunIcon->SetScale(HUD_MAIN_GUN_ICON_TEXTURE_SCALE, HUD_MAIN_GUN_ICON_TEXTURE_SCALE);
	}
}

void UIMainHUD::Draw(void)
{
	for (UINT i = 0; i < m_sprites.getSize(); ++i)
	{
		if (m_sprites[i] && m_sprites[i]->IsVisible())
		{
			m_sprites[i]->Draw();
		}
	}

	// アイテムの所持数を表示
	if (m_player)
	{
		DrawNumers(m_player->GetPlayerAttributes().bulletCount, HUD_BULLET_COUNT_TEXTURE_POS_X, HUD_BULLET_COUNT_TEXTURE_POS_Y);
		DrawNumers(m_player->GetPlayerAttributes().seedCount, HUD_SEED_COUNT_TEXTURE_POS_X, HUD_SEED_COUNT_TEXTURE_POS_Y);
	}
}

void UIMainHUD::DrawNumers(int number, float posX, float posY)
{
	if (number <= 0) return;

	// スコアの位置やテクスチャー座標を反映
	float pw = m_itemCount->GetSize().x;	// スコアの表示幅
	float ph = m_itemCount->GetSize().y;	// スコアの表示高さ
	float px = 0;							// スコアの表示位置X
	float py = posY;						// スコアの表示位置Y

	float tw = 1.0f / 10;		// テクスチャの幅
	float th = 1.0f / 1;		// テクスチャの高さ
	float tx = 0;				// テクスチャの左上X座標
	float ty = 0.0f;			// テクスチャの左上Y座標

	// 桁数分処理する
	for (int i = 0; i < ITEM_COUNT_DIGIT; i++)
	{
		// 今回表示する桁の数字
		float x = (float)(number % 10);

		px = posX - pw * i; // スコアの表示位置X
		tx = x * tw; // テクスチャの左上X座標

		// 表示位置を設定
		m_itemCount->SetPosition(px, py);

		// １枚のポリゴンの頂点とテクスチャ座標を設定
		m_itemCount->SetUV(tx, ty, tw, th);

		// ポリゴン描画
		m_itemCount->Draw();

		// 次の桁へ
		number /= 10;
	}

	m_itemCountSign->SetPosition(posX - pw * ITEM_COUNT_DIGIT, posY);
	m_itemCountSign->Draw();
}
