#include "AppMain.h"

AppMain::AppMain()
{
}

AppMain::~AppMain()
{
	ReleaseRendering();

	wgpuSwapChainRelease(swapchain);
	wgpuQueueRelease(queue);
	wgpuDeviceRelease(device);
}

bool AppMain::Frame()
{
	return false;
}

bool AppMain::InitWGPU(window::Handle wHnd)
{
	device = webgpu::create(wHnd);
	if (device == nullptr) return false;

	queue = wgpuDeviceGetQueue(device);
	swapchain = webgpu::createSwapChain(device);


	return true;

}

void AppMain::InitRendering()
{
}

void AppMain::ReleaseRendering()
{

}

