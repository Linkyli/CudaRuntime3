#version 440 core
out vec4 FragColor;

//in  vec3 Normal
//in  vec3 FragPos;
in GS_OUT {
    vec3 FragPos;
    vec3 Normal;
} gs_out;

uniform vec3 lightPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;


void main()
{
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(gs_out.Normal);
    vec3 lightDir = normalize(lightPos - gs_out.FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
            
    vec3 result = (ambient + diffuse) * objectColor;

    FragColor = vec4(result, 1.0);

}
