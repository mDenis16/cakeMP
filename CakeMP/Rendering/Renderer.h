#pragma once

#include <Common.h>
#include <shv/main.h>
#include <d3d11_1.h>

#include "CefApp/d3d11.h"

NAMESPACE_BEGIN
	;

class Renderer
{
public:
	HWND					 windowHandle = (HWND)0;
	IDXGISwapChain* pSwapChain = nullptr;
	ID3D11DeviceContext* pDeviceContext = nullptr;
	ID3D11RenderTargetView* pRenderTargetView = nullptr;

	ID3D11Device* pDevice = nullptr;
	ID3D11BlendState* pBlender = nullptr;
	ID3D11SamplerState* pSampler = nullptr;
public:

	~Renderer();
	Renderer();


	void CleanupRenderTarget();

	
	void CleanupDeviceD3D();

	void CreateRenderTarget();

	void presentCallback(void* chain);


};

extern void PresentCallBack(void* chain);


NAMESPACE_END;