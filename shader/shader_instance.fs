#version 330 core


in vec3 instanceColor;
out vec4 FragColor;

uniform bool is_black;

void main()
{
    if (is_black){
       FragColor = vec4(0,0,0,0);
    }
    else{
        FragColor = vec4(instanceColor,1.0f);
    }
}
