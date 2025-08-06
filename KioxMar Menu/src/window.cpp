#include "window.hpp"
#include <iostream>
#include <dwmapi.h>
#include <stdio.h>

#include "offsets.h"
#include "GameEngine.h"
#include "Memory.h"



ID3D11Device* Overlay::device = nullptr;


ID3D11DeviceContext* Overlay::device_context = nullptr;


IDXGISwapChain* Overlay::swap_chain = nullptr;


ID3D11RenderTargetView* Overlay::render_targetview = nullptr;


HWND Overlay::overlay = nullptr;
WNDCLASSEX Overlay::wc = { };


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
using namespace offsets;

LRESULT CALLBACK window_procedure(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// set up ImGui window procedure handler
	if (ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu (imgui uses it in their example :shrug:)
			return 0;
		break;

	case WM_DESTROY:
		Overlay::DestroyDevice();
		Overlay::DestroyOverlay();
		Overlay::DestroyImGui();
		PostQuitMessage(0);
		return 0;

	case WM_CLOSE:
		Overlay::DestroyDevice();
		Overlay::DestroyOverlay();
		Overlay::DestroyImGui();
		return 0;
	}


	return DefWindowProc(window, msg, wParam, lParam);
}

bool Overlay::CreateDevice()
{

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));

)
	sd.BufferCount = 2;


	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;


	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;


	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;


	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	sd.OutputWindow = overlay;


	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;

	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;


	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

	HRESULT result = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		featureLevelArray,
		2,
		D3D11_SDK_VERSION,
		&sd,
		&swap_chain,
		&device,
		&featureLevel,
		&device_context);


	if (result == DXGI_ERROR_UNSUPPORTED) {
		result = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_WARP,
			nullptr,
			0U,
			featureLevelArray,
			2, D3D11_SDK_VERSION,
			&sd,
			&swap_chain,
			&device,
			&featureLevel,
			&device_context);

		printf("[>>] DXGI_ERROR | Created with D3D_DRIVER_TYPE_WARP\n");
	}


	if (result != S_OK) {
		printf("Device Not Okay\n");
		return false;
	}

	ID3D11Texture2D* back_buffer{ nullptr };
	swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));


	if (back_buffer)
	{
		device->CreateRenderTargetView(back_buffer, nullptr, &render_targetview);
		back_buffer->Release();

		return true;
	}


	printf("[>>] Failed to create Device\n");
	return false;
}

void Overlay::DestroyDevice()
{

	if (device)
	{
		device->Release();
		device_context->Release();
		swap_chain->Release();
		render_targetview->Release();

		printf("[>>] Released Device\n");
	}
	else
		printf("[>>] Device Not Found when Exiting.\n");
}

void Overlay::CreateOverlay()
{

	wc.cbSize = sizeof(wc);
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = window_procedure;
	wc.hInstance = GetModuleHandleA(0);
	wc.lpszClassName = "carlgwastaken";


	RegisterClassEx(&wc);


	overlay = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
		wc.lpszClassName,
		"cheat",
		WS_POPUP,
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN), // 1920
		GetSystemMetrics(SM_CYSCREEN), // 1080
		NULL,
		NULL,
		wc.hInstance,
		NULL
	);

	if (overlay == NULL)
		printf("Failed to create Overlay\n");


	SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

	
	{
		// first we define our RECT structures that hold our client and window area
		RECT client_area{};
		RECT window_area{};

		// get the client and window area
		GetClientRect(overlay, &client_area);
		GetWindowRect(overlay, &window_area);

		// calculate the difference between the screen and window coordinates
		POINT diff{};
		ClientToScreen(overlay, &diff);

		// calculate the margins for DWM frame extension
		const MARGINS margins{
			window_area.left + (diff.x - window_area.left),
			window_area.top + (diff.y - window_area.top),
			client_area.right,
			client_area.bottom
		};


		DwmExtendFrameIntoClientArea(overlay, &margins);
	}


	ShowWindow(overlay, SW_SHOW);
	UpdateWindow(overlay);


}

const char* FloatToConstChar(float value) {
	static std::string str;
	str = std::to_string(value);
	return str.c_str();
}

void Overlay::DestroyOverlay()
{
	DestroyWindow(overlay);
	UnregisterClass(wc.lpszClassName, wc.hInstance);
}

bool Overlay::CreateImGui()
{
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// Initalize ImGui for the Win32 library
	if (!ImGui_ImplWin32_Init(overlay)) {
		printf("Failed ImGui_ImplWin32_Init\n");
		return false;
	}

	// Initalize ImGui for DirectX 11.0
	if (!ImGui_ImplDX11_Init(device, device_context)) {
		printf("Failed ImGui_ImplDX11_Init\n");
		return false;
	}


	return true;
}

