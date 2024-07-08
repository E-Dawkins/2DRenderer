#include "RenderManager.h"

#include <glfw3.h>

RenderManager* RenderManager::mInstance = nullptr;

RenderManager::RenderManager()
{
    mWindow = nullptr;
}

RenderManager::~RenderManager()
{
    delete mWindow;
}

void RenderManager::Init()
{
    if (!glfwInit())
        return;

    mWindow = glfwCreateWindow(640, 480, "2D Renderer", NULL, NULL);
    if (!mWindow)
    {
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(mWindow);

    glClearColor(1, 1, 1, 1);

    while (!glfwWindowShouldClose(mWindow))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(mWindow);

        glfwPollEvents();
    }

    glfwTerminate();
}

void RenderManager::Quit()
{

}
