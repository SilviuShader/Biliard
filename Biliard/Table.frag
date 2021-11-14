in vec3 FSInputColor;

out vec4 FSOutColor;

void main()
{
    FSOutColor = vec4(FSInputColor, 1.0);
}