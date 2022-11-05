//QC wallhack 2022
//Compile in release mode x64
//Location: qc wallhack\proxydll_cryptsp\bin\Release\x64\cryptsp.dll

#pragma once
#include <Windows.h>
#include <vector>
#include <d3d11.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")

//minhook
#include "MinHook/include/MinHook.h"

//imgui
#include "ImGui\imgui.h"
#include "imgui\imgui_impl_win32.h"
#include "ImGui\imgui_impl_dx11.h"


typedef HRESULT(__stdcall *D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall *D3D11ResizeBuffersHook) (IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
typedef void(__stdcall *D3D11DrawIndexedHook) (ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
typedef void(__stdcall* D3D11DrawIndexedInstancedHook) (ID3D11DeviceContext* pContext, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);


D3D11PresentHook phookD3D11Present = NULL;
D3D11ResizeBuffersHook phookD3D11ResizeBuffers = NULL;
D3D11DrawIndexedHook phookD3D11DrawIndexed = NULL;
D3D11DrawIndexedInstancedHook phookD3D11DrawIndexedInstanced = NULL;

ID3D11Device *pDevice = NULL;
ID3D11DeviceContext *pContext = NULL;

DWORD_PTR* pSwapChainVtable = NULL;
DWORD_PTR* pContextVTable = NULL;
DWORD_PTR* pDeviceVTable = NULL;


#include "main.h" //helper funcs

//==========================================================================================================================//

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND window = NULL;
WNDPROC oWndProc;

void InitImGuiD3D11()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsClassic();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
		return true;
	}
	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

//==========================================================================================================================//

HRESULT __stdcall hookD3D11ResizeBuffers(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	ImGui_ImplDX11_InvalidateDeviceObjects();
	if (nullptr != mainRenderTargetViewD3D11) { mainRenderTargetViewD3D11->Release(); mainRenderTargetViewD3D11 = nullptr; }

	HRESULT toReturn = phookD3D11ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	ImGui_ImplDX11_CreateDeviceObjects();

	return phookD3D11ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

//==========================================================================================================================//

HRESULT __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!initonce)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetViewD3D11);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGuiD3D11();

			//create depthbias rasterizer state
			D3D11_RASTERIZER_DESC rasterizer_desc;
			ZeroMemory(&rasterizer_desc, sizeof(rasterizer_desc));
			rasterizer_desc.FillMode = D3D11_FILL_SOLID;
			rasterizer_desc.CullMode = D3D11_CULL_NONE; 
			rasterizer_desc.FrontCounterClockwise = false;
			rasterizer_desc.SlopeScaledDepthBias = 0.0f;
			rasterizer_desc.DepthBiasClamp = 0.0f;
			rasterizer_desc.DepthClipEnable = true;
			rasterizer_desc.ScissorEnable = false;
			rasterizer_desc.MultisampleEnable = false;
			rasterizer_desc.AntialiasedLineEnable = false;
			rasterizer_desc.DepthBias = 0x7FFFFFFF;
			pDevice->CreateRasterizerState(&rasterizer_desc, &DEPTHBIASState_FALSE);

			//create normal rasterizer state
			D3D11_RASTERIZER_DESC nrasterizer_desc;
			ZeroMemory(&nrasterizer_desc, sizeof(nrasterizer_desc));
			nrasterizer_desc.FillMode = D3D11_FILL_SOLID;
			nrasterizer_desc.CullMode = D3D11_CULL_NONE;
			nrasterizer_desc.FrontCounterClockwise = false;
			nrasterizer_desc.DepthBias = 0;
			nrasterizer_desc.SlopeScaledDepthBias = 0.0f;
			nrasterizer_desc.DepthBiasClamp = 0.0f;
			nrasterizer_desc.DepthClipEnable = true;
			nrasterizer_desc.ScissorEnable = false;
			nrasterizer_desc.MultisampleEnable = false;
			nrasterizer_desc.AntialiasedLineEnable = false;
			pDevice->CreateRasterizerState(&nrasterizer_desc, &DEPTHBIASState_TRUE);

			//load cfg settings
			LoadCfg();

			initonce = true;
		}
		else
			return phookD3D11Present(pSwapChain, SyncInterval, Flags);
	}

	//recreate rendertarget on reset
	if (mainRenderTargetViewD3D11 == NULL)
	{
		ID3D11Texture2D* pBackBuffer = NULL;
		pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetViewD3D11);
		pBackBuffer->Release();
	}

	if (GetAsyncKeyState(VK_INSERT) & 1) { //menu key
		SaveCfg(); //save settings
		showmenu = !showmenu;
	}


	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//menu
	if (showmenu) {
		ImGui::SetNextWindowSize(ImVec2(150.0f, 110.0f)); //size
		ImVec4 Bgcol = ImColor(0.0f, 0.4f, 0.28f, 0.8f); //bg color
		ImGui::PushStyleColor(ImGuiCol_WindowBg, Bgcol);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 0.8f)); //frame color

		//use mouse or arrows & space to navigate
		ImGui::Begin("", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Checkbox("Wallhack Models", &wallhackm);
		ImGui::Checkbox("Wallhack Items", &wallhacki);
		ImGui::Checkbox("Wallhack Weapons", &wallhackw);
		ImGui::Checkbox("Wallhack Ammo", &wallhacka);
		ImGui::End();
	}

	//ImGui::EndFrame();
	ImGui::Render();
	pContext->OMSetRenderTargets(1, &mainRenderTargetViewD3D11, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return phookD3D11Present(pSwapChain, SyncInterval, Flags);
}

