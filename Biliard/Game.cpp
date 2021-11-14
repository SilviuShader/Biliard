#include "glad/glad.h"

#include "Game.h"

#include <iostream>
#include <utility>
#include <glm/gtc/matrix_transform.hpp>

#include "Constants.h"

using namespace std;
using namespace glm;

Game::PlayerDetails::PlayerDetails() :
    Score(0),
    Dead(false),
    AllowedBalls(vector<Ball*>()),
    FinishedBalls(false)
{
}

Game::Game(float windowWidth, float windowHeight) :
    m_windowWidth(windowWidth),
    m_windowHeight(windowHeight),
    m_mousePressed(false),
    m_whiteBall(nullptr),
    m_mousePosition(vec2(0.0f, 0.0f)),
    m_gameState(Game::GameState::Playing),
    m_currentPlayer(Game::Players::Player1)
{
    m_playerDetails[(int)Players::Player1] = PlayerDetails();
    m_playerDetails[(int)Players::Player2] = PlayerDetails();
    cout << "Este randul jucatorului " << (m_currentPlayer + 1) << "." << endl;

    m_tableShader = new Shader("Table.vert", "Table.frag");
    m_colorShader = new Shader("Ball.vert", "Ball.frag");

    CreateTableBuffers();
    CreateBallBuffers();
    CreateLineBuffers();

    OnResize(windowWidth, windowHeight);

    CreateBalls();
    CreateHoles();
}

Game::~Game()
{
    for (auto& hole : m_holes)
    {
        if (hole)
        {
            delete hole;
            hole = nullptr;
        }
    }
    m_holes.clear();

    for (auto& ball : m_balls)
    {
        if (ball)
        {
            delete ball;
            ball = nullptr;
        }
    }
    m_balls.clear();

    FreeLineBuffers();
    FreeBallBuffers();
    FreeTableBuffers();

    if (m_tableShader)
    {
        delete m_tableShader;
        m_tableShader = nullptr;
    }
}

void Game::OnResize(float windowWidth, float windowHeight)
{
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;

    float targetAspectRatio = (float)Constants::GAME_WIDTH / (float)Constants::GAME_HEIGHT;
    float screenAspectRatio = windowWidth / windowHeight;

    float targetToScreenRatio = 1.0f;

    float newWidth = Constants::GAME_WIDTH;
    float newHeight = Constants::GAME_HEIGHT;

    if (screenAspectRatio >= targetAspectRatio)
        newWidth = screenAspectRatio * Constants::GAME_HEIGHT;
    else
        newHeight = Constants::GAME_WIDTH / screenAspectRatio;
    
    m_projectionMatrix = scale(mat4(1.0), vec3(2.0f / newWidth, 2.0f / newHeight, 1.0f));
    m_projectionMatrix = m_projectionMatrix * translate(mat4(1.0f), -vec3((float)newWidth / 2.0f, (float)newHeight / 2.0f, 0.0f));
    m_projectionMatrix = m_projectionMatrix * translate(mat4(1.0f), vec3((newWidth - Constants::GAME_WIDTH) * 0.5f, (newHeight - Constants::GAME_HEIGHT) * 0.5f, 0.0f));
}

