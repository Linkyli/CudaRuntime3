#pragma once
#include<vector>
#include <fstream>
#include"Point3f.h"
#include <iostream>
#include <string>
#include"ReadSTLFile.h"
#include <sstream>    

class ReadPath {
public:
	int CricleNum;
	int PosNum;
	vector<Point3f> CutterPath;
	void ReadPathfile(const char* cfilename);
	bool ReadASCII(const char* buffer);

};
