#pragma once
#include "helper.h"
#include "composition.h"

#include <wincodec.h>

#include "CakeWebView.h"

using namespace std;

class ImageLayer : public Layer
{
public:
	ImageLayer(
		std::shared_ptr<d3d11::Device> const& device,
		std::shared_ptr<d3d11::Texture2D> const& texture)
		: Layer(device, false, false)
		, texture_(texture)
	{
	}

	void render(shared_ptr<d3d11::Context> const& ctx) override
	{
		// simply use the base class method to draw our texture
		render_texture(ctx, texture_);
	}

private:

	shared_ptr<d3d11::Texture2D> const texture_;
};

//
// use WIC to load a texture from an image file
//
class WebLayer : public Layer
{
public:
	WebLayer(
		std::shared_ptr<d3d11::Device> const& device,
		bool want_input,
		CefRefPtr<WebView> const& view)
		: Layer(device, want_input, view->use_shared_textures())
		, view_(view) {
	}

	~WebLayer() {
		if (view_) {
			view_->close();
		}
	}





	void render(shared_ptr<d3d11::Context> const& ctx) override
	{
		// simply use the base class method to draw our texture
		if (view_) {
			render_texture(ctx, view_->texture(ctx));
		}
	}

	void mouse_click(MouseButton button, bool up, int32_t x, int32_t y) override
	{
		if (view_) {
			view_->mouse_click(button, up, x, y);
		}
	}

	void mouse_move(bool leave, int32_t x, int32_t y) override
	{
		if (view_) {
			view_->mouse_move(leave, x, y);
		}
	}

private:

	CefRefPtr<WebView> const view_;
};
