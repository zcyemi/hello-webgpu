#pragma once
#include "webgpu.h"
#include <string.h>

class AppMain
{

public:
	AppMain();
	~AppMain();

	bool Frame();
public:
	WGPUDevice device;
	WGPUQueue queue;
	WGPUSwapChain swapchain;

public:

	bool InitWGPU(window::Handle wHnd);

	void InitRendering();

private:
	void ReleaseRendering();
};

