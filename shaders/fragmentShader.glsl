#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D Texture;

void main()
{
    vec4 color = texture(Texture, TexCoord);
    float on = color.r;
    FragColor = on * vec4(0.7, 0.8, 0.9, 1.0) + 0.1;
}