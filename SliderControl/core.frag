#version 330 core

//simple fragments without lighting, blending, perspective, etc.

in vec3 ourColor;
out vec4 color;

void main()
{
    color = vec4(ourColor, 1.0f);
}