void Game::OnMouseMoved(float mouseX, float mouseY)
{
    // face ca 0 pe y sa fie in josul ferestrei, iar m_windowHeight sa fie in susul ferestre.
    mat4 invertY = scale(mat4(1.0f), vec3(1.0f, -1.0f, 1.0f));
    mat4 translateY = translate(mat4(1.0f), vec3(0.0f, m_windowHeight, 0.0f));

    // trece coordonatele din [0, m_width]x[0, m_height] in [0,1]x[0,1]
    mat4 normalizeToOGL = scale(mat4(1.0f), vec3(1.0f / m_windowWidth, 1.0f / m_windowHeight, 1.0f));

    // TODO: functie speciala pt codul asta, ca se repeta
    float newWidth = Constants::GAME_WIDTH;
    float newHeight = Constants::GAME_HEIGHT;
    {
        float targetAspectRatio = (float)Constants::GAME_WIDTH / (float)Constants::GAME_HEIGHT;
        float screenAspectRatio = m_windowWidth / m_windowHeight;

        float targetToScreenRatio = 1.0f;

        if (screenAspectRatio >= targetAspectRatio)
            newWidth = screenAspectRatio * Constants::GAME_HEIGHT;
        else
            newHeight = Constants::GAME_WIDTH / screenAspectRatio;
    }

    mat4 toGameCoords = scale(mat4(1.0f), vec3(newWidth, newHeight, 1.0f));

    mat4 translateRemaining = translate(mat4(1.0f), vec3(-(newWidth - Constants::GAME_WIDTH) * 0.5f, -(newHeight - Constants::GAME_HEIGHT) * 0.5f, 0.0f));

    mat4 transformMatrix = translateRemaining * toGameCoords * normalizeToOGL * translateY * invertY;

    vec4 mousePos = vec4(mouseX, mouseY, 0.0f, 1.0f);

    vec4 transformedMouse = transformMatrix * mousePos;

    vec2 newMousePosition = vec2(transformedMouse.x, transformedMouse.y);

    if (newMousePosition.x >= 0 && newMousePosition.x <= Constants::GAME_WIDTH &&
        newMousePosition.y >= 0 && newMousePosition.y <= Constants::GAME_HEIGHT)
        m_mousePosition = newMousePosition;
}

void Game::ProcessInput(GLFWwindow* window)
{
    bool prevMousePressed = m_mousePressed;
    m_mousePressed = glfwGetMouseButton(window, 0) == GLFW_PRESS;

    if (prevMousePressed && !m_mousePressed)
        OnMouseReleased();
}

void Game::Update(float deltaTime)
{
    if (m_gameState == GameState::Finished)
        return;

    for (auto& ball : m_balls)
        ball->Update(deltaTime, m_balls, m_holes);

    int badIndex = -1;
    do
    {
        badIndex = -1;

        for (int i = 0; i < m_balls.size(); i++)
        {
            if (!m_balls[i]->OnBoard())
            {
                badIndex = i;
                bool badBall = true;
                for (auto& allowedBall : m_playerDetails[m_currentPlayer].AllowedBalls)
                {
                    if (m_balls[i] == allowedBall)
                    {
                        badBall = false;
                        cout << "Jucatorul " << (m_currentPlayer + 1) << " a bagat in gaura bila." << endl;
                    }
                }
                if (badBall && m_balls[i]->GetBallType() != Ball::BallType::Black)
                {
                    cout << "Jucatorul " << (m_currentPlayer + 1) << " a bagat in gaura bila care apartine celuilalt jucator." << endl;
                }
            }
        }

        if (badIndex != -1)
        {
            if (m_balls[badIndex])
            {
                delete m_balls[badIndex];
                m_balls[badIndex] = nullptr;
            }
            m_balls[badIndex] = m_balls[m_balls.size() - 1];
            m_balls.pop_back();
        }

    } while (badIndex != -1);

    if (!m_playerDetails[m_currentPlayer].FinishedBalls)
    {
        bool finishedBalls = true;
        for (auto& allowedBall : m_playerDetails[m_currentPlayer].AllowedBalls)
        {
            for (auto& ball : m_balls)
            {
                if (allowedBall == ball)
                    finishedBalls = false;
            }
        }
        m_playerDetails[m_currentPlayer].FinishedBalls = finishedBalls;
        if (finishedBalls)
        {
            cout << "Jucatorul " << (m_currentPlayer + 1) << " si-a terminat bilele." << endl;
        }
    }

    bool foundBlack = false;
    for (auto& ball : m_balls)
    {
        if (ball->GetBallType() == Ball::BallType::Black)
        {
            foundBlack = true;
            break;
        }
    }

    if (!foundBlack)
    {
        if (m_playerDetails[m_currentPlayer].FinishedBalls)
            m_playerDetails[(int)(m_currentPlayer + 1) % 2].Dead = true;
        else
            m_playerDetails[m_currentPlayer].Dead = true;
        
        cout << "Jucatorul 1 a " << (m_playerDetails[Players::Player1].Dead ? "pierdut" : "castigat") << "." << endl;
        cout << "Jucatorul 2 a " << (m_playerDetails[Players::Player2].Dead ? "pierdut" : "castigat") << "." << endl;

        m_gameState = GameState::Finished;
        return;
    }

    switch (m_gameState)
    {
    case GameState::Waiting:
        {
            bool allStopped = true;
            for (auto& ball : m_balls)
            {
                if (!ball->IsStopped())
                {
                    allStopped = false;
                    break;
                }
            }
            if (allStopped)
            {
                m_gameState = GameState::Playing;
                m_currentPlayer = (Players)(((int)m_currentPlayer + 1) % 2);
                cout << "Este randul jucatorului " << (m_currentPlayer + 1) << "." << endl;
            }
        }
        break;
    case GameState::Playing:
        break;
    }
}

