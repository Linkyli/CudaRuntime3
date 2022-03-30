#include <vector>
#include <fstream>
#include"Point3f.h"
#include <iostream>
#include <string>
#include"ReadSTLFile.h"
#include <sstream>    

using namespace std;

bool ReadSTLFile::ReadFile(const char* cfilename)
{
    FILE* pFile;
    long lSize;
    char* buffer;
    size_t result;

    /* 若要一个byte不漏地读入整个文件，只能采用二进制方式打开 */
    fopen_s(&pFile, cfilename, "rb");
    if (pFile == NULL)
    {
        fputs("File error", stderr);
        exit(1);
    }

    /* 获取文件大小 */
    fseek(pFile, 0, SEEK_END);
    lSize = ftell(pFile);
    rewind(pFile);

    /* 分配内存存储整个文件 */
    buffer = (char*)malloc(sizeof(char) * lSize);
    if (buffer == NULL)
    {
        fputs("Memory error", stderr);
        exit(2);
    }

    /* 将文件拷贝到buffer中 */
    result = fread(buffer, 1, lSize, pFile);
    if (result != lSize)
    {
        fputs("Reading error", stderr);
        exit(3);
    }


    /* 结束演示，关闭文件并释放内存 */
    fclose(pFile);

    ios::sync_with_stdio(false);
    ReadBinary(buffer);//只判断binary格式的
   /* if (buffer[79] != '\0')//判断格式
    {
        ReadASCII(buffer);
    }
    else
    {
        ReadBinary(buffer);
    }
    */
    ios::sync_with_stdio(true);

    free(buffer);
    return true;
}

bool ReadSTLFile::ReadASCII(const char* buffer)
{
    cout << "ASCII" << endl;
    unTriangles = 0;
    float x, y, z;
    int i;
    string name, useless;
    stringstream ss(buffer);
    ss >> name >> name;
    ss.get();
    do {
        ss >> useless;
        if (useless != "facet")
            break;
        getline(ss, useless);
        getline(ss, useless);
        for (i = 0; i < 3; i++)
        {
            ss >> useless >> x >> y >> z;
            pointList.push_back(Point3f(x, y, z));
        }
        unTriangles++;
        getline(ss, useless);
        getline(ss, useless);
        getline(ss, useless);
    } while (1);
    return true;
}

bool ReadSTLFile::ReadBinary(const char* buffer)
{
    cout << "Binary" << endl;
    const char* p = buffer;
    char name[80];
    int i, j;
    memcpy(name, p, 80);
    p += 80;
    unTriangles = cpyint(p);//获取三角面片数
    for (i = 0; i < unTriangles; i++)//遍历所有三角面片并处理顶点信息，一次循环处理一个顶点；
    {
        //p += 12;//跳过头部法向量，法向量在构建Dexel模型的过程中也可以起到作用，根据法向量直接判断该面是顶部还是底部
        //读取法向量用于后续构建Dexel模型
        normalList.push_back(Point3f(cpyfloat(p), cpyfloat(p), cpyfloat(p)));
        for (j = 0; j < 3; j++)//读取三顶点
            pointList.push_back(Point3f(cpyfloat(p), cpyfloat(p), cpyfloat(p)));
        p += 2;//跳过尾部标志
    }
    return true;
}

int ReadSTLFile::NumTri()
{
    return unTriangles;
}

vector<Point3f>& ReadSTLFile::PointList()
{
    return pointList;
}
vector<Point3f>& ReadSTLFile::NormalList()
{
    return normalList;
}
int ReadSTLFile::cpyint(const char*& p)
{
    int cpy;
    memwriter = (char*)&cpy;
    memcpy(memwriter, p, 4);
    p += 4;
    return cpy;
}
float ReadSTLFile::cpyfloat(const char*& p)
{
    float cpy;
    memwriter = (char*)&cpy;
    memcpy(memwriter, p, 4);
    p += 4;
    return cpy;
}
