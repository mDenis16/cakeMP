#include <Common.h>
#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_client.h>
#include <include/cef_version.h>
#include "helper.h"

#include "CakeWebView.h"
#include "CefModule.h"

#include <functional>
#include <iostream>
#include <shv/main.h>

#include "CakeWebView.h"
#include "ImageLayer.h"


void CefModule::InitComposition()
{
	composition_ = std::make_shared<Composition>(device_, _width, _height);

}
/*
 *
 *IDXGISwapChain*,
			ID3D11RenderTargetView*,
			ID3D11SamplerState*,
			ID3D11BlendState*
 *
 */
void CefModule::InitDevice(ID3D11Device* pDevice, ID3D11DeviceContext* context, IDXGISwapChain* swapchain, ID3D11RenderTargetView* pRenderTargetview, ID3D11BlendState* pBlender, ID3D11SamplerState* pSampler)
{
	sbp::logWrite("CefModule: Init Device\n");
	device_ = std::make_unique<d3d11::Device>(pDevice, context);
	sbp::logWrite("CefModule: Init Swapchain\n");
	swapchain_ = std::make_unique<d3d11::SwapChain>(swapchain, pRenderTargetview, pSampler, pBlender);
}


CefModule::CefModule(HMODULE hmodule)
{
	

	this->m_HInstance = hmodule;

	sbp::logWrite("Initialzied CEF Module class.\n");
}


/*
 *std::shared_ptr<Device> import_device(ID3D11Device* device, ID3D11DeviceContext* devicecontext)
{
	auto const dev = std::make_shared<Device>(device, devicecontext);


	log_message("d3d11: selected adapter: %s\n", dev->adapter_name().c_str());

	log_message("d3d11: selected feature level: 0x%04X\n", 0);

	return dev;
}
 *
 */
CefModule::~CefModule()
{

	/*if (this->serviceWorkedThread_)
	{
		CefRefPtr<CefTask> task(new QuitTask());
		CefPostTask(TID_UI, task.get());
		this->serviceWorkedThread_->join();

		delete serviceWorkedThread_;
	}
	*/

}



void CefModule::ServiceWorker()
{
	

		CefSettings settings;
		settings.no_sandbox = true;

		settings.windowless_rendering_enabled = true;

		settings.multi_threaded_message_loop = false;

		CefString(&settings.browser_subprocess_path).FromASCII("D:\\repos\\cakeMP\\CakeMPCefLauncher\\x64\\Release\\CakeMPCefLauncher.exe");

		CefRefPtr<WebApp> app(new WebApp());

		CefMainArgs main_args(nullptr);

		CefInitialize(main_args, settings, app, nullptr);
	
		CefRunMessageLoop();

}

void CefModule::CreateWebView(std::string const& url)
{
	CefWindowInfo window_info;
	window_info.SetAsWindowless(nullptr);

	window_info.shared_texture_enabled = true;
	window_info.windowless_rendering_enabled = true;
	window_info.external_begin_frame_enabled = false;

	CefBrowserSettings settings;
	settings.windowless_frame_rate = 60;


	string name;

	auto webPointer = new WebView("samp", _width, _height,
		window_info.shared_texture_enabled,
		window_info.external_begin_frame_enabled, device_);

	CefRefPtr<WebView> view(webPointer);

	CefBrowserHost::CreateBrowser(
		window_info,
		view.get(),
		url,
		settings,
		nullptr);

	auto layer = make_shared<WebLayer>(device_, true, view);

	composition_->add_layer(layer);



	layer->move(0, 0, 1.0f, 1.0f);
	webList.push_back(view);


}

CefModule* CefModuleManager = nullptr;

