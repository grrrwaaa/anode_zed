#version 330
precision mediump float;
uniform float u_showmode;
in vec2 v_uv;
in vec3 v_normal;
in vec3 v_pos;
in vec4 v_color;

layout(location = 0) out vec4 frag_out0;

void main() {
    float c = 1.-length(2.*abs(0.5-gl_PointCoord));

    frag_out0.a = 1.;
    if (u_showmode >= 1.) {
        frag_out0.rgb = v_color.rgb;
    } else {
        frag_out0.rgb = v_normal.xyz*0.5+0.5;
    }
    frag_out0 *= c;
}