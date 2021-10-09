#pragma once
#include <mutex>
#include <d3d11.h>
#include <iostream>
#include "helper.h"

#include "FrameBuffer.h"
#include <include\cef_process_message.h>
#include <include\cef_browser.h>
#include <include\cef_render_handler.h>
#include <include\cef_load_handler.h>
#include <include\cef_life_span_handler.h>
#include <include\cef_client.h>
#include <include\cef_task.h>

using namespace std;


class WebView : public CefClient,
	public CefRenderHandler,
	public CefLifeSpanHandler,
	public CefLoadHandler
{
public:
	WebView(
		string name,
		int width,
		int height,
		bool use_shared_textures,
		bool send_begin_Frame,
		shared_ptr<d3d11::Device> const& device
		)
		: name_(name)
		, width_(width)
		, height_(height)
		, use_shared_textures_(use_shared_textures)
		, send_begin_frame_(send_begin_Frame)
		, device_(device)
	
	{
		view_buffer_ = make_shared<FrameBuffer>(device_);
		frame_ = 0;
		fps_start_ = 0ull;
	}

	~WebView() {
		close();
	}
	shared_ptr<d3d11::Texture2D> texture(shared_ptr<d3d11::Context> const& ctx)
	{
		if (view_buffer_) {
			return view_buffer_->swap(ctx);
		}
		return nullptr;
	}

	bool use_shared_textures() const {
		return use_shared_textures_;
	}

	std::condition_variable signal_;
	std::atomic_bool ready_;


	void close()
	{
		// get thread-safe reference
		decltype(browser_) browser;
		{
			lock_guard<mutex> guard(lock_);
			browser = browser_;
			browser_ = nullptr;
		}

		if (browser.get()) {
			browser->GetHost()->CloseBrowser(true);
		}

	}

	CefRefPtr<CefRenderHandler> GetRenderHandler() override {
		return this;
	}

	CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
		return this;
	}

	CefRefPtr<CefLoadHandler> GetLoadHandler() override {
		return this;
	}

	bool OnProcessMessageReceived(
		CefRefPtr<CefBrowser> /*browser*/,
		CefProcessId /*source_process*/,
		CefRefPtr<CefProcessMessage> message) override
	{
		auto name = message->GetName().ToString();
		if (name == "mixer-request-stats")
		{
			// just flag that we need to deliver stats updates
			// to the render process via a message
			needs_stats_update_ = true;
			return true;
		}
		return false;
	}

	void OnPaint(
		CefRefPtr<CefBrowser> /*browser*/,
		PaintElementType type,
		const RectList& dirtyRects,
		const void* buffer,
		int width,
		int height) override
	{
		// this application doesn't support software rasterizing
	

		if (type == PET_VIEW)
		{
			frame_++;
			auto const now = time_now();
			if (!fps_start_) {
				fps_start_ = now;
			}

			/*if (view_buffer_) {
				view_buffer_->on_paint(buffer, width, height);
			}*/

			if ((now - fps_start_) > 1000000)
			{
				auto const fps = frame_ / double((now - fps_start_) / 1000000.0);

				auto const w = view_buffer_ ? view_buffer_->width() : 0;
				auto const h =  view_buffer_ ? view_buffer_->height() : 0;

				log_message("html: OnAcceleratedPaint (%dx%d), fps: %3.2f\n", w, h, fps);

				frame_ = 0;
				fps_start_ = time_now();
			}
		}
		else
		{
			// just update the popup frame ... we are only tracking 
			// metrics for the view

			/*if (popup_buffer_) {
				popup_buffer_->on_paint(buffer, width, height);
			}*/
		}

	}
	
	void OnAcceleratedPaint(
		CefRefPtr<CefBrowser> /*browser*/,
		PaintElementType type,
		const RectList& dirtyRects,
		void* share_handle) override
	{

		if (type == PET_VIEW)
		{
			frame_++;
			auto const now = time_now();
			if (!fps_start_) {
				fps_start_ = now;
			}
			if (type == PET_VIEW)
			{
				if (view_buffer_) {
					view_buffer_->on_gpu_paint((void*)share_handle);
				}
			}
			

			if ((now - fps_start_) > 1000000)
			{
				auto const fps = frame_ / double((now - fps_start_) / 1000000.0);

				auto const w = view_buffer_ ? view_buffer_->width() : 0;
				auto const h =  view_buffer_ ? view_buffer_->height() : 0;

				sbp::logWrite("html: OnAcceleratedPaint (%dx%d), fps: %3.2f\n", w, h, fps);
				
				frame_ = 0;
				fps_start_ = time_now();
			}
		}
		
	}

	void GetViewRect(CefRefPtr<CefBrowser> /*browser*/, CefRect& rect) override
	{
		rect.Set(0, 0, width_, height_);
	}

	void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
	{
		printf("Browser created. \n");
		if (!CefCurrentlyOn(TID_UI))
		{
			assert(0);
			return;
		}

		{
			lock_guard<mutex> guard(lock_);
			if (!browser_.get()) {
				browser_ = browser;
			}
		}
	}


	void OnLoadEnd(CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		int /*httpStatusCode*/)
	{

		printf("OnLoadEnd () Browser called\n");
		ready_ = true;

		//frame->ExecuteJavaScript(CefString("console.error('asd')"), CefString(""), 0);


		//dump_source(frame);
	}

	/*shared_ptr<d3d11::Texture2D> texture(shared_ptr<d3d11::Context> const& ctx)
	{
		if (view_buffer_) {
			return view_buffer_->swap(ctx);
		}
		return nullptr;
	}*/


	//void update_stats(CefRefPtr<CefBrowser> const& browser,
	//	shared_ptr<Composition> const& composition)
	//{
	//	if (!browser || !composition) {
	//		return;
	//	}

	//	auto message = CefProcessMessage::Create("mixer-update-stats");
	//	auto args = message->GetArgumentList();

	//	// create a dictionary to hold all of individual statistic values
	//	// it will get converted by the Render process into a V8 Object
	//	// that gets passed to the script running in the page
	//	auto dict = CefDictionaryValue::Create();
	//	dict->SetInt("width", composition->width());
	//	dict->SetInt("height", composition->height());
	//	dict->SetDouble("fps", composition->fps());
	//	dict->SetDouble("time", composition->time());
	//	dict->SetBool("vsync", composition->is_vsync());

	//	args->SetDictionary(0, dict);

	//	browser->SendProcessMessage(PID_RENDERER, message);
	//}

	void resize(int width, int height)
	{
		// only signal change if necessary
		if (width != width_ || height != height_)
		{
			width_ = width;
			height_ = height;

			auto const browser = safe_browser();
			if (browser)
			{
				browser->GetHost()->WasResized();
				log_message("html resize - %dx%d\n", width, height);
			}
		}
	}

	void dump_source(CefRefPtr<CefFrame> frame)
	{
		/*if (frame.get() && !name_.empty())
		{
			auto const filename = get_temp_filename(name_);
			CefRefPtr<CefStringVisitor> writer(new HtmlSourceWriter(filename));
			frame->GetSource(writer);
		}*/
	}

	//
	// from Masako Toda
	//
	void show_devtools()
	{
		
	}

	void mouse_click(MouseButton button, bool up, int32_t x, int32_t y)
	{
		/*auto const browser = safe_browser();
		if (browser)
		{
			CefMouseEvent mouse;
			mouse.x = x;
			mouse.y = y;
			mouse.modifiers = 0;

			cef_mouse_button_type_t ctype;
			switch (button)
			{
			case MouseButton::Middle: ctype = MBT_MIDDLE; break;
			case MouseButton::Right: ctype = MBT_RIGHT; break;
			case MouseButton::Left: ctype = MBT_LEFT;
			default:break;
			}
			browser->GetHost()->SendMouseClickEvent(mouse, ctype, up, 1);
		}*/
	}

	void mouse_move(bool leave, int32_t x, int32_t y)
	{
		auto const browser = safe_browser();
		if (browser)
		{
			CefMouseEvent mouse;
			mouse.x = x;
			mouse.y = y;
			mouse.modifiers = 0;
			browser->GetHost()->SendMouseMoveEvent(mouse, leave);
		}
	}
	shared_ptr<FrameBuffer> view_buffer_;
	ID3D11Texture2D* shared_buffer_ = nullptr;
	shared_ptr<d3d11::Device> const device_;
private:
	IMPLEMENT_REFCOUNTING(WebView);

	CefRefPtr<CefBrowser> safe_browser()
	{
		lock_guard<mutex> guard(lock_);
		return browser_;
	}
	

	string name_;
	int width_;
	int height_;
	uint32_t frame_;
	uint64_t fps_start_;
	mutex lock_;
	CefRefPtr<CefBrowser> browser_;
	bool needs_stats_update_;
	bool use_shared_textures_;
	bool send_begin_frame_;

	
};
