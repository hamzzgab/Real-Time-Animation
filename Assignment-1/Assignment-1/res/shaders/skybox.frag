#version 330 core
in vec3 TexCoords;
out vec4 color;

uniform samplerCube skybox;

void main()
{
//    color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    color = texture(skybox, TexCoords);
}
