#pragma once
#include<iostream>
#include<vector>
#include"../ReadSTLfile/Point3f.h"
#include"../ReadSTLfile/Point3i.h"

using namespace std;

const int BrickLength  = 512  ;

class Octree {
	/*
		Children follow a predictable pattern to make accesses simple.
		Here, - means less than 'origin' in that dimension, + means greater than.
		child:	0 1 2 3 4 5 6 7
		x:      - - - - + + + +
		y:      - - + + - - + +
		z:      - + - + - + - +
		0-3�������  4-7�����Ұ�
	*/
public:
	Point3f origin;         //! The physical center of this node
	Point3f halfDimension;  //! Half the width/height/depth of this node ���صĳ���ߵİ�ֵ
	Octree* children[8]; //! Pointers to child octants
	int accuracy = 0;
	int exist = 8;//�ýڵ��Ƿ��ѱ��޳�
	bool sub = false;//
	byte index = 0;
	int RenderedIndex = -1;//��RenderedIndex ��Ϊ-1ʱ��֤���������ѱ��������Ⱦ�б�
	bool Rendered = false;
	//byte IsSurface;
	//int sum = 8;
	void* Parent = nullptr;
	void* bricks = nullptr;
	//12 + 12 + 64 + 4 + 4 + 1 + 4 + 16 = 117
public:
	Octree(const Point3f& origin, const Point3f& halfDimension)
		: origin(origin), halfDimension(halfDimension) {
		// Initially, there are no children
		RenderedIndex = -1;
		for (int i = 0; i < 8; ++i)
			children[i] = NULL;
	}

	Octree(const Octree& copy)
		: origin(copy.origin), halfDimension(copy.halfDimension) {
		RenderedIndex = -1;
		for (int i = 0; i < 8;++i) {
			children[i] = NULL;
		}
	}

	~Octree() {
		// Recursively destroy octants
		//for (int i = 0; i < 8; ++i) delete children[i];
	}

	//���ݵ��λ���ж������Ǹ��ӽڵ��У�����˵�ĸ��ӽڵ㺬�иõ�
	int getOctantContainingPoint(const Point3f& point) const {
		int oct = 0;
		if (point.x >= origin.x) oct |= 4;
		if (point.y >= origin.y) oct |= 2;
		if (point.z >= origin.z) oct |= 1;
		return oct;
	}

	//ϸ�ָýڵ�
	void SubDivision() {
		sub = true;
		Point3f SubOrigin;
		Point3f SubHalfDimension((halfDimension.x / 2), (halfDimension.y / 2), (halfDimension.z / 2));

		//float Xoffset = -1 * halfDimension.x;////X���������ҷ���
		//float Yoffset = -1 * halfDimension.y; //Y��������ǰ����
		//float Zoffset = -1 * halfDimension.z;//Z���������Ϸ���
		for (int i = 0; i <= 7; ++i) {
			
			/*if (((i & 1) == 1)) {
				SubOrigin.z = origin.z + halfDimension.z/2;
			}
			else {
				SubOrigin.z = origin.z - halfDimension.z/2;
			}
			if (((i & 2) == 2)) { 
				SubOrigin.y = origin.y + halfDimension.y/2;
			} 
			else{
				SubOrigin.y = origin.y - halfDimension.y/2;
			}

			if (((i & 4) == 4)) { 
				SubOrigin.x = origin.x + halfDimension.x/2;
			}
			else {
				SubOrigin.x = origin.x - halfDimension.x/2;
			}
			*/
			SubOrigin.z = origin.z - halfDimension.z / 2;
			SubOrigin.z = SubOrigin.z + (i & 1) * halfDimension.z;

			SubOrigin.y = origin.y - halfDimension.y / 2;
			SubOrigin.y = SubOrigin.y + (i & 2) * halfDimension.y / 2;

			SubOrigin.x = origin.x - halfDimension.x / 2;
			SubOrigin.x = SubOrigin.x + (i & 4) * halfDimension.x / 4;

			children[i] = new Octree(SubOrigin, SubHalfDimension);
			children[i]->accuracy = accuracy + 1;
			children[i]->Parent = this;
			children[i]->index = i;
			children[i]->exist = 8;

		}


	}

};