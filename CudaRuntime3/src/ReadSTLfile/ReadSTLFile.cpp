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

    /* ��Ҫһ��byte��©�ض��������ļ���ֻ�ܲ��ö����Ʒ�ʽ�� */
    fopen_s(&pFile, cfilename, "rb");
    if (pFile == NULL)
    {
        fputs("File error", stderr);
        exit(1);
    }

    /* ��ȡ�ļ���С */
    fseek(pFile, 0, SEEK_END);
    lSize = ftell(pFile);
    rewind(pFile);

    /* �����ڴ�洢�����ļ� */
    buffer = (char*)malloc(sizeof(char) * lSize);
    if (buffer == NULL)
    {
        fputs("Memory error", stderr);
        exit(2);
    }

    /* ���ļ�������buffer�� */
    result = fread(buffer, 1, lSize, pFile);
    if (result != lSize)
    {
        fputs("Reading error", stderr);
        exit(3);
    }


    /* ������ʾ���ر��ļ����ͷ��ڴ� */
    fclose(pFile);

    ios::sync_with_stdio(false);
    ReadBinary(buffer);//ֻ�ж�binary��ʽ��
   /* if (buffer[79] != '\0')//�жϸ�ʽ
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
    unTriangles = cpyint(p);//��ȡ������Ƭ��
    for (i = 0; i < unTriangles; i++)//��������������Ƭ����������Ϣ��һ��ѭ������һ�����㣻
    {
        //p += 12;//����ͷ�����������������ڹ���Dexelģ�͵Ĺ�����Ҳ���������ã����ݷ�����ֱ���жϸ����Ƕ������ǵײ�
        //��ȡ���������ں�������Dexelģ��
        normalList.push_back(Point3f(cpyfloat(p), cpyfloat(p), cpyfloat(p)));
        for (j = 0; j < 3; j++)//��ȡ������
            pointList.push_back(Point3f(cpyfloat(p), cpyfloat(p), cpyfloat(p)));
        p += 2;//����β����־
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
