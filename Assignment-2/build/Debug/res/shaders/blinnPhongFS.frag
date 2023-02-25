#version 330 core
struct Material
{
    sampler2D diffuse;
    sampler2D ambient;
    sampler2D specular;

};

struct Light
{
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

uniform float shininess;


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
    
    float spec = 0.0f;
    
    vec3 reflectDir = reflect(-lightDir, norm);
    spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);


    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords)) / distance * distance;
    
    color = vec4(ambient + diffuse + specular, 1.0f);
}
