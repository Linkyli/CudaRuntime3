#version 440 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 viewInverse;
uniform mat4 projectionInverse;

in VS_OUT {
     vec3 FragPos;
} gs_in[];//��Ϊ�ڼ�����ɫ���ж�������һ�����ʽ��������������Ҫʹ���������洢����������Ϣ


out GS_OUT {
     vec3 FragPos;
     vec3 Normal;
} gs_out;//�Խṹ�����ʽ�����ƬԪ��ɫ��

//out vec3 FragPos;//�����Ƭ����ɫ�����ǵ����������ԣ���˲���������ʽ

vec3 GetNormal()
{
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(a, b));
}


void main() {    

    vec3 Normal = GetNormal();

    gl_Position = gl_in[0].gl_Position;
    gs_out.FragPos = gs_in[0].FragPos;
    gs_out.Normal = Normal;
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    gs_out.FragPos = gs_in[1].FragPos;
    gs_out.Normal = Normal;
    EmitVertex();

    gl_Position = gl_in[2].gl_Position;
    gs_out.FragPos = gs_in[2].FragPos;
    gs_out.Normal = Normal;
    EmitVertex();

    EndPrimitive();
}
