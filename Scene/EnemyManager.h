//=============================================================================
//
// エネミーたちのまとめ役ちゃん [EnemyManager.h]
// Author : 
// 敵キャラの生成・更新・描画・UI表示までぜーんぶお世話してくれる管理クラスですっ！
// ダブルリンクリストで敵ちゃんたちをしっかり整列させてるお姉さんタイプかも…？
//
//=============================================================================
#pragma once

#include "Scene/Enemy.h"
#include "Utility/DoubleLinkedList.h"
#include "Utility/SingletonBase.h"

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
class EnemyManager : public SingletonBase<EnemyManager>, public IDebugUI
{
public:
	EnemyManager();

	void Init(const Player* player = nullptr);
	void Draw(void);
	void Update(void);
	void DrawUI(EnemyUIType type);

	// 敵キャラの生成
	void SpawnEnemy(EnemyType enemType, Transform trans, EnemyState initState); 

	const DoubleLinkedList<Enemy*>* GetEnemy() { return &m_enemyList; }

private:

	virtual void RenderImGui(void) override;
	virtual const char* GetPanelName(void) const override { return "Enemy Manager"; };

	DoubleLinkedList<Enemy*> m_enemyList;
	Renderer& m_renderer = Renderer::get_instance();
	const Player* m_player;
	bool m_drawBoundingBox = true;
};