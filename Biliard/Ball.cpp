#include "Ball.h"

#include "Constants.h"

using namespace std;
using namespace glm;

const float Ball::BALL_RADIUS = 20.0f;

Ball::Ball(vec2 position, vec3 color, bool solid, BallType ballType) :
    m_position(position),
    m_velocity(vec2(0.0f, 0.0f)),
    m_color(color),
    m_solid(solid),
    m_ballType(ballType),
    m_stopped(true),
    m_onBoard(true)
{
    switch (m_ballType) 
    {
    case BallType::White:
        ResetWhite();
        break;
    case BallType::Black:
        ResetBlack();
        break;
    }
}

void Ball::Update(float deltaTime, vector<Ball*>& otherBalls, vector<Hole*>& holes)
{
    for (auto& otherBall : otherBalls)
    {
        if (otherBall == this)
            continue;

        vec2 dir = otherBall->m_position - m_position;
        if (length(dir) <= 2.0f * BALL_RADIUS)
            ResolveColission(otherBall);
    }

    if (m_position.x - BALL_RADIUS <= 0.0f)
        ResolveColission(vec2(0.0f, m_position.y));

    if (m_position.x + BALL_RADIUS >= Constants::GAME_WIDTH)
        ResolveColission(vec2(Constants::GAME_WIDTH, m_position.y));

    if (m_position.y - BALL_RADIUS <= 0.0f)
        ResolveColission(vec2(m_position.x, 0.0f));

    if (m_position.y + BALL_RADIUS >= Constants::GAME_HEIGHT)
        ResolveColission(vec2(m_position.x, Constants::GAME_HEIGHT));

    UpdateFriction(deltaTime);

    m_stopped = length(m_velocity) < VELOCITY_BIAS;

    m_position += m_velocity * deltaTime * VELOCITY_MULTIPLIER;

    for (auto& hole : holes)
    {
        vec2 dir = hole->GetPosition() - m_position;
        if (length(dir) < DISTANCE_TO_ENTER_HOLE)
        {
            switch (m_ballType)
            {
            case BallType::White:
                ResetWhite();
                break;
            case BallType::Black:
                m_onBoard = false;
                break;
            case BallType::Normal:
                m_onBoard = false;
                break;
            }
        }
    }
}

void Ball::SetPosition(vec2 position)
{
    m_position = position;
}

void Ball::SetVelocity(vec2 velocity)
{
    m_velocity = velocity;
}

vec2 Ball::GetPosition() const
{
    return m_position;
}

vec3 Ball::GetColor() const
{
    return m_color;
}

Ball::BallType Ball::GetBallType() const
{
    return m_ballType;
}

bool Ball::IsSolid() const
{
    return m_solid;
}

bool Ball::IsStopped() const
{
    return m_stopped;
}

bool Ball::OnBoard() const
{
    return m_onBoard;
}

void Ball::ResetWhite()
{
    m_color = vec3(1.0f, 1.0f, 1.0f);
    m_position = vec2(Constants::GAME_WIDTH / 2.0f - WHITE_BALL_OFFSET, Constants::GAME_HEIGHT / 2.0f);
    m_velocity = vec2(0.0f, 0.0f);
}

void Ball::ResetBlack()
{
    m_color = vec3(0.0f, 0.0f, 0.0f);
}

// Metoda bazata pe: https://stackoverflow.com/questions/345838/ball-to-ball-collision-detection-and-handling
void Ball::ResolveColission(Ball* otherBall)
{
    vec2 fromOther = m_position - otherBall->m_position;
    float dist = length(fromOther);

    vec2 minTranslation = fromOther * (((2.0f * BALL_RADIUS) - dist) / dist);

    m_position            += minTranslation * 0.5f;
    otherBall->m_position += minTranslation * -0.5f;

    vec2 v = m_velocity - otherBall->m_velocity;
    float vn = dot(v, normalize(minTranslation));

    if (vn > 0.0f)
        return;

    float i = (-(1.0f + RESTITUTION) * vn) / (2.0f * (1.0f / BALL_MASS));
    vec2 impulse = normalize(minTranslation) * i;

    m_velocity = m_velocity + impulse * (1.0f / BALL_MASS);
    otherBall->m_velocity = otherBall->m_velocity - impulse * (1.0f / BALL_MASS);
}

// Functie similara cu cea pentru cerc vs cerc, doar a ca fost adaptata sa mearga pentru pereti.
void Ball::ResolveColission(vec2 colissionPoint)
{
    vec2 fromOther = m_position - colissionPoint;
    float dist = length(fromOther);

    vec2 minTranslation = fromOther * (((BALL_RADIUS) - dist) / dist);

    float im1 = 1.0f / BALL_MASS;
    float im2 = 1.0f / WALL_MASS;

    m_position += minTranslation;

    vec2 v = m_velocity;
    float vn = dot(v, normalize(minTranslation));

    if (vn > 0.0f)
        return;

    float i = (-(1.0f + RESTITUTION) * vn) / (im1 + im2);
    vec2 impulse = normalize(minTranslation) * i;

    m_velocity = m_velocity + impulse * im1;
}

void Ball::UpdateFriction(float deltaTime)
{
    vec2 direction = m_velocity;

    if (length(direction) < VELOCITY_BIAS)
    {
        m_velocity = vec2(0.0f, 0.0f);
        return;
    }

    vec2 friction = -normalize(direction) * FRICTION_MULTIPLIER;
    vec2 prevVelocity = m_velocity;
    m_velocity += friction * deltaTime;

    if (dot(prevVelocity, m_velocity) <= 0.0f)
        m_velocity = vec2(0.0f, 0.0f);
}