void Game::Render()
{
    mat4 tableModel = scale(mat4(1.0f), vec3(Constants::GAME_WIDTH * 0.5f, Constants::GAME_HEIGHT * 0.5f, 1.0f));
    tableModel = translate(mat4(1.0f), vec3(Constants::GAME_WIDTH / 2.0f, Constants::GAME_HEIGHT / 2.0f, 0.0f)) * tableModel;
    
    m_tableShader->Use();
    m_tableShader->SetMatrix4("Projection", m_projectionMatrix);
    m_tableShader->SetMatrix4("Model", tableModel);

    glBindVertexArray(m_tableVao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_tableEbo);
    glDrawElements(GL_TRIANGLES, TABLE_INDICES_COUNT, GL_UNSIGNED_INT, 0);

    for (auto& hole : m_holes)
    {
        mat4 holeModel = scale(mat4(1.0f), vec3(HOLE_RADIUS, HOLE_RADIUS, 1.0f));
        holeModel = translate(mat4(1.0f), vec3(hole->GetPosition().x, hole->GetPosition().y, 0.0f)) * holeModel;

        m_colorShader->Use();
        m_colorShader->SetVec3("Color", vec3(0.0f, 0.0f, 0.0f));
        m_colorShader->SetMatrix4("Projection", m_projectionMatrix);
        m_colorShader->SetMatrix4("Model", holeModel);

        glBindVertexArray(m_ballVao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, BALL_OUTSIDE_VERTICES_COUNT + 2);
    }

    if (m_mousePressed && m_gameState == GameState::Playing)
        RenderHelperLines();

    for (auto& ball : m_balls)
    {
        mat4 ballModel = scale(mat4(1.0f), vec3(Ball::BALL_RADIUS, Ball::BALL_RADIUS, 1.0f));
        ballModel = translate(mat4(1.0f), vec3(ball->GetPosition().x, ball->GetPosition().y, 0.0f)) * ballModel;

        m_colorShader->Use();
        m_colorShader->SetVec3("Color", ball->GetColor());
        m_colorShader->SetMatrix4("Projection", m_projectionMatrix);
        m_colorShader->SetMatrix4("Model", ballModel);

        glBindVertexArray(m_ballVao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, BALL_OUTSIDE_VERTICES_COUNT + 2);

        if (!ball->IsSolid())
        {
            ballModel = scale(mat4(1.0f), vec3(Ball::BALL_RADIUS * 0.5f, Ball::BALL_RADIUS * 0.5f, 1.0f));
            ballModel = translate(mat4(1.0f), vec3(ball->GetPosition().x, ball->GetPosition().y, 0.0f)) * ballModel;

            m_colorShader->Use();
            m_colorShader->SetVec3("Color", vec3(1.0f, 1.0f, 1.0f));
            m_colorShader->SetMatrix4("Projection", m_projectionMatrix);
            m_colorShader->SetMatrix4("Model", ballModel);

            glBindVertexArray(m_ballVao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, BALL_OUTSIDE_VERTICES_COUNT + 2);
        }
    }
}

