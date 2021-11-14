uniform vec3 Color;

out vec4 FSOutColor;

void main()
{
    FSOutColor = vec4(Color, 1.0);
}