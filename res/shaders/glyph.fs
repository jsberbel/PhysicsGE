#version 420

in vec2 ex_TexCoord;

layout(location = 0) out vec4 out_Color; // write to the back buffer 0

uniform sampler2D text;
uniform vec3 textColor;

void main()
{    
    out_Color = vec4(textColor, texture(text, ex_TexCoord).r);
}  