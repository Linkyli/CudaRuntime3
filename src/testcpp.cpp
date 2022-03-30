#include<iostream>
#include <sstream>
#include <iomanip>
#include<bitset>
#include<../src/ReadSTLfile/Point3f.h>
#include<../src/Octree/MyStruct.h>
#include<../src/ReadSTLfile/ReadPath.h>
using namespace std;


int main() {
	ReadPath path;
	path.ReadPathfile("./res/temp.path");

	Point3f Pos(1, 2, 3);
	float temp = *(&Pos.x + 2);
	cout << "temp:"<<temp << endl;

	byte A = -1;

	cout<<"A:" << bitset<sizeof(A) * 8>(A) << endl;
	return 0;
}



/*
int main() {
	long long int Fint = 1;
	long long int A = 85624869866995;
	cout<< "long long int :" << sizeof(long long int) << " ×Ö½Ú" << endl;

	int a = 4045;
	int index = a / 64;
	int forward = a % 64;

	long long int mask = 1;
	//forward = 63 - forward;
	mask = mask << forward;

	cout << "a£º" << a << "index: " << index << "forward: " << forward << endl;
	long long int Anti_Fint = ~Fint;
	cout << A << endl;


	cout << bitset<sizeof(A) * 8>(A) << "\n";

	A = A | mask;
	A = A & mask;
	cout << bitset<sizeof(mask) * 8>(mask) << endl;

	bool T = false;
	bool B;
	B = B >>4;
	cout << T << endl;
	cout<<"bool: " << bitset<sizeof(B) * 8>(B) << endl << endl;

	for (int i = 0; i < 512; i++) {
		for (int j = 0; j < 8;j++) {

			cout << i * 8 + j << endl;
        }
	}




	BitBricks TestBrick;
	TestBrick.brick[10] = B;
	for (int j = 0; j < 512; j++) {
		cout << bitset<sizeof(char) * 8>(TestBrick.brick[j]) << endl;
	}
	
	int     BrickIndex = 0;
	int        Forward = 0;


	system("pause");
	return 0;
}
*/