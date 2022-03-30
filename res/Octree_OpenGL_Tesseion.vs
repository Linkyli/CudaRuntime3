#version 440 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 topologys;
//out vec3 ourColor; // 向片段着色器输出一个颜色

out VS_OUT {
   vec3 FragPos;
   vec4 topology;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;



void main()
{
     //gl_Position = vec4(position.x, position.y, position.z,1.0f);

     gl_Position = projection * view * model * vec4(position.x, position.y, position.z,1.0f);
     vs_out.topology = projection * view * model * vec4(topologys.x, topologys.y, topologys.z,1.0f);

     vs_out.FragPos = position;
}
