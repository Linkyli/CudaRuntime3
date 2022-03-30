#pragma once
#include<vector>
#include"Point3f.h"
using namespace std;

class ReadSTLFile
{
public:
    bool ReadFile(const char* cfilename);
    int NumTri();
    vector<Point3f>& PointList();
    vector<Point3f>& NormalList();
private:
    vector<Point3f> pointList;
    vector<Point3f> normalList;
    unsigned int unTriangles;
    bool ReadASCII(const char* cfilename);
    bool ReadBinary(const char* cfilename);

    char* memwriter;
    int cpyint(const char*& p);
    float cpyfloat(const char*& p);
};
