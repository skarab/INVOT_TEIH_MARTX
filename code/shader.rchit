#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

struct hitPayload
{
	float t;
	vec4 color;
};

hitAttributeEXT vec4 hitColor;

layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{
	prd.t = gl_HitTEXT;
    prd.color = hitColor;
}
