#version 330
precision mediump float;

// uniform sampler2D u_tex_normals;
// uniform sampler2D u_tex_cloud;
in vec2 v_uv;
in vec3 v_normal;
in vec4 v_color;

layout(location = 0) out vec4 frag_out0;

void main() {
    //frag_out0 = texture(u_tex_cloud, v_uv);
    frag_out0 = vec4(v_normal, 1.);
    // frag_out0 = vec4(v_uv, 0, 1);
    // frag_out0 = v_color;
    // frag_out0 = vec4(1.);
}