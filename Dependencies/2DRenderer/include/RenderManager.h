#pragma once
#include "DLLCommon.h"

struct GLFWwindow;

class RENDERER_API RenderManager
{
protected:
	RenderManager();
	~RenderManager();

public:
	static RenderManager& GetInstance()
	{
		if (!mInstance)
			mInstance = new RenderManager();

		return *mInstance;
	}

	RenderManager(const RenderManager&) = delete;
	RenderManager& operator=(const RenderManager&) = delete;

	void Init();
	void Quit();

protected:
	void InitOpenGL();
	void CreateGraphicObjects();
	void LoadTexture();
	
	void RenderLoop();

protected:
	static RenderManager* mInstance;

	GLFWwindow* mWindow;
};

