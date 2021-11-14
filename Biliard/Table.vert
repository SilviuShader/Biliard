#version 430 core
layout (location = 0) in vec3 VSInputPosition;
layout (location = 1) in vec3 VSInputColor;

uniform mat4 Projection;
uniform mat4 Model;

out vec3 FSInputColor;

void main()
{
    gl_Position = Projection * Model * vec4(VSInputPosition, 1.0);
    FSInputColor = VSInputColor;
}