#version 330 core

struct Light
{
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;

uniform vec3 SurfaceColor;
uniform vec3 WarmColor;
uniform vec3 CoolColor;
uniform float DiffuseWarm;
uniform float DiffuseCool;
uniform Light light;

//variables passed by the vertex shader
in vec3 ReflectVec;
in vec3 ViewVec;

out vec4 fragColor;

void main() {
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = light.direction - FragPos;
    float distance = length(lightDir);
    lightDir = normalize(-light.direction);
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    
    float NdotL = dot(Normal, lightDir) * 0.5 + 0.5;
    
    vec3 kcool = min(CoolColor + DiffuseCool * SurfaceColor, 1.0);
    vec3 kwarm = min(WarmColor + DiffuseWarm * SurfaceColor, 1.0);


    vec3 kfinal = mix(kcool, kwarm, NdotL);

    vec3 nreflect = normalize(reflectDir);
    vec3 nview = normalize(viewDir);

    float spec = max(dot(nreflect, nview), 0.0);
    
    fragColor = vec4(min(kfinal + spec, 1.0), 1.0);
}
