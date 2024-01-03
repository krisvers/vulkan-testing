#version 450

layout (binding = 1) uniform sampler2D unif_sampler;

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_color;
layout (location = 2) in vec2 v_uv;

layout (location = 0) out vec4 out_color;

void main() {
	//out_color = vec4(v_uv, 0.0f, 1.0f);
	//out_color = vec4((v_pos + 1) / 2, 1.0f);
	//out_color = vec4(texture(unif_sampler, v_uv).xyz, 1.0f);
}