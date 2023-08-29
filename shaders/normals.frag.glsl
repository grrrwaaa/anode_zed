#version 330
precision mediump float;
uniform sampler2D u_tex_normals;
uniform sampler2D u_tex_cloud;

in vec2 v_uv;
layout(location = 0) out vec4 frag_out0;

void main() {
    frag_out0 = vec4(1);
	frag_out0 = vec4(v_uv, 0., 1.);
    vec3 normal = texture(u_tex_normals, v_uv).xyz;
    vec3 cloud = texture(u_tex_cloud, v_uv).xyz;
    //float color = texture(u_tex_cloud, v_uv).w;

    vec3 snormal = (normal*0.5+0.5);
    frag_out0 = vec4(snormal, 1.);
    frag_out0 = vec4(-0.2*cloud.z*snormal, 1.);
}