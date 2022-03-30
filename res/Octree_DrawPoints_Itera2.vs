#version 440 core
layout(location = 0) in vec3 position;
layout(location = 1) in float pre;

out VS_OUT{
   vec3 Dimension;
} vs_out;

uniform vec3 HalfDimension;

void main()
{
    gl_Position = vec4(position, 1.0f);
    vec3 temp = HalfDimension;
    vs_out.Dimension = (temp/(pre)) * 2;


    //vec3 temp = vec3(20,20,20);
    //temp.x = HalfDimension.x / pre;
    //temp.y = HalfDimension.y / pre;
    //temp.z = HalfDimension.z / pre;
    //vs_out.Dimension = (temp/(pre.x)) * 2;
    //vs_out.Dimension = HalfDimension * 2 / pre.x ;

}
//layout(location = 1) in vec3 pre;

/*
    vec3 temp;
    vec3 temp1 = vec3(20,20,20);

    int a = Itera/abs(Itera);
    int itera = Itera;


    temp1.x = HalfDimension.x /itera;
    temp1.y = HalfDimension.y /itera;
    temp1.z = HalfDimension.z /itera;

   
    temp1.x = temp1.x /itera;
    temp1.y = temp1.y /itera;
    temp1.z = temp1.z /itera;
*/