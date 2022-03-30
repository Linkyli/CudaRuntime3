#include<iostream>
#include <sstream>
#include <iomanip>
#include<bitset>
#include<../src/Octree/MyStruct.h>
using namespace std;
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

	/*
	for (int i = 0; i < 64; i++) {
		mask = 1;
		forward = 63 - i;
		mask = mask << forward;
		cout << bitset<sizeof(mask) * 8>(mask) << endl;
	}

	*/


	BitBricks TestBrick;
	TestBrick.brick[10] = B;
	for (int j = 0; j < 512; j++) {
		cout << bitset<sizeof(char) * 8>(TestBrick.brick[j]) << endl;
	}
	
	int     BrickIndex = 0;
	int        Forward = 0;
	/*
	for (int j = 0; j < BrickLength; j++) {
		BrickIndex = j / 64;
		Forward = j % 64;
		mask = 1;
		mask = mask << Forward;
		if (Forward > 5) continue;
		TestBrick.brick[BrickIndex] = TestBrick.brick[BrickIndex] | mask;
	}
	*/
	/*
	for (int i = 0; i < 64;i++) {
		
		cout << bitset<sizeof(TestBrick.brick[i]) * 8>(TestBrick.brick[i]) << endl;
	}
	cout << "ÅÐ¶ÏÕý¸º" << endl;
	for (int j = 0; j < BrickLength; j++) {
		BrickIndex = j / 64;
		Forward = j % 64;
		mask = 1;
		mask = mask << Forward;
		if (Forward == 0)cout << endl;
		if ((TestBrick.brick[BrickIndex] & mask) == 0) {
			cout << 0;
		}
		else {
			cout << 1;
		}
	
	}
	*/

	system("pause");
	return 0;
}