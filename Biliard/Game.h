#pragma once

#include <vector>
#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

#include "Shader.h"
#include "Ball.h"
#include "Hole.h"

class Game
{
private:

    enum GameState
    {
        Playing,
        Waiting,
        Finished
    };

    enum Players
    {
        Player1,
        Player2
    };

    struct TableVertex
    {
        glm::vec3 Position;
        glm::vec3 Color;
    };

    struct PlayerDetails
    {
    public:

        PlayerDetails();
    
    public:

        int                Score;
        bool               Dead;
        bool               FinishedBalls;
        std::vector<Ball*> AllowedBalls;
    };

    struct RayIntersection
    {
        glm::vec2 Point;
        glm::vec2 Normal;
        Ball*     Ball;
    };

private:

           const int   TABLE_INDICES_COUNT         = 6;
           const float NORMAL_BALLS_OFFSET         = 100.0f;
           const float NORMAL_BALLS_DIST_BETWEEN   = 50.0f;
           const float HOLE_BIAS                   = 15.0f;
           const float HOLE_RADIUS                 = 30.0f;

    static const int   BALL_OUTSIDE_VERTICES_COUNT = 20;

public:

    Game(float, float);
    ~Game();

    void OnResize(float, float);
    void OnMouseMoved(float, float);
    void ProcessInput(GLFWwindow*);

    void Update(float);
    void Render();

private:

    void            OnMouseReleased();

    void            CreateTableBuffers();
    void            FreeTableBuffers();

    void            CreateBallBuffers();
    void            FreeBallBuffers();

    void            CreateLineBuffers();
    void            FreeLineBuffers();

    void            CreateBalls();
    void            CreateHoles();

    void            RenderHelperLines();

    RayIntersection GetRayIntersection(glm::vec2, glm::vec2, Ball* = nullptr);
    int             FindLineCircleIntersections(float, float, float, glm::vec2, glm::vec2, glm::vec2&, glm::vec2&);
    bool            VerticalIntersect(glm::vec2, glm::vec2, float, glm::vec2&);
    bool            Horizontalntersect(glm::vec2, glm::vec2, float, glm::vec2&);
    glm::mat4       LineModelFromTo(glm::vec2, glm::vec2);

private:

    Shader*            m_tableShader;
    Shader*            m_colorShader;

    unsigned int       m_tableVbo;
    unsigned int       m_tableVao;
    unsigned int       m_tableEbo;

    unsigned int       m_ballVbo;
    unsigned int       m_ballVao;

    unsigned int       m_lineVbo;
    unsigned int       m_lineVao;
    
    std::vector<Ball*> m_balls;
    Ball*              m_whiteBall;
    std::vector<Hole*> m_holes;

    float              m_windowWidth;
    float              m_windowHeight;

    bool               m_mousePressed;
    glm::vec2          m_mousePosition;

    glm::mat4          m_projectionMatrix;

    GameState          m_gameState;
    Players            m_currentPlayer;
    PlayerDetails      m_playerDetails[2];
};