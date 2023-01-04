#version 330 core

in vec3 view_pos;
in vec3 world_pos;
in vec2 tex_coord;

uniform vec4 camera_pos;
uniform vec4 light_pos;
uniform sampler2D normal_map;

layout(location = 0) out vec3 color;

void main()
{
    vec3 diffuse_color = vec3(0.2, 0.2, 0.2);
    vec3 light_color = vec3(1.0, 1.0, 1.0);
    vec3 ambient_light_color = vec3(0.1, 0.1, 0.1);
    vec3 specular_color = vec3(0.7, 0.7, 0.7);
    float specular_power = 20.0;
    vec3 view_dir = normalize(camera_pos.xyz - world_pos);
    vec3 half_vector = normalize(light_pos.xyz - view_dir.xyz);

    vec3 view_normal = texture(normal_map, tex_coord).rgb;

    vec3 diffuse_shading = (light_color * clamp(dot(normalize(view_normal), normalize(light_pos.xyz - view_pos)), 0, 1) * diffuse_color);
    vec3 specular_shading = (light_color * pow(clamp(dot(normalize(view_normal), normalize(half_vector)), 0, 1), specular_power) * specular_color);
    vec3 ambient_shading = (ambient_light_color * diffuse_color);

    color = (diffuse_shading + specular_shading + ambient_shading);

}