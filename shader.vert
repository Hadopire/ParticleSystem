#version 410

layout (location=0) in vec2 position;

void main() {
	gl_Position = vec4(position.xy, 0.5, 1.0);
}