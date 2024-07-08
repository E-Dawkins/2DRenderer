#pragma once
#include "DLLCommon.h"

struct GLFWwindow;

class RENDERER_API RenderManager
{
public:
	static RenderManager& GetInstance()
	{
		if (!mInstance)
			mInstance = new RenderManager();

		return *mInstance;
	}

	void Init();
	void Quit();

	RenderManager(const RenderManager&) = delete;
	RenderManager& operator=(const RenderManager&) = delete;

protected:
	RenderManager();
	~RenderManager();

protected:
	static RenderManager* mInstance;

	GLFWwindow* mWindow;
};

