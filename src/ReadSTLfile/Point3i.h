#pragma once
#include<math.h>
#include<windows.h>


class Point3i
{
public:
    int x, y, z;
public:
    Point3i();
    Point3i(int _x, int _y, int _z);
    int SetParam(int _x, int _y, int _z);
    int GetCos(Point3i a);

    Point3i operator-(Point3i a) {
        return Point3i(x - a.x, y - a.y, z - a.z);
    }

    Point3i operator+(Point3i a) {
        return Point3i(x + a.x, y + a.y, z + a.z);
    }

    bool operator==(const Point3i& a) const
    {
        return (x == a.x && y == a.y && z == a.z);
    }
   /*inline Vertex_i IVertex()
    {
        return Vertex_i(x, y, z);
    }*/

};