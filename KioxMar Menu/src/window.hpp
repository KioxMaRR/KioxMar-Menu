#pragma once
#include <d3d11.h>
#
#include "..\external\imgui\imgui.h"
#include "..\external\imgui\imgui_impl_dx11.h"
#include "..\external\imgui\imgui_impl_win32.h"
extern ImVec4 espColor;
extern ImVec4 crosshairColor;
extern bool LimitEspDistance;
extern float espDistanceLimit;
class Overlay {
public:

	static bool CreateDevice();
	static void DestroyDevice();

	static void CreateOverlay();
	static void DestroyOverlay();


	static bool CreateImGui();
	static void DestroyImGui();


	static void StartRender();
	static void EndRender();


	static void Render();

	static HWND overlay;
	static WNDCLASSEX wc;


	static bool IsWindowInForeground(HWND window) { return GetForegroundWindow() == window; }
	static bool BringToForeground(HWND window) { return SetForegroundWindow(window); }


	static void SetForeground(HWND window);

	static ID3D11Device* device;
	static ID3D11DeviceContext* device_context;
	static IDXGISwapChain* swap_chain;
	static ID3D11RenderTargetView* render_targetview;


	inline static bool RenderMenu;


	inline static bool shouldRun;
};

inline Overlay overlay;

extern bool drawCrosshair;

extern bool showEsp;

extern bool showBulletESP;

extern bool ShowCar;
extern bool ShowPlayer;
extern bool ShowZombi;
extern bool ShowEspLine;
extern bool ShowEspBox;
extern bool  AimBot ;
extern bool ShowAnimal;