#pragma once

#include <glm/glm.hpp>

class Hole
{
public:

    Hole(glm::vec2);

    glm::vec2 GetPosition() const;

private:

    glm::vec2 m_position;
};