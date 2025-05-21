//=============================================================================
//
// デバッグ情報をまとめて面倒みる管理お姉さま [Debugproc.h]
// Author :
// ImGuiベースのUIを登録・描画して、ログ出力やフレームごとのデバッグ表示を制御するコアクラスですっ！
// ぜんぶのIDebugUIちゃんたちを優しくまとめてくれる、冷静で頼れる司令塔なの~
// 
//=============================================================================
#pragma once
#include "main.h"
#include "Core/Graphics/Renderer.h"
#include "Utility/SingletonBase.h"
#include "Utility/SimpleArray.h"
#include "UI/imgui/imgui.h"
#include "UI/imgui/imgui_impl_dx11.h"
#include "UI/imgui/imgui_impl_win32.h"

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************

class IDebugUI
{
public:
	virtual void RenderImGui() {};
	virtual void RenderDebugInfo() {}
	virtual const char* GetPanelName() const { return nullptr; };
};

class DebugProc : public SingletonBase<DebugProc>
{
public:
	void Init(HWND hwnd);
	void Shutdown(void);

	void PrintDebugProc(char* fmt, ...);
	void Register(IDebugUI* ui);

	void BeginFrame(void);
	void Draw(void);


private:

	void DrawDebugProc(void);

	SimpleArray<IDebugUI*> m_debugUIs;
	HWND m_hwnd = nullptr;
	Renderer& m_Renderer = Renderer::get_instance();
	char m_aStrDebug[1024] = { "\0" };	// デバッグ情報
	static constexpr int MAX_DEBUG_LINES = 100;
	char m_lines[MAX_DEBUG_LINES][512];
	int m_lineCount = 0;
};