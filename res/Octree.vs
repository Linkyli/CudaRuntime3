#version 330 core

layout (location = 0) in vec3 position;
//out vec3 ourColor; // ��Ƭ����ɫ�����һ����ɫ

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;



void main()
{
     //gl_Position = vec4(position.x, position.y, position.z,1.0f);
     gl_Position = projection * view * model * vec4(position.x, position.y, position.z,1.0f);
}