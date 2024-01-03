#version 450

layout (binding = 0) uniform ubuffer {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec2 in_uv;

layout (location = 0) out vec3 v_pos;
layout (location = 1) out vec3 v_color;
layout (location = 2) out vec2 v_uv;

void main() {
	v_pos = in_pos;
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_pos, 1.0);
	v_color = in_color;
	v_uv = in_uv;
}