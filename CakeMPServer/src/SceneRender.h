#pragma once

#include <thread>

class SceneRender
{
public:
	void Init();
	

	std::thread* renderThread;
	void OnRender();
};
