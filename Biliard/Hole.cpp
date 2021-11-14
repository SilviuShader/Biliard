#include "Hole.h"

using namespace glm;

Hole::Hole(vec2 position) :
    m_position(position)
{
}

vec2 Hole::GetPosition() const
{
    return m_position;
}