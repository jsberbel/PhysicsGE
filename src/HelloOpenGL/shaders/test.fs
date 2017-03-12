#version 420

in  vec4 ex_Color;

layout(location = 0) out vec4 out_Color;

void main(void) {
	out_Color = ex_Color;
}