//==========================================================================================================================//

void __stdcall hookD3D11DrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	
	ID3D11Buffer* veBuffer;
	UINT veWidth;
	UINT Stride;
	UINT veBufferOffset;
	D3D11_BUFFER_DESC veDesc;

	//get models
	pContext->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);
	if (veBuffer) {
		veBuffer->GetDesc(&veDesc);
		veWidth = veDesc.ByteWidth;
	}
	if (NULL != veBuffer) {
		veBuffer->Release();
		veBuffer = NULL;
	}

	//the models
	if(wallhackm == 1)
	if(veWidth == 33019776|| //blazkowicz
		veWidth == 32416480|| //ranger
		veWidth == 37471908|| //scalebearer
		veWidth == 36695844|| //visor
		veWidth == 34416600|| //anarki
		veWidth == 27186138|| //nyx
		veWidth == 49464832|| //sorlag
		veWidth == 37147024|| //clutch
		veWidth == 40442608|| //galena
		veWidth == 36916404|| //slash
		veWidth == 30639408|| //doom slayer
		veWidth == 35216464|| //keel
		veWidth == 65903852|| //strogg
		veWidth == 36858108|| //death knight
		veWidth == 27980754|| //athena
		veWidth == 43159020) //eisen
	{
		pContext->RSSetState(DEPTHBIASState_FALSE); //wh on

		phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);

		pContext->RSSetState(DEPTHBIASState_TRUE); //wh off
	}	

	//ctf flag
	if (wallhacki == 1)
	if(IndexCount == 16002 && veWidth == 339528)
	{
		pContext->RSSetState(DEPTHBIASState_FALSE); //wh on

		phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);

		pContext->RSSetState(DEPTHBIASState_TRUE); //wh off
	}

	//fix wallhack
	if (wallhackm == 1|| wallhacki == 1|| wallhackw == 1|| wallhacka == 1) //&& pscdesc[3].ByteWidth == 480
	if ((IndexCount == 18963 && veWidth == 54720764) || //awoken map1
		(IndexCount == 22494 && veWidth == 61246792) || //blood covenant
		(IndexCount == 11178 && veWidth == 62215376) || //blood run
		(IndexCount == 20319 && veWidth == 66169560) || //burial chamber, whats IndexCount == 20199 && veWidth == 422082
		(IndexCount == 29340 && veWidth == 66682728) || //church of azathoth
		(IndexCount == 60762 && veWidth == 66982464) || //citadel
		(IndexCount == 19824 && veWidth == 66088608) || //corrupted keep
		(IndexCount == 4986 && veWidth == 67003608) || //deep embrace
		(IndexCount == 648 && veWidth == 36378192) || //exile
		(IndexCount == 11265 && veWidth == 23886072) || //insomnia
		(IndexCount == 13548 && veWidth == 59518192) || //lockbox
		(IndexCount == 35826 && veWidth == 62887880) || //ruins of sarnath
		(IndexCount == 28224 && veWidth == 66891856) || //tempest shrine
		(IndexCount == 16887 && veWidth == 4053728) || //the dark zone
		(IndexCount == 6300 && veWidth == 63972264) || //the longest yard
		(IndexCount == 6792 && veWidth == 66900888) || //the molten falls
		(IndexCount == 11514 && veWidth == 51351000) || //tower of koth
		(IndexCount == 9810 && veWidth == 41729976)) //vale of pnath
		{
			return;
		}

	/*
	//logger
	if (countnum == IndexCount / 10)
		if (GetAsyncKeyState(VK_END) & 1)//log key
			Log("Stride == %d && IndexCount == %d && veWidth == %d", Stride, IndexCount, veWidth);

	if (countnum == IndexCount / 10)
	{
		return;
	}

	//hold down P key until a texture is erased, press END to log values of those textures
	if (GetAsyncKeyState('O') & 1) //-
		countnum--;
	if (GetAsyncKeyState('P') & 1) //+
		countnum++;
	if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState('9') & 1) //reset, set to -1
		countnum = -1;
	*/
	
    return phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}

