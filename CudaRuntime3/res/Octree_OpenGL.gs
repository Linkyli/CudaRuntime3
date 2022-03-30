#version 440 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 viewInverse;
uniform mat4 projectionInverse;

in VS_OUT {
     vec3 FragPos;
} gs_in[];//因为在几何着色器中顶点是以一组的形式被输入进来，因此要使用数组来存储多个顶点的信息


out GS_OUT {
     vec3 FragPos;
     vec3 Normal;
} gs_out;//以结构体的形式输出到片元着色器

//out vec3 FragPos;//输出到片段着色器是是单个顶点属性，因此不用数组形式

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
