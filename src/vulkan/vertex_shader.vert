#version 450

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_tex_coords;
layout (location = 2) in vec4 in_color;

layout (location = 0) out vec2 out_uv;
layout (location = 1) out vec4 out_color;

layout (binding = 0) uniform Transform_data
{
	mat4 model, view, projection;
} transform_data;

void main() 
{
	out_uv  = in_tex_coords.st;
	out_color = in_color;
	gl_Position = transform_data.projection * transform_data.view * transform_data.model * vec4(in_pos, 1.0f);
	gl_Position.y = -gl_Position.y;
}