void Overlay::DestroyImGui()

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Overlay::StartRender()
{

	MSG msg;
	while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (GetAsyncKeyState(VK_NUMPAD0) & 1) {
		RenderMenu = !RenderMenu;

		// If we are rendering the menu set the window styles to be able to clicked on.
		if (RenderMenu) {
			SetWindowLong(overlay, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);
		}
		else {
			SetWindowLong(overlay, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED);
		}
	}
}

void Overlay::EndRender()
{

	float color[4]{ 0, 0, 0, 0 };
	// Set the render target and then clear it
	device_context->OMSetRenderTargets(1, &render_targetview, nullptr);
	device_context->ClearRenderTargetView(render_targetview, color);

	// Render ImGui
	ImGui::Render();





	// Render ImGui draw data.
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present rendered frame with V-Sync
	swap_chain->Present(1U, 0U);

	// Present rendered frame without V-Sync
	//swap_chain->Present(0U, 0U);
}




bool switchState = false;
bool showSecondWindow = false;
bool showFourthWindow = false;
bool showFifthWindow = false;
bool showEsp = true;
bool showBulletESP = true;
bool showSlider = false;
bool drawCrosshair = false;
float sliderValue = 10.0f;
float eyeAccomNormal = 1.0f;
float back = -4.0f;
float front = 4.0f;
bool ShowEspBox = false;
bool ShowEspLine = false;
bool AimBot = false;
bool ShowAnimal = false;

