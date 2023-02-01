#version 330 core
struct Material
{
    sampler2D ambient;
    sampler2D diffuse;
    sampler2D specular;
};

struct Light
{
    vec3 direction;
    vec3 color;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform float darkenCoefficient;
uniform float shininess;
uniform bool blinn;

out vec4 color;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    // AMBIENT
    vec3 ambient = light.ambient * vec3(texture(material.ambient, TexCoords));
    
    // DIFFUSE
    vec3 norm = normalize(Normal);
    vec3 lightDir = light.direction - FragPos;
    float distance = length(lightDir);
    lightDir = normalize(-light.direction);
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords)) / distance * distance;
    
    // SPECULAR
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // MINNAERT
    float nDotL = dot(norm, lightDir);
    float nDotE = dot(norm, viewDir);
    
    float intensity = max(clamp(nDotL, 0.0, 1.0) * pow(nDotL * nDotE, darkenCoefficient) / distance * distance, 0);
    
    //BLINN
    float spec = 0.0f;
    if (blinn){
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(Normal, halfwayDir), 0.0), shininess);
    }else{
        vec3 reflectDir = reflect(-lightDir, norm);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    }
    
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords)) / distance * distance;
    
    vec3 result = ambient + intensity * diffuse + specular;

    color = vec4(result, 1.0);
}
