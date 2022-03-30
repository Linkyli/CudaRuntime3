#include"../src/ReadSTLfile/ReadPath.h"

void ReadPath::ReadPathfile(const char* cfilename) {
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



    ReadASCII(buffer);

    free(buffer);
    return;
}

bool ReadPath::ReadASCII(const char* buffer) {
    //CricleNum;
    float x, y, z;
    string Position;
    stringstream ss(buffer);
    ss >> CricleNum >> PosNum;

    while (CricleNum) {
        while (PosNum) {
            ss >> x >> y >> z;
            CutterPath.push_back(Point3f(x, y, z));
            PosNum--;
        }
        ss >> PosNum;
       // cout << "PosNum: " << PosNum;
        CricleNum--;
    }
    
    //cout << "CutterPath.size: " << CutterPath.size()<<endl;


    /*
    for (int i = 0; i < 200; i++) {
        cout << "Position "<< i <<" (" << CutterPath[i].x << "," << CutterPath[i].y << "," << CutterPath[i].z << ")" << endl;
        cout <<"test: " << CutterPath[i].x * CutterPath[i].y * CutterPath[i].z << endl;
    }
    */

    /*
    cout << "CricleNum:" << CricleNum <<endl<< "PosNum: " << PosNum << endl;

    getline(ss, Position);
    //Position >> x >> y >> z;
    ss >> x >> y >> z;
   // float x = Position;
    cout << "Position(" << x<<"," << y<<"," << z<<")"<<endl;

    ss >> x >> y >> z;
    // float x = Position;
    cout << "Position(" << x << "," << y << "," << z << ")"<<endl;


    getline(ss, Position);
    cout << "getline(ss, Position):(" << Position<<",";
    getline(ss, Position);
    cout << Position<<",";
    getline(ss, Position);
    cout << Position<<")" << endl;
    */


    return true;
}