//==========================================================================================================================//

void __stdcall hookD3D11DrawIndexedInstanced(ID3D11DeviceContext* pContext, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
	ID3D11Buffer* veBuffer2;
	UINT veWidth2;
	UINT Stride2;
	UINT veBufferOffset2;
	D3D11_BUFFER_DESC veDesc2;

	//get models
	pContext->IAGetVertexBuffers(0, 1, &veBuffer2, &Stride2, &veBufferOffset2);
	if (veBuffer2) {
		veBuffer2->GetDesc(&veDesc2);
		veWidth2 = veDesc2.ByteWidth;
	}
	if (NULL != veBuffer2) {
		veBuffer2->Release();
		veBuffer2 = NULL;
	}
	
	//wallhack items(health, armor, powerups)
	if (wallhacki == 1)
	if((Stride2 == 8 && IndexCountPerInstance == 948 && veWidth2 == 16824)|| //mega health
		(Stride2 == 8 && IndexCountPerInstance == 4266 && veWidth2 == 47208) || //heavy armor
		(Stride2 == 8 && IndexCountPerInstance == 2184 && veWidth2 == 25824) || //light armour
		(Stride2 == 8 && IndexCountPerInstance == 972 && veWidth2 == 30672) || //small health inside
		(Stride2 == 8 && IndexCountPerInstance == 2280 && veWidth2 == 30672) || //small health bubble
		(Stride2 == 8 && IndexCountPerInstance == 1008 && veWidth2 == 24352) || //quad damage
		(Stride2 == 8 && IndexCountPerInstance == 1020 && veWidth2 == 21984) || //battle suit
		(Stride2 == 8 && IndexCountPerInstance == 720 && veWidth2 == 99700) || //hourglass
		(Stride2 == 8 && IndexCountPerInstance == 2550 && veWidth2 == 40232))  //armor shards
	{
		pContext->RSSetState(DEPTHBIASState_FALSE); //wh on

		phookD3D11DrawIndexedInstanced(pContext, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

		pContext->RSSetState(DEPTHBIASState_TRUE); //wh off
	}

	//wallhack weapons
	if (wallhackw == 1)
	if((Stride2 == 8 && IndexCountPerInstance == 4275 && veWidth2 == 109816) || // rocket launcher
	(Stride2 == 8 && IndexCountPerInstance == 3000 && veWidth2 == 102432) || // railgun
	(Stride2 == 8 && IndexCountPerInstance == 924 && veWidth2 == 63920) || // super shotgun 
	(Stride2 == 8 && IndexCountPerInstance == 828 && veWidth2 == 88960) || // heavy machinegun 
	(Stride2 == 8 && IndexCountPerInstance == 7212 && veWidth2 == 93608) || // super nailgun
	(Stride2 == 8 && IndexCountPerInstance == 882 && veWidth2 == 88856) || // lightning gun
	(Stride2 == 8 && IndexCountPerInstance == 8232 && veWidth2 == 141616))  // tri-bolt
	{
		pContext->RSSetState(DEPTHBIASState_FALSE); //wh on

		phookD3D11DrawIndexedInstanced(pContext, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

		pContext->RSSetState(DEPTHBIASState_TRUE); //wh off
	}
	
	//wallhack ammo
	if (wallhacka == 1)
	if((Stride2 == 8 && IndexCountPerInstance == 4644 && veWidth2 == 97240) || //railgun ammo
	(Stride2 == 8 && IndexCountPerInstance == 4644 && veWidth2 == 76024) || //tri-bolt ammo
	(Stride2 == 8 && IndexCountPerInstance == 4644 && veWidth2 == 96936) || //rocket ammo
	(Stride2 == 8 && IndexCountPerInstance == 4644 && veWidth2 == 96864) || //lg ammo
	(Stride2 == 8 && IndexCountPerInstance == 4644 && veWidth2 == 96808) || //nailgun ammo
	(Stride2 == 8 && IndexCountPerInstance == 4644 && veWidth2 == 96904) || //shotgun ammo
	(Stride2 == 8 && IndexCountPerInstance == 4644 && veWidth2 == 96912)) //heavy machinegun ammo
	{
		pContext->RSSetState(DEPTHBIASState_FALSE); //wh on

		phookD3D11DrawIndexedInstanced(pContext, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

		pContext->RSSetState(DEPTHBIASState_TRUE); //wh off
	}
	
	/*
	//logger
	if (countnum == veWidth2 / 1000)//1000
		if (GetAsyncKeyState(VK_END) & 1)//log key
			Log("Stride2 == %d && IndexCountPerInstance == %d && veWidth2 == %d", Stride2, IndexCountPerInstance, veWidth2);

	if (countnum == veWidth2 / 1000)//1000
	{
		return;
	}

	//hold down P key until a texture is erased, press END to log values of those textures
	if (GetAsyncKeyState('O') & 1) //-
		countnum--;
	if (GetAsyncKeyState('P') & 1) //+
		countnum++;
	if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState('9') & 1) //reset, set to -1
		countnum = -1;
	*/
	return phookD3D11DrawIndexedInstanced(pContext, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

//==========================================================================================================================//

const int MultisampleCount = 1;
LRESULT CALLBACK DXGIMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){ return DefWindowProc(hwnd, uMsg, wParam, lParam); }
DWORD __stdcall InitHooks(LPVOID)
{
	HMODULE hDXGIDLL = 0;
	do
	{
		hDXGIDLL = GetModuleHandle(L"dxgi.dll");
		Sleep(4000);
	} while (!hDXGIDLL);
	Sleep(100);

	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

    IDXGISwapChain* pSwapChain;

	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DXGIMsgProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
	RegisterClassExA(&wc);
	HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);

	D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
	D3D_FEATURE_LEVEL obtainedLevel;
	ID3D11Device* d3dDevice = nullptr;
	ID3D11DeviceContext* d3dContext = nullptr;

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(scd));
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = MultisampleCount;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Windowed = ((GetWindowLongPtr(hWnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

	scd.BufferDesc.Width = 1;
	scd.BufferDesc.Height = 1;
	scd.BufferDesc.RefreshRate.Numerator = 0;
	scd.BufferDesc.RefreshRate.Denominator = 1;

	UINT createFlags = 0;
#ifdef _DEBUG
	// This flag gives you some quite wonderful debug text. Not wonderful for performance, though!
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	IDXGISwapChain* d3dSwapChain = 0;

	if (FAILED(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createFlags,
		requestedLevels,
		sizeof(requestedLevels) / sizeof(D3D_FEATURE_LEVEL),
		D3D11_SDK_VERSION,
		&scd,
		&pSwapChain,
		&pDevice,
		&obtainedLevel,
		&pContext)))
	{
		MessageBox(hWnd, L"Failed to create DirectX device and swapchain!", L"Error", MB_ICONERROR);
		return NULL;
	}


    pSwapChainVtable = (DWORD_PTR*)pSwapChain;
    pSwapChainVtable = (DWORD_PTR*)pSwapChainVtable[0];

    pContextVTable = (DWORD_PTR*)pContext;
    pContextVTable = (DWORD_PTR*)pContextVTable[0];

	pDeviceVTable = (DWORD_PTR*)pDevice;
	pDeviceVTable = (DWORD_PTR*)pDeviceVTable[0];

	if (MH_Initialize() != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pSwapChainVtable[8], hookD3D11Present, reinterpret_cast<void**>(&phookD3D11Present)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pSwapChainVtable[8]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pSwapChainVtable[13], hookD3D11ResizeBuffers, reinterpret_cast<void**>(&phookD3D11ResizeBuffers)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pSwapChainVtable[13]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pContextVTable[12], hookD3D11DrawIndexed, reinterpret_cast<void**>(&phookD3D11DrawIndexed)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[12]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pContextVTable[20], hookD3D11DrawIndexedInstanced, reinterpret_cast<void**>(&phookD3D11DrawIndexedInstanced)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[20]) != MH_OK) { return 1; }
	
    DWORD dwOld;
    VirtualProtect(phookD3D11Present, 2, PAGE_EXECUTE_READWRITE, &dwOld);

	while (true) {
		Sleep(10);
	}

	pDevice->Release();
	pContext->Release();
	pSwapChain->Release();

    return NULL;
}

//==========================================================================================================================

BOOL __stdcall DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{ 
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH: 
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, 0, InitHooks, NULL, 0, NULL);
		break;

	case DLL_PROCESS_DETACH: 
		if (MH_Uninitialize() != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pSwapChainVtable[8]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pSwapChainVtable[13]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pContextVTable[12]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pContextVTable[20]) != MH_OK) { return 1; }
		break;
	}
	return TRUE;
}
