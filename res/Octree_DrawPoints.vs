#version 440 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 HalfDimension;

out VS_OUT{
   vec3 Dimension;
} vs_out;

void main()
{
    gl_Position = vec4(position, 1.0f);
    vs_out.Dimension = HalfDimension * 2;
}