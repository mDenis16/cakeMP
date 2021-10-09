#pragma once
#include <mutex>

#include "d3d11.h"
using namespace  std;
class FrameBuffer
{
public:
	FrameBuffer(shared_ptr<d3d11::Device> const& device)
		: device_(device)
		, dirty_(false)
	{
	}

	int32_t width() {
		if (shared_buffer_) {
			return shared_buffer_->width();
		}
		return 0;
	}

	int32_t height() {
		if (shared_buffer_) {
			return shared_buffer_->height();
		}
		return 0;
	}

	void on_paint(const void* buffer, uint32_t width, uint32_t height)
	{
		uint32_t stride = width * 4;
		size_t cb = stride * height;

		if (!shared_buffer_ ||
			(shared_buffer_->width() != width) ||
			(shared_buffer_->height() != height))
		{
			shared_buffer_ = device_->create_texture(
				width, height, DXGI_FORMAT_B8G8R8A8_UNORM, nullptr, 0);

			sw_buffer_ = shared_ptr<uint8_t>((uint8_t*)malloc(cb), free);
		}

		if (sw_buffer_ && buffer)
		{
			// todo: support dirty rect(s)
			memcpy(sw_buffer_.get(), buffer, cb);
		}

		dirty_ = true;
	}

	//
	// called in response to Cef's OnAcceleratedPaint notification
	//
	void on_gpu_paint(void* shared_handle)
	{
		// Note: we're not handling keyed mutexes yet

		lock_guard<mutex> guard(lock_);

		// did the shared texture change?
		if (shared_buffer_)
		{
			if (shared_handle != shared_buffer_->share_handle()) {
				shared_buffer_.reset();
			}
		}

		// open the shared texture
		if (!shared_buffer_)
		{
			shared_buffer_ = device_->open_shared_texture((void*)shared_handle);
			if (!shared_buffer_) {
				sbp::logWrite("Error!!! could not open shared texture!");
			}
		}

		dirty_ = true;
	}

	//
	// this method returns what should be considered the front buffer
	// we're simply using the shared texture directly 
	//
	// ... this method could be expanded on to handle 
	// synchronization through a keyed mutex
	// 
	shared_ptr<d3d11::Texture2D> swap(shared_ptr<d3d11::Context> const& ctx)
	{
		lock_guard<mutex> guard(lock_);

		// using software buffer? just copy to texture
		if (sw_buffer_ && shared_buffer_ && dirty_)
		{
			d3d11::ScopedBinder<d3d11::Texture2D> binder(ctx, shared_buffer_);
			shared_buffer_->copy_from(
				sw_buffer_.get(),
				shared_buffer_->width() * 4,
				shared_buffer_->height());
		}

		dirty_ = false;
		return shared_buffer_;
	}

private:

	mutex lock_;
	atomic_bool abort_;
	shared_ptr<d3d11::Texture2D> shared_buffer_;
	std::shared_ptr<d3d11::Device> const device_;
	shared_ptr<uint8_t> sw_buffer_;
	bool dirty_;
};
