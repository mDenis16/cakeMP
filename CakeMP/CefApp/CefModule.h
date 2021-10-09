#pragma once
#include <mutex>
#include <atomic>
#include <d3d11.h>
#include <include\cef_render_handler.h>
#include <include\cef_v8.h>

#include "Composition.h"
#include "d3d11.h"
#include <include\cef_app.h>


//
// helper function to convert a 
// CefDictionaryValue to a CefV8Object
//
inline CefRefPtr<CefV8Value> to_v8object(CefRefPtr<CefDictionaryValue> const& dictionary)
{
	auto const obj = CefV8Value::CreateObject(nullptr, nullptr);
	if (dictionary)
	{
		auto const attrib = V8_PROPERTY_ATTRIBUTE_READONLY;
		CefDictionaryValue::KeyList keys;
		dictionary->GetKeys(keys);
		for (auto const& k : keys)
		{
			auto const type = dictionary->GetType(k);
			switch (type)
			{
			case VTYPE_BOOL: obj->SetValue(k,
				CefV8Value::CreateBool(dictionary->GetBool(k)), attrib);
				break;
			case VTYPE_INT: obj->SetValue(k,
				CefV8Value::CreateInt(dictionary->GetInt(k)), attrib);
				break;
			case VTYPE_DOUBLE: obj->SetValue(k,
				CefV8Value::CreateDouble(dictionary->GetDouble(k)), attrib);
				break;
			case VTYPE_STRING: obj->SetValue(k,
				CefV8Value::CreateString(dictionary->GetString(k)), attrib);
				break;

			default: break;
			}
		}
	}
	return obj;
}

//
// V8 handler for our 'mixer' object available to javascript
// running in a page within this application
//
class MixerHandler :
	public CefV8Accessor
{
public:
	MixerHandler(
		CefRefPtr<CefBrowser> const& browser,
		CefRefPtr<CefV8Context> const& context)
		: browser_(browser)
		, context_(context)
	{
		auto window = context->GetGlobal();
		auto const obj = CefV8Value::CreateObject(this, nullptr);
		obj->SetValue("requestStats", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
		window->SetValue("mixer", obj, V8_PROPERTY_ATTRIBUTE_NONE);
	}

	void update(CefRefPtr<CefDictionaryValue> const& dictionary)
	{
		if (!request_stats_) {
			return;
		}

		context_->Enter();
		CefV8ValueList values;
		values.push_back(to_v8object(dictionary));
		request_stats_->ExecuteFunction(request_stats_, values);
		context_->Exit();
	}

	bool Get(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		CefRefPtr<CefV8Value>& retval,
		CefString& /*exception*/) override {

		if (name == "requestStats" && request_stats_ != nullptr) {
			retval = request_stats_;
			return true;
		}

		// Value does not exist.
		return false;
	}

	bool Set(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		const CefRefPtr<CefV8Value> value,
		CefString& /*exception*/) override
	{
		if (name == "requestStats") {
			request_stats_ = value;

			// notify the browser process that we want stats
			auto message = CefProcessMessage::Create("mixer-request-stats");
			if (message != nullptr && browser_ != nullptr) {
				browser_->SendProcessMessage(PID_BROWSER, message);
			}
			return true;
		}
		return false;
	}

	IMPLEMENT_REFCOUNTING(MixerHandler);

private:

	CefRefPtr<CefBrowser> const browser_;
	CefRefPtr<CefV8Context> const context_;
	CefRefPtr<CefV8Value> request_stats_;
};



class WebApp : public CefApp,
	public CefBrowserProcessHandler,
	public CefRenderProcessHandler
{
public:
	WebApp() {
	}

	
	void OnBeforeCommandLineProcessing(
		CefString const& /*process_type*/,
		CefRefPtr<CefCommandLine> command_line) override
	{
		// disable creation of a GPUCache/ folder on disk
		command_line->AppendSwitch("disable-gpu-shader-disk-cache");
		command_line->AppendSwitch("off-screen-rendering-enabled");
		//command_line->AppendSwitch("disable-accelerated-video-decode");

		// un-comment to show the built-in Chromium fps meter
		command_line->AppendSwitch("show-fps-counter");		

		//command_line->AppendSwitch("disable-gpu-vsync");

		// Most systems would not need to use this switch - but on older hardware, 
		// Chromium may still choose to disable D3D11 for gpu workarounds.
		// Accelerated OSR will not at all with D3D11 disabled, so we force it on.
		//
		// See the discussion on this issue:
		// https://github.com/daktronics/cef-mixer/issues/10
		//
		command_line->AppendSwitchWithValue("use-angle", "d3d11");

		// tell Chromium to autoplay <video> elements without 
		// requiring the muted attribute or user interaction
		command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");

#if !defined(NDEBUG)
		// ~RenderProcessHostImpl() complains about DCHECK(is_self_deleted_)
		// when we run single process mode ... I haven't figured out how to resolve yet
		//command_line->AppendSwitch("single-process");
#endif
	}

	virtual void OnContextInitialized() override {
	}

	//
	// CefRenderProcessHandler::OnContextCreated
	//
	// Adds our custom 'mixer' object to the javascript context running
	// in the render process
	//
	void OnContextCreated(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefV8Context> context) override
	{
		mixer_handler_ = new MixerHandler(browser, context);
	}

	//
	// CefRenderProcessHandler::OnBrowserDestroyed
	//
	void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override
	{
		mixer_handler_ = nullptr;
	}

	//
	// CefRenderProcessHandler::OnProcessMessageReceived
	//
	bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
		CefProcessId /*source_process*/,
		CefRefPtr<CefProcessMessage> message)
	{
		auto const name = message->GetName().ToString();
		if (name == "mixer-update-stats")
		{
			if (mixer_handler_ != nullptr)
			{
				// we expect just 1 param that is a dictionary of stat values
				auto const args = message->GetArgumentList();
				auto const size = args->GetSize();
				if (size > 0) {
					auto const dict = args->GetDictionary(0);
					if (dict && dict->GetSize() > 0) {
						mixer_handler_->update(dict);
					}
				}
			}
			return true;
		}
		return false;
	}

private:

	IMPLEMENT_REFCOUNTING(WebApp);

	CefRefPtr<MixerHandler> mixer_handler_;
};



class CefModule
{
public:
	CefModule(HMODULE  hmodule);
	~CefModule();
	void InitDevice(shared_ptr<d3d11::Device> const& device);

	 static void ServiceWorker();


	int _width, _height = 0;

	std::condition_variable signal_;
	std::atomic_bool ready_;
	std::mutex lock_;

	std::thread* serviceWorkedThread_;

	HMODULE m_HInstance;

	static CefModule* cefModuleInstance;

	void CreateWebView(std::string const& url);
	
	void InitComposition();
	void InitDevice(ID3D11Device* pDevice, ID3D11DeviceContext* context, IDXGISwapChain* swapchain, ID3D11RenderTargetView* pRenderTargetview, ID3D11BlendState* pBlender, ID3D11SamplerState* pSampler);

	std::shared_ptr<d3d11::Device>  device_ = nullptr ;
	std::shared_ptr<d3d11::SwapChain> swapchain_ = nullptr;
	std::vector< CefRefPtr<WebView> > webList;
	std::shared_ptr<Composition>  composition_;
private:
	class QuitTask : public CefTask
	{
	public:
		QuitTask() { }
		void Execute() override {
			CefQuitMessageLoop();
		}
		IMPLEMENT_REFCOUNTING(QuitTask);
	};

};

extern CefModule* CefModuleManager;





