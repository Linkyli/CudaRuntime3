#version 440 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 viewInverse;
uniform mat4 projectionInverse;

in VS_OUT {
     vec3 FragPos;
     vec4 topology;
} gs_in[];//因为在几何着色器中顶点是以一组的形式被输入进来，因此要使用数组来存储多个顶点的信息


out GS_OUT {
     vec3 FragPos;
     vec3 Normal;
} gs_out;//以结构体的形式输出到片元着色器


vec3 GetNormal()
{
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(a, b));
}

void EmitTriangle(vec4 a,vec4 b,vec4 c){
    vec3 m = vec3(a) - vec3(b);
    vec3 n = vec3(b) - vec3(c);

    vec3 Normal = normalize(cross(m, n));

    gl_Position = a;
    gs_out.FragPos = vec3(viewInverse * projectionInverse * a);
    gs_out.Normal = Normal;
    EmitVertex();

    gl_Position = b;
    gs_out.FragPos = vec3(viewInverse * projectionInverse * b);
    gs_out.Normal = Normal;
    EmitVertex();

    gl_Position = c;
    gs_out.FragPos = vec3(viewInverse * projectionInverse * c);
    gs_out.Normal = Normal;
    EmitVertex();
    
    EndPrimitive();
    
}
void main() {    

    vec4 FacePoint = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position) / 3;
    
    vec4 edgePoint[3] ;
    vec4 vertexPoint[3] ;
    vec4 facePoint[3];

    edgePoint[0] =  (gl_in[0].gl_Position + gl_in[1].gl_Position)  * (0.375) + (gs_in[0].topology + gl_in[2].gl_Position)  * (0.125);
    edgePoint[1] =  (gl_in[0].gl_Position + gl_in[2].gl_Position)  * (0.375) + (gs_in[1].topology + gl_in[1].gl_Position)  * (0.125);
    edgePoint[2] =  (gl_in[1].gl_Position + gl_in[2].gl_Position)  * (0.375) + (gs_in[2].topology + gl_in[0].gl_Position)  * (0.125);
     
    facePoint[0] = (gl_in[0].gl_Position + gl_in[1].gl_Position + gs_in[0].topology)/3;
    facePoint[1] = (gl_in[0].gl_Position + gl_in[2].gl_Position + gs_in[2].topology)/3;
    facePoint[2] = (gl_in[1].gl_Position + gl_in[2].gl_Position + gs_in[1].topology)/3;
    
    vec4 SumFacePoint =  facePoint[0] +  facePoint[1] + facePoint[2] + FacePoint ;
    
  /*  vertexPoint[0] =    0.5 * 0.125 *  ((SumFacePoint -  facePoint[1])/3) + 0.375 * 0.125 * ( 4 * gl_in[0].gl_Position + gl_in[1].gl_Position  +
                         gl_in[2].gl_Position  + gs_in[0].topology + gs_in[2].topology )  + (0.5625) * gl_in[0].gl_Position;

    vertexPoint[1] =    0.5 * 0.125 *  ((SumFacePoint -  facePoint[2])/3) + 0.375 * 0.125 * ( 4 * gl_in[1].gl_Position + gl_in[0].gl_Position  +
                         gl_in[2].gl_Position  + gs_in[0].topology + gs_in[1].topology )  +  (0.5625)* gl_in[1].gl_Position;

    vertexPoint[2] =    0.5 * 0.125 *  ((SumFacePoint -  facePoint[0])/3) + 0.375 * 0.125 * ( 4 * gl_in[2].gl_Position + gl_in[0].gl_Position  +  
                         gl_in[1].gl_Position  + gs_in[1].topology + gs_in[2].topology )  +  (0.5625) * gl_in[2].gl_Position;
   
   */

   float m2 = 1.0f / 3.0f;
   float m3 = 2.0f / 3.0f;

    vertexPoint[0] =    m2 *  ((SumFacePoint -  facePoint[1])/3) + 0.125 * m3 * ( 4 * gl_in[0].gl_Position + gl_in[1].gl_Position  +
                         gl_in[2].gl_Position  + gs_in[0].topology + gs_in[2].topology );

    vertexPoint[1] =    m2 *  ((SumFacePoint -  facePoint[2])/3) + 0.125 * m3 * ( 4 * gl_in[1].gl_Position + gl_in[0].gl_Position  +
                         gl_in[2].gl_Position  + gs_in[0].topology + gs_in[1].topology );

    vertexPoint[2] =    m2 *  ((SumFacePoint -  facePoint[0])/3) + 0.125 * m3 * ( 4 * gl_in[2].gl_Position + gl_in[0].gl_Position  +  
                         gl_in[1].gl_Position  + gs_in[1].topology + gs_in[2].topology );

    EmitTriangle(FacePoint, edgePoint[0] ,vertexPoint[1]);
    EmitTriangle(FacePoint, edgePoint[0] ,vertexPoint[2]);

    EmitTriangle(FacePoint, vertexPoint[1], edgePoint[0]);
    EmitTriangle(FacePoint, edgePoint[1] ,vertexPoint[1]);
   
    EmitTriangle(FacePoint, vertexPoint[2], edgePoint[1]);
    EmitTriangle(FacePoint, edgePoint[2] ,vertexPoint[2]);



}
