#version 330 core
struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float     shininess;
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

// [3] OpenGL. (2014, June 1). In Learn OpenGL. https://learnopengl.com/Getting-started/OpenGL

void main()
{
    float shine = material.shininess;
    
    // AMBIENT - ka
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    
    // DIFFUSE - kd
    vec3 norm = normalize(Normal);               // N
    vec3 lightDir = normalize(-light.direction); // L

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    
    // SPECULAR - ks
    vec3 viewDir = normalize(viewPos - FragPos); // E
    vec3 reflectDir = reflect(-lightDir, norm);  // R
    
//    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess); // n
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess); // n
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    
    color = vec4(ambient + diffuse + specular, 1.0f);
}
