#version 330 core

in Varyings {
    vec4 color;
    vec2 tex_coord;
} fs_in;

out vec4 frag_color;

uniform vec4 tint;
uniform sampler2D tex;

void main(){
    // (Req 7) Modify the following line to compute the fragment color
    // by multiplying the tint with the vertex color and with the texture color 
    // frag_color = vec4(1.0);
    // frag_color = vec4(tint*fs_in.color*texture(tex,fs_in.tex_coord));
    frag_color = tint*fs_in.color*texture(tex,fs_in.tex_coord);  // multiply tint, vertex color and texture color to get final color   
}