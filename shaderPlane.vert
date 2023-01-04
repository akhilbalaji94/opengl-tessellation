#version 330 core
layout(location=0) in vec3 pos;
layout(location=1) in vec3 norm;
layout(location=2) in vec2 tex;
uniform mat4 mvp;
uniform mat4 mv;
uniform mat4 m;

out vec3 view_pos;
out vec3 world_pos;
out vec2 tex_coord;

void main()
{
    gl_Position = mvp * vec4(pos, 1.0);
    view_pos = vec3(mv * vec4(pos, 1.0));
    world_pos = vec3(m * vec4(pos, 1.0));
    tex_coord = tex;
}