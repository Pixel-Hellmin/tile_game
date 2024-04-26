#version 330 core

out vec4 FragColor;

uniform vec4 ourColor;

in vec3 vertexColor;

void main()
{
    FragColor = vec4(vertexColor, 1.0);
}
