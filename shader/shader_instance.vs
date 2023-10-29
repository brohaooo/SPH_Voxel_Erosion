#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 translation;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

vec4 translate(vec4 pos, vec3 translation) {
    return vec4(pos.x+translation.x, pos.y+translation.y, pos.z+translation.z,1.0f);
}


void main()
{
    gl_Position = projection * view * translate(model * vec4(aPos, 1.0f),translation);
    //gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
