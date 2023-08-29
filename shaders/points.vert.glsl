#version 330

uniform mat4 u_projmatrix;
uniform mat4 u_viewmatrix;
uniform mat4 u_modelmatrix;
uniform sampler2D u_tex_normals;
uniform sampler2D u_tex_cloud;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
out vec2 v_uv;
out vec3 v_normal;
out vec4 v_color;

void main() {
	v_uv = a_texCoord;

    gl_Position = u_projmatrix * u_viewmatrix * u_modelmatrix * vec4(a_position.xyz, 1.);

    //float camDist = length(viewpos.xyz);
    //gl_PointSize = pulse_size * u_pointsize/camDist;

    // rgba unpack from texture(u_tex_cloud, v_uv).w

    v_normal = a_normal;

    v_color = texture(u_tex_cloud, v_uv);
    v_normal = texture(u_tex_normals, v_uv).xyz;
}