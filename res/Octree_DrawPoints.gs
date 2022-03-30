#version 440 core
layout (points) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in VS_OUT {
   vec3 Dimension;
} gs_in[];


out GS_OUT {
     vec3 FragPos;
     vec3 Normal;
} gs_out;//以结构体的形式输出到片元着色器

  vec4 P1;//y轴方向
  vec4 P2;//z轴方向
  vec4 P3;//yz轴方向


//vec4 P1,vec4 P2 ,vec4 P3
vec3 GetNormal()
{
    vec3 a = vec3(P1) - vec3(P3);
    vec3 b = vec3(P3) - vec3(P2);
    return normalize(cross(a, b));
}

void EmitQuard1(){
     P1 = gl_in[0].gl_Position;
     P2 = gl_in[0].gl_Position;
     P3 = gl_in[0].gl_Position;
     P1.y = gl_in[0].gl_Position.y + gs_in[0].Dimension.y;
     P2.z = gl_in[0].gl_Position.z + gs_in[0].Dimension.z;
     P3.yz = gl_in[0].gl_Position.yz + gs_in[0].Dimension.yz;//初始化三个矩形顶点

     vec3 FragPos1 = vec3(model * P1);
     vec3 FragPos2 = vec3(model * P2);
     vec3 FragPos3 = vec3(model * P3);//计算面片的世界坐标用于计算光照

     P1 = projection * view * model * P1;
     P2 = projection * view * model * P2;
     P3 = projection * view * model * P3;//获取投影坐标，输出面片的的顶点坐标
     
    
    
    vec3 a = vec3(P3) - vec3(P2);
    vec3 b = vec3(P3) - vec3(P1);
    vec3 normal = normalize(cross(a, b));////使用的是相机坐标系 

     gl_Position =  projection * view * model * gl_in[0].gl_Position;
     gs_out.FragPos =  vec3(model * gl_in[0].gl_Position);
     gs_out.Normal = normal;
     EmitVertex();//添加顶点1

     gl_Position = P1;
     gs_out.FragPos = FragPos1;
     gs_out.Normal = normal;
     EmitVertex();//添加顶点2

     gl_Position = P2;
     gs_out.FragPos = FragPos2;
     gs_out.Normal = normal;
     EmitVertex();//添加顶点3

     gl_Position = P3;
     gs_out.FragPos = FragPos3;
     gs_out.Normal = normal;
     EmitVertex();//添加顶点4

     EndPrimitive();//添加输出的一个矩形面片
    
}

void EmitQuard2(){

     P1 = gl_in[0].gl_Position;
     P2 = gl_in[0].gl_Position;
     P3 = gl_in[0].gl_Position;

     P1.x = gl_in[0].gl_Position.x + gs_in[0].Dimension.x;
     P2.z = gl_in[0].gl_Position.z + gs_in[0].Dimension.z;
     P3.xz = gl_in[0].gl_Position.xz + gs_in[0].Dimension.xz;

     vec3 FragPos1 = vec3(model * P1);
     vec3 FragPos2 = vec3(model * P2);
     vec3 FragPos3 = vec3(model * P3);

     P1 = projection * view * model * P1;
     P2 = projection * view * model * P2;
     P3 = projection * view * model * P3;
     
     vec3 a = vec3(P3) - vec3(P2);
     vec3 b = vec3(P3) - vec3(P1);
     vec3 normal = normalize(cross(a,b));////使用的是相机坐标系 

    
     gl_Position =  projection * view * model * gl_in[0].gl_Position;
     gs_out.FragPos =  vec3(model * gl_in[0].gl_Position);
     gs_out.Normal = normal;
     EmitVertex();


     gl_Position = P1;
     gs_out.FragPos = FragPos1;
     gs_out.Normal = normal;
     EmitVertex();
     
     gl_Position = P2;
     gs_out.FragPos = FragPos2;
     gs_out.Normal = normal;
     EmitVertex();
      
     gl_Position = P3;
     gs_out.FragPos = FragPos3;
     gs_out.Normal = normal;
     EmitVertex();

     EndPrimitive();
    
}

void EmitQuard3(){

     P1 = gl_in[0].gl_Position;
     P2 = gl_in[0].gl_Position;
     P3 = gl_in[0].gl_Position;

     P1.x = gl_in[0].gl_Position.x + gs_in[0].Dimension.x;
     P2.y = gl_in[0].gl_Position.y + gs_in[0].Dimension.y;
     P3.xy = gl_in[0].gl_Position.xy + gs_in[0].Dimension.xy;

     vec3 FragPos1 = vec3(model * P1);
     vec3 FragPos2 = vec3(model * P2);
     vec3 FragPos3 = vec3(model * P3);

     P1 = projection * view * model * P1;
     P2 = projection * view * model * P2;
     P3 = projection * view * model * P3;
     
    vec3 a = vec3(P3) - vec3(P1);
    vec3 b = vec3(P3) - vec3(P2);
    vec3 normal = normalize(cross(a, b));////使用的是相机坐标系 
    
     gl_Position =  projection * view * model * gl_in[0].gl_Position;
     gs_out.FragPos = vec3(model * gl_in[0].gl_Position);
     gs_out.Normal = normal;
     EmitVertex();


     gl_Position = P1;
     gs_out.FragPos = FragPos1;
     gs_out.Normal = normal;
     EmitVertex();
     
     gl_Position = P2;
     gs_out.FragPos = FragPos2;
     gs_out.Normal = normal;
     EmitVertex();
      
     gl_Position = P3;
     gs_out.FragPos = FragPos3;
     gs_out.Normal = normal;
     EmitVertex();

     EndPrimitive();
    
}

void main() {  
    if((gs_in[0].Dimension.x +  gs_in[0].Dimension.y +  gs_in[0].Dimension.z) == 0 ) return;
    EmitQuard1();
    EmitQuard2();
    EmitQuard3();
}