void Game::OnMouseReleased()
{
    if (m_gameState == GameState::Playing)
    {
        m_whiteBall->SetVelocity(m_whiteBall->GetPosition() - m_mousePosition);
        m_gameState = GameState::Waiting;
    }
}

void Game::CreateTableBuffers()
{
    TableVertex vertices[] =
    {
        vec3(1.0f,  1.0f, 0.0f), vec3(0.0f, 0.15f, 0.0f),
        vec3(1.0f, -1.0f, 0.0f), vec3(0.0f, 0.1f, 0.0f),
        vec3(-1.0f, -1.0f, 0.0f), vec3(0.0f, 0.1f, 0.0f),
        vec3(-1.0f,  1.0f, 0.0f), vec3(0.0f, 0.15f, 0.0f)
    };

    unsigned int indices[] =
    {
        0, 3, 1,
        1, 3, 2
    };

    int verticesCount = sizeof(vertices) / sizeof(TableVertex);

    glGenVertexArrays(1, &m_tableVao);

    glBindVertexArray(m_tableVao);

    glGenBuffers(1, &m_tableVbo);

    glBindBuffer(GL_ARRAY_BUFFER, m_tableVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TableVertex) * verticesCount, vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TableVertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TableVertex), (void*)sizeof(vec3));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &m_tableEbo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_tableEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * TABLE_INDICES_COUNT, indices, GL_STATIC_DRAW);
}

void Game::FreeTableBuffers()
{
    glBindVertexArray(m_tableVao);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &m_tableVbo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &m_tableEbo);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &m_tableVao);
}

void Game::CreateBallBuffers()
{
    constexpr int totalVerticesCount = BALL_OUTSIDE_VERTICES_COUNT + 2;
    vec3 vertices[totalVerticesCount];

    int index = 0;
    vertices[index++] = vec3(0.0f, 0.0f, 0.0f);
    constexpr float angleIncrement = two_pi<float>() / (float)BALL_OUTSIDE_VERTICES_COUNT;
    float angle = 0.0f;

    while(index < totalVerticesCount - 1)
    {
        vertices[index++] = vec3(cosf(angle), sinf(angle), 0.0f);
        angle += angleIncrement;
    }
    vertices[index++] = vertices[1];

    glGenVertexArrays(1, &m_ballVao);

    glBindVertexArray(m_ballVao);

    glGenBuffers(1, &m_ballVbo);

    glBindBuffer(GL_ARRAY_BUFFER, m_ballVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * totalVerticesCount, vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);
}

void Game::FreeBallBuffers()
{
    glBindVertexArray(m_ballVao);

    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &m_ballVbo);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &m_ballVao);
}

void Game::CreateLineBuffers()
{
    vec3 vertices[] =
    {
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f)
    };

    int verticesCount = sizeof(vertices) / sizeof(vec3);

    glGenVertexArrays(1, &m_lineVao);

    glBindVertexArray(m_lineVao);

    glGenBuffers(1, &m_lineVbo);

    glBindBuffer(GL_ARRAY_BUFFER, m_lineVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * verticesCount, vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);
}

void Game::FreeLineBuffers()
{
    glBindVertexArray(m_lineVao);

    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &m_lineVbo);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &m_lineVao);
}

