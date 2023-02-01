#version 330 core
#define PI 3.14159

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

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

uniform float shininess;
uniform float albedo;
uniform float roughness;

//float albedo = 3.0;
//float roughness = 0.1;

// framebuffer output
out vec4 fb_color;

float t(float s, vec3 lightDir, vec3 norm, vec3 eye) {
    if (s > 0) {
       return max(dot(norm, lightDir), dot(norm, eye));
    }
    else{
        return 1.0;
    }
}

void main() {
    
    // AMBIENT
    vec3 ambient = light.ambient * vec3(texture(material.ambient, TexCoords));
    
    // DIFFUSE
    vec3 norm = normalize(Normal);
    vec3 lightDir = light.direction - FragPos;
    float distance = length(lightDir);
    lightDir = normalize(-light.direction);
    
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords)) / distance * distance;
    
    vec3 eye = normalize(-FragPos);
    float specularity = 0.0;
    
    float a = (1/PI) * (1.0 - (0.5 * ((roughness * roughness) / ((roughness * roughness) + 0.33))) +
                (0.17 * albedo * ((roughness * roughness) / ((roughness * roughness) + 0.13))));
    float b = (1/PI) * (0.45 * ((roughness * roughness) / ((roughness * roughness) + 0.09)));
    float s = dot(lightDir, eye) - (dot(norm, lightDir) * dot(norm, eye));
    float lambert = albedo * dot(norm, lightDir) * (a + (b * (s / t(s, lightDir, norm, eye))));
    
    if (lambert > 0) {
        vec3 h = normalize(lightDir+eye);
        float specularAngle = max(dot(h, norm), 0.0);
        specularity = pow(specularAngle, shininess);
    }
    vec3 fragColour = ambient + lambert * diffuse + ( specularity * light.specular * vec3(texture(material.specular, TexCoords)) ) / distance * distance;

    fb_color = vec4(fragColour, 1);
}

