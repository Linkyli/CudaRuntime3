#include"Point3f.h"

Point3f::Point3f() :x(0), y(0), z(0)
{
}

Point3f::Point3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
{
}

int Point3f::SetParam(float _x, float _y, float _z)
{
    x = _x;
    y = _y;
    z = _z;
    return 0;
}


float Point3f::GetCos(Point3f a) {
    return (x * a.x + y * a.y + z * a.z) / (sqrt(x * x + y * y + z * z) * sqrt(a.x * a.x + a.y * a.y + a.z * a.z));
}

