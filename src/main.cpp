#include "includes.h"

#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb/stb_image.h"

#include "libs/ImGui/fonts/Lato-Regular.h"

#define WINDOW_WIDTH  400
#define WINDOW_HEIGHT 475

//Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_pMainRenderTargetView = nullptr;

//Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void SetupImGuiStyle();
void MainWindow(HWND mainWindow);
bool Inject(HWND ownerWindow, const char* pTargetProcessName, const char* pFilePath);
std::string OpenFileDialog(HWND ownerWindow);

//Main Code
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
int main()
{
	//Create application window
	WNDCLASSEXW windowClass = { sizeof(windowClass), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, skCrypt(L"VInject"), nullptr };
	::RegisterClassExW(&windowClass);
	HWND hWnd = ::CreateWindowExW(0L, windowClass.lpszClassName, skCrypt(L"VInject"), WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX, 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, windowClass.hInstance, nullptr);

	//Initialize Direct 3D
	if (!CreateDeviceD3D(hWnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
		return 1;
	}

	//Show the window
	::ShowWindow(hWnd, SW_SHOWDEFAULT);
	::UpdateWindow(hWnd);

	//Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	SetupImGuiStyle();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// Load Fonts
   // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
   // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
   // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
   // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
   // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
   // - Read 'docs/FONTS.md' for more instructions and details.
   // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
   //io.Fonts->AddFontDefault();
   //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
   //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
   //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
   //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
   //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
   //IM_ASSERT(font != nullptr);
	ImFontConfig latoRegularConfig = {};
	latoRegularConfig.FontDataOwnedByAtlas = true;
	latoRegularConfig.PixelSnapH = true;

	ImFont* pFontLatoRegular = io.Fonts->AddFontFromMemoryTTF(Lato_Regular, sizeof(Lato_Regular), 21.0f, &latoRegularConfig, io.Fonts->GetGlyphRangesDefault());
	IM_ASSERT(pFontLatoRegular != nullptr);

	//Our state
	bool bShowMainWindow = true;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//Main Loop
	bool done = false;
	while (!done)
	{
		//Poll and handle messages (inputs, window resize, etc.)
		//See the WndProc() function below for our to disptach events to the Win32 backend.
		MSG msg;
		while (::PeekMessageA(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessageA(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		//Handle window resize (we don't resize directly in the WM_SIZE handler)
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		{
			MainWindow(hWnd);
		}
		//Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pMainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_pMainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		//g_pSwapChain->Present(1, 0); //Present with vsync
		g_pSwapChain->Present(0, 0); //present without vsync
	}

	//Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hWnd);
	::UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);

	return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pMainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_pMainRenderTargetView) { g_pMainRenderTargetView->Release(); g_pMainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

ID3D11ShaderResourceView* LoadImageFromUInt32Array(ID3D11Device* pDevice, const uint32_t* pImageData, int width, int height)
{
	ID3D11Texture2D* pTexture = nullptr;
	ID3D11ShaderResourceView* textureView = nullptr;

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = pImageData;
	subResource.SysMemPitch = desc.Width * 4; // 4 bytes / pixel
	subResource.SysMemSlicePitch = 0;

	if (SUCCEEDED(pDevice->CreateTexture2D(&desc, &subResource, &pTexture)))
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		if (FAILED(pDevice->CreateShaderResourceView(pTexture, &srvDesc, &textureView)))
		{
			// could not create shader for resource view
		}
		pTexture->Release();
	}
	else
	{
		//could not create 2d texture

	}

	return textureView;
}

void SetupImGuiStyle()
{
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	ImGuiStyle& style = ImGui::GetStyle();

	style.FrameBorderSize = 1.0f;
	style.FrameRounding = 2.0f;

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(23.0f / 255.0f, 23.0f / 255.0f, 23.0f / 255.0f, 1.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(23.0f / 255.0f, 23.0f / 255.0f, 23.0f / 255.0f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(64.0f / 255.0f, 64.0f / 255.0f, 64.0f / 255.0f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(23.0f / 255.0f, 23.0f / 255.0f, 23.0f / 255.0f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(38.0f / 255.0f, 38.0f / 255.0f, 38.0f / 255.0f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(38.0f / 255.0f, 38.0f / 255.0f, 38.0f / 255.0f, 1.0f);

}

void MainWindow(HWND mainWindow)
{
	static char processName[64] = "";

	static std::string selectedProcess{ "Select Process..." };
	static std::string selectedFile{ "Select File..." };

	ImGui::SetNextWindowPos(ImVec2(-1, 0));
	ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));
	ImGui::Begin(skCrypt("VInject"), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
	{
		/*
		static const ID3D11ShaderResourceView* pTextureView = LoadImageFromUInt32Array(g_pd3dDevice, vinject, VINJECT_WIDTH, VINJECT_HEIGHT);
		if (pTextureView)
		{
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - (400.0f * 0.5f));
			ImGui::Image((void*)pTextureView, ImVec2((float)400.0f, (float)80.0f));
		}
		*/
		if (ImGui::BeginPopupModal(skCrypt("##EnterProcessName"), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
		{
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - (ImGui::CalcTextSize(skCrypt("enter process name:")).x * 0.5f));
			ImGui::Text(skCrypt("enter process name:"));
			ImGui::Dummy(ImVec2(380.0f, 0.0f));

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::InputText("##ProcessNameField", processName, sizeof(processName));

			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - (120.0f * 0.5f));
			if (ImGui::Button(skCrypt("OK"), ImVec2(120, 0)))
			{
				selectedProcess = processName;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::SetCursorPos(ImVec2((ImGui::GetContentRegionAvail().x * 0.5f) - (ImGui::CalcTextSize(skCrypt("V I N J E C T")).x * 0.5f), 30.0f));
		ImGui::Text(skCrypt("V I N J E C T"));

		ImGui::SetCursorPos(ImVec2((ImGui::GetContentRegionAvail().x * 0.5f) - (372.0f * 0.5f), ImGui::GetContentRegionAvail().y * 0.5f - 50.0f));
		if (ImGui::Button(selectedProcess.c_str(), ImVec2(372.0f, 45.0f)))
		{
			ImGui::OpenPopup(skCrypt("##EnterProcessName"));
		}

		ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - (372.0f * 0.5f));
		if (ImGui::Button(selectedFile.c_str(), ImVec2(372.0f, 45.0f)))
		{
			std::string filePath = OpenFileDialog(mainWindow);

			if (!filePath.empty())
			{
				selectedFile = filePath;
			}
		}

		ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - (372.0f * 0.5f));
		if (ImGui::Button(skCrypt("Inject"), ImVec2(372.0f, 45.0f)))
		{
			Inject(mainWindow, selectedProcess.c_str(), selectedFile.c_str());
		}

		ImGui::SetCursorPos(ImVec2((ImGui::GetContentRegionAvail().x * 0.5f) - (ImGui::CalcTextSize(skCrypt("Version 1.0")).x * 0.5f), ImGui::GetWindowSize().y - 75.0f));
		ImGui::Text(skCrypt("Version 1.0"));
	}
	ImGui::End();
}

bool Inject(HWND ownerWindow, const char* pTargetProcessName, const char* pFilePath)
{
	if (!ownerWindow || !pTargetProcessName || !pFilePath)
	{
		MessageBoxA(NULL, skCrypt("passed invalid parameter(s)!"), skCrypt("error"), MB_OK | MB_ICONERROR);
		return false;
	}

	PROCESSENTRY32 PE32{ 0 };
	PE32.dwSize = sizeof(PE32);

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = GetLastError();
		MessageBoxA(ownerWindow, std::to_string(dwError).c_str(), skCrypt("CreateToolhelp32Snapshot failed!"), MB_OK | MB_ICONERROR);
		return false;
	}

	DWORD PID = 0;
	BOOL bRet = Process32First(hSnap, &PE32);
	while (bRet)
	{
		if (!strcmp(pTargetProcessName, PE32.szExeFile))
		{
			PID = PE32.th32ProcessID;
			break;
		}
		bRet = Process32Next(hSnap, &PE32);
	}

	CloseHandle(hSnap);

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
	if (!hProc)
	{
		DWORD dwError = GetLastError();
		MessageBoxA(ownerWindow, std::to_string(dwError).c_str(), skCrypt("OpenProcess failed!"), MB_OK | MB_ICONERROR);
		return false;
	}

	if (!ManualMap(hProc, pFilePath))
	{
		CloseHandle(hProc);
		MessageBoxA(ownerWindow, skCrypt("mapping DLL failed!"), skCrypt("error"), MB_OK | MB_ICONERROR);
		return false;
	}

	CloseHandle(hProc);

	return true;
}

std::string OpenFileDialog(HWND ownerWindow)
{
	OPENFILENAME ofn;
	char fileName[MAX_PATH] = "";
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = ownerWindow;
	ofn.lpstrFilter = "DLL Files\0*.dll\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "";

	if (GetOpenFileName(&ofn))
		return fileName;

	return "";
}