#include <fstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

#include "glad/glad.h"
#include "Shader.h"

using namespace std;
using namespace glm;

Shader::Shader(const string vertexPath, const string fragmentPath, 
    const string tessControlPath, const string tessEvaluationPath)
{
    unsigned int vertex, fragment;
    unsigned int tessControl, tessEvaluation;
    bool addedTessControl = false;
    bool addedTessEvaluation = false;
    int success;
    char infoLog[SHADER_COMPILE_LOG_LENGTH];

    string vsString = ReadFile(vertexPath);
    string fsString = ReadFile(fragmentPath);

    const char* vertexShaderSource = vsString.c_str();
    const char* fragmentShaderSource = fsString.c_str();

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, SHADER_COMPILE_LOG_LENGTH, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    if (tessControlPath.size() > 0)
    {
        string tcsString = ReadFile(tessControlPath);
        const char* tessControlShaderSource = tcsString.c_str();

        tessControl = glCreateShader(GL_TESS_CONTROL_SHADER);
        glShaderSource(tessControl, 1, &tessControlShaderSource, NULL);
        glCompileShader(tessControl);

        glGetShaderiv(tessControl, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(tessControl, SHADER_COMPILE_LOG_LENGTH, NULL, infoLog);
            cout << "ERROR::SHADER::TESS_CONTROL::COMPILATION_FAILED\n" << infoLog << endl;
        }

        addedTessControl = true;
    }

    if (tessEvaluationPath.size() > 0)
    {
        string tesString = ReadFile(tessEvaluationPath);
        const char* tessEvaluationShaderSource = tesString.c_str();

        tessEvaluation = glCreateShader(GL_TESS_EVALUATION_SHADER);
        glShaderSource(tessEvaluation, 1, &tessEvaluationShaderSource, NULL);
        glCompileShader(tessEvaluation);

        glGetShaderiv(tessEvaluation, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(tessEvaluation, SHADER_COMPILE_LOG_LENGTH, NULL, infoLog);
            cout << "ERROR::SHADER::TESS_EVALUATION::COMPILATION_FAILED\n" << infoLog << endl;
        }

        addedTessEvaluation = true;
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, SHADER_COMPILE_LOG_LENGTH, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    m_programId = glCreateProgram();
    glAttachShader(m_programId, vertex);
    
    if (addedTessControl)
        glAttachShader(m_programId, tessControl);
    if (addedTessEvaluation)
        glAttachShader(m_programId, tessEvaluation);

    glAttachShader(m_programId, fragment);
    glLinkProgram(m_programId);

    glGetProgramiv(m_programId, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(m_programId, SHADER_COMPILE_LOG_LENGTH, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::Use()
{
    glUseProgram(m_programId);
}

void Shader::SetBool(const string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(m_programId, name.c_str()), (int)value);
}

void Shader::SetInt(const string& name, int value) const
{
    glUniform1i(glGetUniformLocation(m_programId, name.c_str()), value);
}

void Shader::SetFloat(const string& name, float value) const
{
    glUniform1f(glGetUniformLocation(m_programId, name.c_str()), value);
}

void Shader::SetVec2(const string& name, const vec2& value) const
{
    glUniform2f(glGetUniformLocation(m_programId, name.c_str()), value.x, value.y);
}

void Shader::SetVec3(const string& name, const vec3& value) const
{
    glUniform3f(glGetUniformLocation(m_programId, name.c_str()), value.x, value.y, value.z);
}

void Shader::SetVec4(const string& name, const vec4& value) const
{
    glUniform4f(glGetUniformLocation(m_programId, name.c_str()), value.x, value.y, value.z, value.w);
}

void Shader::SetMatrix4(const string& name, mat4& value) const
{
    float* ptr = value_ptr(value);
    glUniformMatrix4fv(glGetUniformLocation(m_programId, name.c_str()), 1, GL_FALSE, ptr);
}

string Shader::ReadFile(const string filename)
{
    try
    {
        ifstream t(filename);
        string str((istreambuf_iterator<char>(t)),
            istreambuf_iterator<char>());

        return str;
    }
    catch (ifstream::failure e)
    {
        cout << "ERROR::FILE::NOT_SUCCESFULLY_READ" << endl;
    }

    return "";
}
