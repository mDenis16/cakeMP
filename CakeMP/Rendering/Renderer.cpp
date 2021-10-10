
#include <Common.h>

#include <Rendering/Renderer.h>
#include <shv/main.h>


#include <include/cef_command_line.h>
#include <include/cef_sandbox_win.h>
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include "CefApp/CakeWebView.h"
#include <iostream>


#include <d3d11_1.h>

#include "CefApp/CefModule.h"
#include "CefApp/d3d11.h"
#include <wrl\client.h>
using namespace Microsoft::WRL;


NAMESPACE_BEGIN

Renderer::~Renderer()
{


}

Renderer::Renderer()
{
}
void Renderer::CleanupRenderTarget()
{
	if (pRenderTargetView) { pRenderTargetView->Release(); pRenderTargetView = NULL; }
}
void Renderer::CleanupDeviceD3D()
{

	CleanupRenderTarget();
	if (pSwapChain) { pSwapChain->Release(); pSwapChain = nullptr; }
	if (pDeviceContext) { pDeviceContext->Release(); pDeviceContext = nullptr; }
	if (pDevice) { pDevice->Release(); pDevice = nullptr; }
}
void Renderer::CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC sd;
	pSwapChain->GetDesc(&sd);

	// Create the render target
	ID3D11Texture2D* pBackBuffer;
	D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
	ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
	render_target_view_desc.Format = sd.BufferDesc.Format;
	render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	pDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &pRenderTargetView);
	pDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, NULL);
	pBackBuffer->Release();
}

void Renderer::presentCallback(void* chain)
{
	
	if (!pSwapChain)
	{
		pSwapChain = (IDXGISwapChain*)chain;
		if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (LPVOID*)&this->pDevice)))
		{
			logWrite("Renderer : Failed to Get Device Ptr!");
			return;
		}
		if (!pDevice)
		{
			logWrite("Renderer : pDevice invalid");
			return;
		}
		this->pDevice->GetImmediateContext(&this->pDeviceContext);

		if (!this->pDeviceContext)
		{
			logWrite("Renderer : Failed to Get DeviceContext Ptr");
			return;
		}


		{
			ID3D11Texture2D* d3d11FrameBuffer;
			HRESULT hResult = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3d11FrameBuffer);
			assert(SUCCEEDED(hResult));

			hResult = pDevice->CreateRenderTargetView(d3d11FrameBuffer, 0, &pRenderTargetView);
			assert(SUCCEEDED(hResult));

			pDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, NULL);

			

			d3d11FrameBuffer->Release();


			// create a default sampler to use

			{
				D3D11_SAMPLER_DESC desc = {};
				desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
				desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
				desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
				desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
				desc.MinLOD = 0.0f;
				desc.MaxLOD = D3D11_FLOAT32_MAX;
				desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
				hResult = pDevice->CreateSamplerState(&desc, &pSampler);
				assert(SUCCEEDED(hResult));
			}

			// create a default blend state to use (pre-multiplied alpha)

			{
				D3D11_BLEND_DESC desc;
				desc.AlphaToCoverageEnable = FALSE;
				desc.IndependentBlendEnable = FALSE;
				auto const count = sizeof(desc.RenderTarget) / sizeof(desc.RenderTarget[0]);
				for (size_t n = 0; n < count; ++n)
				{
					desc.RenderTarget[n].BlendEnable = TRUE;
					desc.RenderTarget[n].SrcBlend = D3D11_BLEND_ONE;
					desc.RenderTarget[n].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
					desc.RenderTarget[n].SrcBlendAlpha = D3D11_BLEND_ONE;
					desc.RenderTarget[n].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
					desc.RenderTarget[n].BlendOp = D3D11_BLEND_OP_ADD;
					desc.RenderTarget[n].BlendOpAlpha = D3D11_BLEND_OP_ADD;
					desc.RenderTarget[n].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				}
				hResult = pDevice->CreateBlendState(&desc, &pBlender);
				assert(SUCCEEDED(hResult));
			}


			CefModuleManager->InitDevice(pDevice, pDeviceContext, pSwapChain, pRenderTargetView, pBlender, pSampler);
			CefModuleManager->_width = 1920; CefModuleManager->_height = 1080;
			CefModuleManager->InitComposition();

			CefModuleManager->CreateWebView("https://www.youtube.com/embed/9t7HxGW8ACo?autoplay=1");
	       
			
		}
	}

	if (this->pSwapChain && pDevice && pDeviceContext && pRenderTargetView)
	{

		//pDeviceContext->ClearRenderTargetView(pRenderTargetView, backgroundColor);
		auto ctx = CefModuleManager->device_->immedidate_context();

		if (!ctx || !CefModuleManager->swapchain_) return;

		CefModuleManager->swapchain_->bind(ctx);

		//CefModuleManager->swapchain_->clear(0.0f, 0.0f, 0.0f, 1.0f);

		CefModuleManager->composition_->render(ctx);

	
		//pSwapChain->Present(1, 0);
	}
}




	NAMESPACE_END
