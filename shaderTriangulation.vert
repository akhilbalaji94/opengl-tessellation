#version 330 core
layout(location=0) in vec3 model_pos;
layout(location=1) in vec3 model_norm;
layout(location=2) in vec2 model_tex;

out vec3 pos;
out vec3 norm;
out vec2 tex;

void main()
{
    pos = model_pos;
    norm = model_norm;
    tex = model_tex;
}