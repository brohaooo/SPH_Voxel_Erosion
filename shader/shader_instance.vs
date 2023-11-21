#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 translation;
layout (location = 2) in vec3 color;


out vec3 instanceColor;

uniform mat4 scale;
uniform mat4 view;
uniform mat4 projection;

vec4 translate(vec4 pos, vec3 translation) {
    return vec4(pos.x+translation.x, pos.y+translation.y, pos.z+translation.z,1.0f);
}


void main()
{
    gl_Position = projection * view * translate(scale * vec4(aPos, 1.0f),translation);
    //gl_Position = projection * view * scale * vec4(aPos, 1.0f);

    instanceColor = color;

}
