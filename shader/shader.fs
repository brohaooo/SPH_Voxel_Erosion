#version 330 core
out vec4 FragColor;

// in vec2 TexCoord;

// uniform sampler2D texture1;
// uniform sampler2D texture2;
uniform vec4 color;

void main()
{
    // FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
    // FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    FragColor = color;
}
