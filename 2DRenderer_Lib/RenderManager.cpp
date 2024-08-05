#include "RenderManager.h"

#define GLFW_INCLUDE_NONE
#include <glfw3.h>
#define GLEW_STATIC
#include <glew.h>

#include "Texture2D.h"

static const GLchar* vertexSource = R"glsl(
    #version 150 core
    in vec2 position;
    in vec3 color;
    in vec2 texcoord;
    out vec3 Color;
    out vec2 Texcoord;
    void main()
    {
        Color = color;
        Texcoord = texcoord;
        gl_Position = vec4(position, 0.0, 1.0);
    }
)glsl";
static const GLchar* fragmentSource = R"glsl(
    #version 150 core
    in vec3 Color;
    in vec2 Texcoord;
    out vec4 outColor;
    uniform sampler2D tex;
    uniform float gamma;
    void main()
    {
        vec3 color = texture(tex, Texcoord).rgb;
        float alpha = texture(tex, Texcoord).a;
        color = pow(color, vec3(1.0 / gamma));
        outColor = vec4(color, alpha);
    }
)glsl";

static GLfloat vertices[] = {
    // Position     Color               Texcoord
    -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, // Top-left
     0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Top-right
     0.5f, -0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f, // Bottom-right
    -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,   0.0f, 1.0f, // Bottom-left
};

static GLuint elements[] = {
    0, 1, 2,
    2, 3, 0
};

RenderManager* RenderManager::mInstance = nullptr;

RenderManager::RenderManager()
{
    mWindow = nullptr;
    mShaderProgram = 0;
}

RenderManager::~RenderManager()
{

}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void RenderManager::Init()
{
    InitOpenGL();
    CreateGraphicObjects();
    LoadTexture();

    RenderLoop();
}

void RenderManager::Quit()
{
    glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
}

void RenderManager::InitOpenGL()
{
    // Initialize GLFW
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        return;

    // Create GLFW window and make it the current context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    mWindow = glfwCreateWindow(640, 640, "2D Renderer", NULL, NULL);
    if (!mWindow)
    {
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(mWindow);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    glewInit();

    glfwSwapInterval(1);
}

void RenderManager::CreateGraphicObjects()
{
    // Create vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create vertex buffer object
    GLuint vbo;
    glGenBuffers(1, &vbo); // Generate 1 buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create element buffer object
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    // Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Link the vertex & fragment shader into a shader program
    mShaderProgram = glCreateProgram();
    glAttachShader(mShaderProgram, vertexShader);
    glAttachShader(mShaderProgram, fragmentShader);
    glBindFragDataLocation(mShaderProgram, 0, "outColor");
    glLinkProgram(mShaderProgram);
    glUseProgram(mShaderProgram);

    // Specify the layout of the vertex data
    constexpr int vertDataSize = 7 * sizeof(GLfloat);

    GLint posAttrib = glGetAttribLocation(mShaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, vertDataSize, 0);

    GLint colAttrib = glGetAttribLocation(mShaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, vertDataSize, (void*)(2 * sizeof(GLfloat)));

    GLint texAttrib = glGetAttribLocation(mShaderProgram, "texcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, vertDataSize, (void*)(5 * sizeof(GLfloat)));
}

void RenderManager::LoadTexture()
{
    // Create texture
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    Texture2D t = Texture2D("./PNGSuite/1-interlacing/basi6a16.png");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.mPNGProps.width, t.mPNGProps.height, 0, GL_RGBA, GL_FLOAT, t.mPNGProps.pixels.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Bind gamma value
    GLint gammaLocation = glGetUniformLocation(mShaderProgram, "gamma");
    glUniform1f(gammaLocation, t.mPNGProps.gamma);
}

void RenderManager::RenderLoop()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        int width, height;
        glfwGetWindowSize(mWindow, &width, &height);

        glViewport(0, 0, width, height);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(mWindow);
        glfwPollEvents();
    }

    glfwDestroyWindow(mWindow);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
