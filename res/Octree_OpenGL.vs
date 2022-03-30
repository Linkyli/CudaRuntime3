#version 440 core

layout (location = 0) in vec3 position;

out VS_OUT {
   vec3 FragPos;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;



void main()
{
     //gl_Position = vec4(position.x, position.y, position.z,1.0f);

     gl_Position = projection * view * model * vec4(position.x, position.y, position.z,1.0f);

     vs_out.FragPos = vec3(model * vec4(position, 1.0));

}