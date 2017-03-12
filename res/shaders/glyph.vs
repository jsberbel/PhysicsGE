#version 420

layout(location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>

uniform mat4 projection;

out vec2 ex_TexCoord;

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    ex_TexCoord = vertex.zw;
}  