void GrennFormat()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	style.Alpha = 1.0f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 10.0f;
	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(30.0f, 30.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding = 5.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 10.0f;
	style.PopupBorderSize = 0.0f;
	style.FramePadding = ImVec2(5.0f, 3.5f);
	style.FrameRounding = 5.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(5.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);
	style.IndentSpacing = 5.0f;
	style.ColumnsMinSpacing = 5.0f;
	style.ScrollbarSize = 15.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 15.0f;
	style.GrabRounding = 5.0f;
	style.TabRounding = 5.0f;
	style.TabBorderSize = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(1.0f, 1.0f, 1.0f, 0.3605149984359741f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3803921639919281f, 0.4235294163227081f, 0.572549045085907f, 0.5490196347236633f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2588235437870026f, 0.2588235437870026f, 0.2588235437870026f, 0.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 0.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.2352941185235977f, 0.2352941185235977f, 0.2352941185235977f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.294117659330368f, 0.294117659330368f, 0.294117659330368f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.4509803950786591f, 1.0f, 0.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 0.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.294117659330368f, 0.294117659330368f, 0.294117659330368f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.501960813999176f, 0.3019607961177826f, 1.0f, 0.5490196347236633f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);





}
void DarkFormat() {

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.0f);   
	colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.0f);
	colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 0.5f);

	colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 0.95f);    
	colors[ImGuiCol_TextDisabled] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

	colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.16f, 1.0f);   
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.35f, 0.35f, 0.36f, 0.4f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.40f, 0.40f, 0.41f, 0.5f);

	colors[ImGuiCol_TitleBg] = ImVec4(0.13f, 0.13f, 0.14f, 1.0f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);

	colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.26f, 0.9f);  
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.6f, 0.6f, 0.6f, 0.6f);      
	colors[ImGuiCol_ButtonActive] = ImVec4(0.9f, 0.9f, 0.9f, 0.7f);      

	colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.3f, 0.6f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.5f, 0.5f, 0.6f, 0.7f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.7f, 0.7f, 0.9f, 0.8f);

	colors[ImGuiCol_Tab] = ImVec4(0.2f, 0.2f, 0.25f, 0.6f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.7f, 0.7f, 0.7f, 0.8f);
	colors[ImGuiCol_TabActive] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);

	colors[ImGuiCol_Separator] = ImVec4(0.4f, 0.4f, 0.4f, 0.5f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(1.0f, 1.0f, 1.0f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.9f);

	colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 1.0f, 1.0f, 0.4f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.8f);

	style.WindowRounding = 6.0f;
	style.FrameRounding = 4.0f;
	style.ScrollbarRounding = 6.0f;
	style.GrabRounding = 3.0f;



}
void Overlay::Render()
{


	

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	CDispatcher* mem = CDispatcher::Get();
	uint64_t base = mem->GetModuleBase("DayZ_x64.exe");
	uint64_t worldPtr = mem->ReadMemory<uint64_t>(base + offsets::world::world);
	Camera cam;
	static int currentTab = 3;

	ImGui::SetNextWindowSize({ 600, 400 });
	ImGui::Begin("KioxMar Menu", &RenderMenu,
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoResize);

	float totalWidth = ImGui::GetWindowSize().x;
	float tabWidth = totalWidth / 4.0f;
	float tabHeight = 30.0f;

	const char* tabs[] = { "ESP", "Visual", "Aim", "Settings" };
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));

	for (int i = 0; i < 4; ++i) {
		if (i > 0) ImGui::SameLine();

		ImVec2 pos = ImGui::GetCursorScreenPos();
		if (ImGui::Selectable(("##tab" + std::to_string(i)).c_str(), currentTab == i, 0, ImVec2(tabWidth, tabHeight)))
			currentTab = i;

		
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 textSize = ImGui::CalcTextSize(tabs[i]);
		float textX = pos.x + (tabWidth - textSize.x) * 0.5f;
		float textY = pos.y + (tabHeight - textSize.y) * 0.5f;
		drawList->AddText(ImVec2(textX, textY), ImGui::GetColorU32(ImGuiCol_Text), tabs[i]);
	}

	ImGui::PopStyleVar();
	ImGui::Separator();

	if (currentTab == 0) {
		ImGui::Text("ESP Settings:");


		ImGui::Checkbox("Enable ESP", &showEsp);
		ImGui::SameLine(0, 10); 
		ImGui::Checkbox("Show Players", &ShowPlayer);
		ImGui::SameLine(0, 10); 
		ImGui::Checkbox("Show Zombies", &ShowZombi);
		ImGui::SameLine(0, 10); 
		ImGui::Checkbox("Show Cars", &ShowCar);
		ImGui::SameLine(0, 10);
		ImGui::Checkbox("Show Animal", &ShowAnimal);
		
		ImGui::BeginGroup(); 
		ImGui::Checkbox("Box", &ShowEspBox);
		ImGui::Checkbox("Line", &ShowEspLine);
		ImGui::Checkbox("Bullet ESP", &showBulletESP);
		ImGui::EndGroup(); 


		
		ImGui::Text("ESP Color");
		ImGui::ColorEdit4("##espColor", (float*)&espColor);
		ImGui::Checkbox("Limit ESP Distance", &LimitEspDistance);
		ImGui::SliderFloat("Esp Distance (m)",&espDistanceLimit,100.0f,2500.0f,"%.0f m");
			
	}

	else if (currentTab == 1) {
		if (ImGui::Button("XRay")) {
			showSlider = !showSlider;
		}

		if (showSlider) {
			ImGui::SliderFloat("XRay Value", &sliderValue, 0.0f, 250.0f);
			mem->WriteMemory<float>(worldPtr + offsetof(World, EyeAccom), sliderValue);
		}
		else {
			mem->WriteMemory<float>(worldPtr + offsetof(World, EyeAccom), eyeAccomNormal);
		}

		ImGui::Checkbox("Draw Crosshair", &drawCrosshair);

		
		ImGui::Text("Crosshair Color");
		ImGui::ColorEdit4("##crosshairColor", (float*)&crosshairColor);
		ImGui::Text("[KioxMar Message] The downgrade lock has been applied");




	}

	else if (currentTab == 2) {
	
		ImGui::Text("[KioxMar Message] The downgrade lock has been applied");
	}

	else if (currentTab == 3) {
		static bool themeInitialized = false;
		static int currentTheme = 0;
		const char* themes[] = { "Dark White", "Purple", "Light" };

		if (!themeInitialized) {
			DarkFormat();
			themeInitialized = true;
		}

		ImGui::Text("Settings:");
		ImGui::Spacing();
		ImGui::Text("Theme");

		if (ImGui::BeginCombo("##ThemeCombo", themes[currentTheme])) {
			for (int i = 0; i < IM_ARRAYSIZE(themes); ++i) {
				bool selected = (currentTheme == i);
				if (ImGui::Selectable(themes[i], selected)) {
					currentTheme = i;

					switch (i) {
					case 0: DarkFormat(); break;
					case 1: GrennFormat(); break;
					case 2: ImGui::StyleColorsLight(); break;
					}
				}
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();

		}

		if (ImGui::Button("Exit"))
		{
			exit(0);

		}

	}

	ImGui::End();

}



void Overlay::SetForeground(HWND window)
{
	if (!IsWindowInForeground(window))
		BringToForeground(window);
}

