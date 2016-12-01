#version 410

layout (location=0) in vec4 position;
layout (location=1) in vec4 inColor;

out vec4 color;

void main() {
	color = inColor;
	gl_Position = vec4(position.xyz, 1.0);
}
