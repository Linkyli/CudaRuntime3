#pragma once
#include"Point3i.h"

Point3i::Point3i() :x(0), y(0), z(0)
{
}

Point3i::Point3i(int _x, int _y, int _z) : x(_x), y(_y), z(_z)
{
}

int Point3i::SetParam(int _x, int _y, int _z)
{
    x = _x;
    y = _y;
    z = _z;
    return 0;
}


int Point3i::GetCos(Point3i a) {
    return (x * a.x + y * a.y + z * a.z) / (sqrt(x * x + y * y + z * z) * sqrt(a.x * a.x + a.y * a.y + a.z * a.z));
}