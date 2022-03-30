#pragma once
#include<math.h>
//#include<d3d9.h>
//#include<d3dx9math.h>//µÈ´ı´¦Àí
#include<windows.h>



class Point3f
{
public:
    float x, y, z;
public:
    Point3f();
    Point3f(float _x, float _y, float _z);
    int SetParam(float _x, float _y, float _z);
    float GetCos(Point3f a);

    Point3f operator-(const Point3f& a) {
        return Point3f(x - a.x, y - a.y, z - a.z);
    }

    Point3f operator+(const Point3f& a) {
        return Point3f(x + a.x, y + a.y, z + a.z);
    }

    Point3f operator*(const  float a) {
        return Point3f(x * a, y * a, z * a);
    }

    bool operator==(const Point3f& p) const
    {
        if (((x - p.x) >= 0.000001)) return false;
        if (((y - p.y) >= 0.000001)) return false;
        if (((z - p.z) >= 0.000001)) return false;
        return true;
    }
 
    /*/inline Vertex_f IVertex()
    {
        return Vertex_f(x, y, z);
    }*/

};
