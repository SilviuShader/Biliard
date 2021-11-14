#pragma once

#include <string>
#include <glm/glm.hpp>

class Shader
{
private:

    static const int SHADER_COMPILE_LOG_LENGTH = 512;

public:

    Shader(const std::string, const std::string, const std::string = "", const std::string = "");

    void Use();

    void SetBool(const std::string&, bool)             const;
    void SetInt(const std::string&, int)               const;
    void SetFloat(const std::string&, float)           const;
    void SetVec2(const std::string&, const glm::vec2&) const;
    void SetVec3(const std::string&, const glm::vec3&) const;
    void SetVec4(const std::string&, const glm::vec4&) const;
    void SetMatrix4(const std::string&, glm::mat4&)    const;

private:

    std::string ReadFile(const std::string);

private:

    int m_programId;
};