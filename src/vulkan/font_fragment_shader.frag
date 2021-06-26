#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec2 in_uv;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 out_frag_color;

layout (binding = 1) uniform sampler texture_sampler;
layout (binding = 2) uniform texture2D textures[200];

layout(push_constant) uniform Push_constants {
	int texture_index;
} push_constants;

void main() 
{
	out_frag_color = in_color * vec4(1.0, 1.0, 1.0, texture(sampler2D(textures[push_constants.texture_index], texture_sampler), in_uv).r);

}
