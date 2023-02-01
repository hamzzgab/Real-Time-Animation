#version 330 core
layout (location = 0) in vec4 vertex;
out vec2 TexCoords;

uniform mat4 projection;

// [3] OpenGL. (2014, June 1). In Learn OpenGL. https://learnopengl.com/Getting-started/OpenGL

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
