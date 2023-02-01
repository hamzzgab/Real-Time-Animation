#version 330 core
struct Light
{
    vec3 direction;
};

in vec3 Normal;

out vec4 color;

uniform vec3 viewPos;
uniform int layers;
uniform Light light;

void main()
{
    float intensity;
    intensity = dot(-light.direction, normalize(Normal));
    
    if (layers == 1)
        color = vec4(0.3, 1.0, 0.3, 1.0);
    else if (layers == 2)
        if (intensity > 0.95)
            color = vec4(0.3, 1.0, 0.3, 1.0);
        else
            color = vec4(0.0, 0.0, 0.0, 1.0);
    else if (layers == 3)
        if (intensity > 0.95)
            color = vec4(0.3, 1.0, 0.3, 1.0);
        else if (intensity > 0.45)
            color = vec4(0.1, 0.5, 0.1, 1.0);
        else
            color = vec4(0.0, 0.0, 0.0, 1.0);
    else if (layers == 4)
        if (intensity > 0.95)
            color = vec4(0.3, 1.0, 0.3, 1.0);
        else if (intensity > 0.65)
            color = vec4(0.2, 0.6, 0.2, 1.0);
        else if (intensity > 0.35)
            color = vec4(0.1, 0.2, 0.1, 1.0);
        else
            color = vec4(0.0, 0.0, 0.0, 1.0);
    else if (layers == 5)
        if (intensity > 0.95)
            color = vec4(0.3, 1.0, 0.3, 1.0);
        else if (intensity > 0.75)
            color = vec4(0.2, 0.7, 0.2, 1.0);
        else if (intensity > 0.55)
            color = vec4(0.1, 0.4, 0.1, 1.0);
        else if (intensity > 0.35)
            color = vec4(0.0, 0.1, 0.0, 1.0);
        else
            color = vec4(0.0, 0.0, 0.0, 1.0);
    else if (layers == 6)
        if (intensity > 0.95)
            color = vec4(0.3, 1.0, 0.3, 1.0);
        else if (intensity > 0.75)
            color = vec4(0.2, 0.8, 0.2, 1.0);
        else if (intensity > 0.55)
            color = vec4(0.1, 0.6, 0.1, 1.0);
        else if (intensity > 0.35)
            color = vec4(0.0, 0.4, 0.0, 1.0);
        else if (intensity > 0.15)
            color = vec4(0.0, 0.2, 0.0, 1.0);
        else
            color = vec4(0.0, 0.0, 0.0, 1.0);
    else
        color = vec4(0.0, 1.0, 0.0, 1.0);
}

