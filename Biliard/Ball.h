#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "Hole.h"

class Ball
{
public:

    enum class BallType
    {
        Normal,
        White,
        Black
    };

public:
           const float WHITE_BALL_OFFSET      = 200.0f;
           const float BALL_MASS              = 10.0f;
           const float WALL_MASS              = 100.0f;
           const float RESTITUTION            = 0.98f;
           const float VELOCITY_BIAS          = 0.01f;
           const float FRICTION_MULTIPLIER    = 30.0f;
           const float VELOCITY_MULTIPLIER    = 5.0f;
           const float DISTANCE_TO_ENTER_HOLE = 25.0f;
    static const float BALL_RADIUS;

public:

    Ball(glm::vec2, glm::vec3, bool, BallType = BallType::Normal);

    void      Update(float, std::vector<Ball*>&, std::vector<Hole*>&);

    void      SetPosition(glm::vec2);
    void      SetVelocity(glm::vec2);

    glm::vec2 GetPosition() const;
    glm::vec3 GetColor()    const;
    BallType  GetBallType() const;

    bool      IsSolid()     const;
    bool      IsStopped()   const;
    bool      OnBoard()     const;

private:

    void ResetWhite();
    void ResetBlack();

    void ResolveColission(Ball*);
    void ResolveColission(glm::vec2);
    void UpdateFriction(float deltaTime);

private:

    glm::vec2 m_position;
    glm::vec2 m_velocity;
    glm::vec3 m_color;

    bool      m_solid;
    BallType  m_ballType;

    bool      m_stopped;
    bool      m_onBoard;
};