void Game::CreateBalls()
{
    m_whiteBall = new Ball(vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), true, Ball::BallType::White);
    m_balls.push_back(m_whiteBall);

    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), true, Ball::BallType::Black));

    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(1.0f, 0.956f, 0.156f), true));
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.215f, 0.333f, 0.921f), true));
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.776f, 0.145f, 0.756f), true));
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(1.0f, 0.439f, 0.062f), true));
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.976f, 0.050f, 0.058f), true));
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.050f, 0.811f, 0.603f), true));
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.713f, 0.121f, 0.156f), true));

    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.0f, 0.654f, 0.384f), false));
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.843f, 0.274f, 0.050f), false));
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.807f, 0.117f, 0.780f), false)); 
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.156f, 0.239f, 0.729f), false));
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.992f, 0.823f, 0.168f), false));
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.560f, 0.090f, 0.125f), false));
    m_balls.push_back(new Ball(vec2(0.0f, 0.0f), vec3(0.933f, 0.070f, 0.078f), false));

    for (int i = 1; i < m_balls.size(); i++)
    {
        int otherIndex = (rand() % (m_balls.size() - 1)) + 1;
        swap(m_balls[i], m_balls[otherIndex]);
    }

    for (auto& ball : m_balls)
    {
        if (ball->GetBallType() == Ball::BallType::Normal)
        {
            if (ball->IsSolid())
                m_playerDetails[Players::Player1].AllowedBalls.push_back(ball);
            else
                m_playerDetails[Players::Player2].AllowedBalls.push_back(ball);
        }
    }

    int columnCount = 0;
    int totalPerColumn = 1;

    float xPosition = (Constants::GAME_WIDTH / 2.0f) + NORMAL_BALLS_OFFSET;

    for (int i = 1; i < m_balls.size(); i++)
    {
        float y = Constants::GAME_HEIGHT / 2.0f;
        float totalDist = NORMAL_BALLS_DIST_BETWEEN * (totalPerColumn - 1);
        y = y - (totalDist / 2.0f);
        if (totalPerColumn >= 2)
        {
            y = y + (totalDist) * (float(columnCount) / float(totalPerColumn - 1));
        }
        m_balls[i]->SetPosition(vec2(xPosition, y));
        columnCount++;

        if (columnCount >= totalPerColumn)
        {
            columnCount = 0;
            totalPerColumn++;
            xPosition += NORMAL_BALLS_DIST_BETWEEN;
        }
    }
}

void Game::CreateHoles()
{
    // gaurile de jos
    m_holes.push_back(new Hole(vec2(HOLE_BIAS, HOLE_BIAS)));
    m_holes.push_back(new Hole(vec2(Constants::GAME_WIDTH / 2.0f, HOLE_BIAS)));
    m_holes.push_back(new Hole(vec2(Constants::GAME_WIDTH - HOLE_BIAS, HOLE_BIAS)));

    // gaurile de sus
    m_holes.push_back(new Hole(vec2(HOLE_BIAS, Constants::GAME_HEIGHT - HOLE_BIAS)));
    m_holes.push_back(new Hole(vec2(Constants::GAME_WIDTH / 2.0f, Constants::GAME_HEIGHT - HOLE_BIAS)));
    m_holes.push_back(new Hole(vec2(Constants::GAME_WIDTH - HOLE_BIAS, Constants::GAME_HEIGHT - HOLE_BIAS)));
}

