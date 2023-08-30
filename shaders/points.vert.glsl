#version 330

uniform mat4 u_projmatrix;
uniform mat4 u_viewmatrix;
uniform mat4 u_modelmatrix;
uniform float u_pointsize;

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec4 a_color;
out vec2 v_uv;
out vec3 v_normal;
out vec4 v_color;
out vec3 v_pos;

void main() {
    vec4 viewpos = u_viewmatrix * u_modelmatrix * vec4(a_position.xyz, 1.);
    gl_Position = u_projmatrix * viewpos;

    float camDist = length(viewpos.xyz);
    gl_PointSize = u_pointsize/camDist;

    // rgba unpack from texture(u_tex_cloud, v_uv).w

	v_uv = a_texCoord;
    v_normal = mat3(u_modelmatrix) * a_normal;
    v_color = a_color.rgba; 
    v_pos = a_position.xyz;
}