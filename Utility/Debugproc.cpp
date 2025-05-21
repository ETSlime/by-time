//=============================================================================
//
// デバッグ表示処理 [Debugproc.cpp]
// Author : 
//
//=============================================================================
#include <stdio.h>
#include "Utility/Debugproc.h"
#include "Core/Graphics/ShadowMapRenderer.h"

void DebugProc::Init(HWND hwnd)
{
	m_hwnd = hwnd;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(m_Renderer.GetDevice(), m_Renderer.GetDeviceContext());

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
}

void DebugProc::Shutdown()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

//=============================================================================
// デバッグ表示処理の描画処理
//=============================================================================
void DebugProc::DrawDebugProc(void)
{
	RECT rect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

	m_Renderer.DebugTextOut(m_aStrDebug, 0, 0);

	// 情報クリア
	memset(m_aStrDebug, 0, sizeof m_aStrDebug);
}

//=============================================================================
// デバッグ表示の登録
//=============================================================================
void DebugProc::PrintDebugProc(char* fmt, ...)
{
#if 0
	long* pParam;
	static char aBuf[256];

	pParam = (long*)&fmt;
	sprintf(aBuf, fmt, pParam[1], pParam[2], pParam[3], pParam[4],
		pParam[5], pParam[6], pParam[7], pParam[8],
		pParam[9], pParam[10], pParam[11], pParam[12]);
#else
	va_list list;			// 可変引数を処理する為に使用する変数
	char* pCur;
	char aBuf[256] = { "\0" };
	char aWk[32];

	// 可変引数にアクセスする前の初期処理
	va_start(list, fmt);

	pCur = fmt;
	for (; *pCur; ++pCur)
	{
		if (*pCur != '%')
		{
			sprintf_s(aWk, "%c", *pCur);
		}
		else
		{
			pCur++;

			switch (*pCur)
			{
			case 'd':
				// 可変引数にアクセスしてその変数を取り出す処理
				sprintf_s(aWk, "%d", va_arg(list, int));
				break;

			case 'f':
				// 可変引数にアクセスしてその変数を取り出す処理
				sprintf_s(aWk, "%.2f", va_arg(list, double));		// double型で指定
				break;

			case 's':
				// 可変引数にアクセスしてその変数を取り出す処理
				sprintf_s(aWk, "%s", va_arg(list, char*));
				break;

			case 'c':
				// 可変引数にアクセスしてその変数を取り出す処理
				sprintf_s(aWk, "%c", va_arg(list, char));
				break;

			default:
				sprintf_s(aWk, "%c", *pCur);
				break;
			}
		}
		strcat_s(aBuf, aWk);
	}

	// 可変引数にアクセスした後の終了処理
	va_end(list);

	// 連結
	if ((strlen(m_aStrDebug) + strlen(aBuf)) < ((sizeof m_aStrDebug) - 1))
	{
		strcat_s(m_aStrDebug, aBuf);
	}
#endif
}

void DebugProc::Register(IDebugUI* ui)
{
	m_debugUIs.push_back(ui);
}

void DebugProc::BeginFrame(void)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	m_lineCount = 0;
}

void DebugProc::Draw(void)
{
	DrawDebugProc();

	//ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));
	//ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

	ImGui::Begin("Debug Panel", nullptr, ImGuiWindowFlags_None);

	for (auto* ui : m_debugUIs)
	{
		if (ui->GetPanelName() && ImGui::CollapsingHeader(ui->GetPanelName()))
			ui->RenderImGui();
		ui->RenderDebugInfo();
	}

	for (int i = 0; i < m_lineCount; ++i)
	{
		ImGui::TextUnformatted(m_lines[i]);
	}

	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}