void Game::RenderHelperLines()
{
    glLineWidth(5.0f);
    mat4 lineModel = LineModelFromTo(m_whiteBall->GetPosition(), m_mousePosition);

    m_colorShader->Use();
    m_colorShader->SetVec3("Color", vec3(1.0f, 1.0f, 1.0f));
    m_colorShader->SetMatrix4("Projection", m_projectionMatrix);
    m_colorShader->SetMatrix4("Model", lineModel);

    glBindVertexArray(m_lineVao);
    glDrawArrays(GL_LINES, 0, 2);

    vec2 direction = normalize(m_whiteBall->GetPosition() - m_mousePosition);
    vec2 ballPosition = m_whiteBall->GetPosition();
    RayIntersection whiteBallHit = GetRayIntersection(ballPosition, direction, m_whiteBall);

    mat4 lineModel2 = LineModelFromTo(m_whiteBall->GetPosition(), whiteBallHit.Point);

    m_colorShader->Use();
    m_colorShader->SetVec3("Color", vec3(1.0f, 1.0f, 1.0f));
    m_colorShader->SetMatrix4("Projection", m_projectionMatrix);
    m_colorShader->SetMatrix4("Model", lineModel2);

    glBindVertexArray(m_lineVao);
    glDrawArrays(GL_LINES, 0, 2);

    vec2 beginLinePos = whiteBallHit.Point;
    vec2 newDirection = reflect(direction, whiteBallHit.Normal);
    Ball* excludeBall = nullptr;
    if (whiteBallHit.Ball)
    {
        vec2 futureWhiteBallPos = whiteBallHit.Point - normalize(direction) * Ball::BALL_RADIUS;
        vec2 fromOther = futureWhiteBallPos - whiteBallHit.Ball->GetPosition();

        beginLinePos = whiteBallHit.Ball->GetPosition();
        newDirection = -normalize(fromOther);
        excludeBall = whiteBallHit.Ball;
    }
    RayIntersection nextIntersection = GetRayIntersection(beginLinePos, newDirection, excludeBall);

    mat4 lineModel3 = LineModelFromTo(beginLinePos, nextIntersection.Point);

    m_colorShader->Use();
    m_colorShader->SetVec3("Color", vec3(1.0f, 1.0f, 1.0f));
    m_colorShader->SetMatrix4("Projection", m_projectionMatrix);
    m_colorShader->SetMatrix4("Model", lineModel3);

    glBindVertexArray(m_lineVao);
    glDrawArrays(GL_LINES, 0, 2);
}

Game::RayIntersection Game::GetRayIntersection(vec2 startPosition, vec2 direction, Ball* exceptionBall)
{
    vec2 endPosition = startPosition + direction * 1000.0f;
    vec2 closestIntersect = endPosition;
    vec2 normal = vec2(0.0f, 1.0f);

    RayIntersection result;

    result.Ball = nullptr;

    vec2 wallIntersection;

    if (VerticalIntersect(startPosition, direction, 0.0f, wallIntersection))
    {
        if (length(wallIntersection - startPosition) < length(closestIntersect - startPosition))
        {
            closestIntersect = wallIntersection;
            normal = vec2(1.0f, 0.0f);
        }
    }

    if (VerticalIntersect(startPosition, direction, Constants::GAME_WIDTH, wallIntersection))
    {
        if (length(wallIntersection - startPosition) < length(closestIntersect - startPosition))
        {
            closestIntersect = wallIntersection;
            normal = vec2(-1.0f, 0.0f);
        }
    }

    if (Horizontalntersect(startPosition, direction, 0.0f, wallIntersection))
    {
        if (length(wallIntersection - startPosition) < length(closestIntersect - startPosition))
        {
            closestIntersect = wallIntersection;
            normal = vec2(0.0f, 1.0f);
        }
    }

    if (Horizontalntersect(startPosition, direction, Constants::GAME_HEIGHT, wallIntersection))
    {
        if (length(wallIntersection - startPosition) < length(closestIntersect - startPosition))
        {
            closestIntersect = wallIntersection;
            normal = vec2(0.0f, -1.0f);
        }
    }

    for (auto& ball : m_balls)
    {
        if (ball != exceptionBall)
        {
            vec2 intersection1;
            vec2 intersection2;
            vec2 ballPosition = ball->GetPosition();
            int intersectionCount = FindLineCircleIntersections(ballPosition.x, ballPosition.y, Ball::BALL_RADIUS, startPosition, endPosition, intersection1, intersection2);

            if (intersectionCount >= 1)
            {
                if (length(intersection1 - startPosition) < length(closestIntersect - startPosition) &&
                    dot(direction, intersection1 - startPosition) > 0.0f)
                {
                    closestIntersect = intersection1;
                    normal = normalize(intersection1 - ballPosition);
                    result.Ball = ball;
                }
            }

            if (intersectionCount >= 2)
            {
                if (length(intersection2 - startPosition) < length(closestIntersect - startPosition) &&
                    dot(direction, intersection2 - startPosition) > 0.0f)
                {
                    closestIntersect = intersection2;
                    normal = normalize(intersection2 - ballPosition);
                    result.Ball = ball;
                }
            }
        }
    }

    result.Point = closestIntersect;
    result.Normal = normal;

    return result;
}

