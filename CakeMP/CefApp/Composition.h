#pragma once

#include "d3d11.h"
#include <vector>
#include <mutex>

#include "helper.h"

// basic rect for floats
struct Rect
{
	float x;
	float y;
	float width;
	float height;
};

class Composition;




//
// a simple abstraction for a 2D layer within a composition
// 
// see image_layer.cpp or html_layer.cpp for example implementations
//
class Layer
{
public:
	Layer(std::shared_ptr<d3d11::Device> const& device, bool want_input, bool flip);
	~Layer();

	virtual void attach(std::shared_ptr<Composition> const&);

	virtual void move(float x, float y, float width, float height);

	virtual void tick(double);
	virtual void render(std::shared_ptr<d3d11::Context> const&) = 0;

	virtual void mouse_click(MouseButton button, bool up, int32_t x, int32_t y);
	virtual void mouse_move(bool leave, int32_t x, int32_t y);

	Rect bounds() const;

	std::shared_ptr<Composition> composition() const;

	bool want_input() const;

protected:

	void render_texture(
		std::shared_ptr<d3d11::Context> const& ctx,
		std::shared_ptr<d3d11::Texture2D> const& texture);

	bool flip_;
	Rect bounds_;
	bool want_input_;

	std::shared_ptr<d3d11::Geometry> geometry_;
	std::shared_ptr<d3d11::Effect> effect_;
	std::shared_ptr<d3d11::Device> const device_;

private:
	std::weak_ptr<Composition> composition_;
};

//
// A collection of layers. 
// A composition will render 1-N layers to a D3D11 device
//
class Composition : public std::enable_shared_from_this<Composition>
{
public:
	Composition(std::shared_ptr<d3d11::Device> const& device,
		int width, int height);

	int width() const { return width_; }
	int height() const { return height_; }

	double fps() const;
	double time() const;

	bool is_vsync() const;

	void tick(double);
	void render(std::shared_ptr<d3d11::Context> const&);

	void add_layer(std::shared_ptr<Layer> const& layer);
	bool remove_layer(std::shared_ptr<Layer> const& layer);

	void resize(bool vsync, int width, int height);

	void mouse_click(MouseButton button, bool up, int32_t x, int32_t y);
	void mouse_move(bool leave, int32_t x, int32_t y);

private:

	std::shared_ptr<Layer> layer_from_point(int32_t& x, int32_t& y);

	int width_;
	int height_;
	uint32_t frame_;
	int64_t fps_start_;
	double fps_;
	double time_;
	bool vsync_;
	std::shared_ptr<d3d11::Device> const device_;
	std::vector<std::shared_ptr<Layer>> layers_;
	std::mutex lock_;
};

int cef_initialize(HINSTANCE);
void cef_uninitialize();
std::string cef_version();

// create a composition from a JSON string
std::shared_ptr<Composition> create_composition(
	std::shared_ptr<d3d11::Device> const& device,
	std::string const& json);

// create a layer to show a image
std::shared_ptr<Layer> create_image_layer(
	std::shared_ptr<d3d11::Device> const& device,
	std::string const& file_name);

// create a layer to show a web page (using CEF)
std::shared_ptr<Layer> create_web_layer(
	std::shared_ptr<d3d11::Device> const& device,
	std::string const& url,
	int width,
	int height,
	bool want_input,
	bool view_source);