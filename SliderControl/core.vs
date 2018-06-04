#version 330 core

//simple fragments without lighting, blending, perspective, etc.

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

out vec3 ourColor;
uniform mat4 transform;

void main ()
{
    gl_Position = transform * vec4(position, 1.0f);
    ourColor = color;
}