//http://csharphelper.com/blog/2014/09/determine-where-a-line-intersects-a-circle-in-c/
int Game::FindLineCircleIntersections(
    float cx, float cy, float radius,
    glm::vec2 point1, glm::vec2 point2,
    vec2& intersection1, vec2& intersection2)
{
    float dx, dy, A, B, C, det, t;

    dx = point2.x - point1.x;
    dy = point2.y - point1.y;

    A = dx * dx + dy * dy;
    B = 2 * (dx * (point1.x - cx) + dy * (point1.y - cy));
    C = (point1.x - cx) * (point1.x - cx) +
        (point1.y - cy) * (point1.y - cy) -
        radius * radius;

    det = B * B - 4 * A * C;
    if ((A <= 0.0000001) || (det < 0))
    {
        // No real solutions.
        intersection1 = vec2(0.0f, 0.0f);
        intersection2 = vec2(0.0f, 0.0f);
        return 0;
    }
    else if (det == 0)
    {
        // One solution.
        t = -B / (2 * A);
        intersection1 = vec2(point1.x + t * dx, point1.y + t * dy);
        intersection2 = vec2(0.0f, 0.0f);
        return 1;
    }
    else
    {
        // Two solutions.
        t = (float)((-B + sqrtf(det)) / (2 * A));
        intersection1 = vec2(point1.x + t * dx, point1.y + t * dy);
        t = (float)((-B - sqrtf(det)) / (2 * A));
        intersection2 = vec2(point1.x + t * dx, point1.y + t * dy);
        return 2;
    }
}

bool Game::VerticalIntersect(vec2 startPos, vec2 direction, float x1, vec2& intersection)
{
    intersection = vec2(0.0f, 0.0f);
    if (abs(direction.x) < 0.01f)
    {
        return false;
    }

    float t = (x1 - startPos.x) / direction.x;
    float y = startPos.y + t * direction.y;

    vec2 intersectPoint = vec2(x1, y);

    if (dot(intersectPoint - startPos, direction) > 0.0f)
    {
        intersection = intersectPoint;
        return true;
    }
    return false;
}

bool Game::Horizontalntersect(vec2 startPos, vec2 direction, float y1, vec2& intersection)
{
    intersection = vec2(0.0f, 0.0f);
    if (abs(direction.y) < 0.01f)
    {
        return false;
    }

    float t = (y1 - startPos.y) / direction.y;
    float x = startPos.x + t * direction.x;

    vec2 intersectPoint = vec2(x, y1);

    if (dot(intersectPoint - startPos, direction) > 0.0f)
    {
        intersection = intersectPoint;
        return true;
    }

    return false;
}

mat4 Game::LineModelFromTo(vec2 from, vec2 to)
{
    vec2 direction = to - from;
    float magnitude = length(direction);

    float angle = -atan2(direction.x, direction.y);
    return translate(mat4(1.0f), vec3(from.x, from.y, 0.0f)) * rotate(mat4(1.0f), angle, vec3(0.0f, 0.0f, 1.0f)) * scale(mat4(1.0f), vec3(magnitude, magnitude, 1.0f));
}