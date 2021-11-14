#include "glad/glad.h"

#include <iostream>
#include <GLFW/glfw3.h>

#include "Game.h"

using namespace std;

constexpr auto WINDOW_WIDTH = 1280;
constexpr auto WINDOW_HEIGHT = 720;

Game* game = nullptr;

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    
    if (game)
        game->OnResize(width, height);
}

void MouseCallback(GLFWwindow* window, double posX, double posY)
{
    if (game)
        game->OnMouseMoved(posX, posY);
}

void ProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (game)
        game->ProcessInput(window);
}

int main(int argc, char const* argv[])
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Biliard", NULL, NULL);

    if (window == NULL)
    {
        cout << "Failed to create GLFW window." << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, MouseCallback);

    game = new Game(WINDOW_WIDTH, WINDOW_HEIGHT);

    float previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - previousTime;

        ProcessInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        game->Update(deltaTime);
        game->Render();

        glfwSwapBuffers(window);
        glfwPollEvents();

        previousTime = currentTime;
    }

    if (game)
    {
        delete game;
        game = nullptr;
    }

    glfwTerminate();

    return 0;
}