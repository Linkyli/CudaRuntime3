#pragma once
#include<../src/Octree/Octree.h>

using namespace std;




struct BitBricks {
	BYTE brick[BrickLength] = {0};
	//bool brick[4096];
	//long long int brick[128];
};

struct Node {
	bool exist;//���Node�ڵ��Ƿ����
	bool Sub;//���Node�ڵ��Ƿ��ѱ��ָ�
};
struct OctreeNode {
	Node* Nodes;
	Point3f Origin;//�������õ�Cutter��halfDimension;
	Point3f HalfDimension;//�������õ�Cutter��halfDimension;
};


struct BoundBox
{
	Point3f Origin;
	float length;//x
	float width;//y
	float height;//z
};

struct Cutter
{
	Point3f SubOrigin;//��ʼ���꣬����̺߳ż���õ�Node������
	Point3f halfDimension;//Node�ĳߴ�
	Point3f CutterPos;//���ߵ�λ��
	Point3f CutterSize;//���߳ߴ�
	BoundBox CutterBox;//���ߵİ�Χ��
};

struct Device {
	Cutter* Device_Cutter;
	bool* D_buffer;
	Device(bool* B,Cutter* C) {
		Device_Cutter = C;
		D_buffer = B;
	}
};

struct RootOctree {
	Point3f origin;         //! The physical center of this node
	Point3f halfDimension;  //! Half the width/height/depth of this node ���صĳ���ߵİ�ֵ
	//Octree* children[8]; //! Pointers to child octants
	bool exist = true;//�ýڵ��Ƿ��ѱ��޳�
	void* bricks = nullptr;
};

struct Bricks {
	bool brick[BrickLength] = { false };
	//int RenderedBrick = 0;//���ڼ�¼�ж���brickλ�ڿ���Ⱦ�б��У�����Ⱦδ������ǰ���ܿ���ʮ�ֽӽ�BrickLength�������ᷢ���ϴ�ı䣻
	bool Rendered[BrickLength] = { false };//��¼��brick�Ƿ񱻼�����Ⱦ�б�
	void* ToVoxel;
	int sum = BrickLength;
	Bricks() {
	}
	Bricks(Octree* voxel) {
		ToVoxel = voxel;
		//for (int i = 0; i < 64; i++) {brick[i] = true;}
	}
};
struct CudaOctree {
	OctreeNode* HostOctree;
	Node* HostBuffer;
	Node* DeviceBuffer;
	BitBricks* HostBrick;
	BitBricks* DeviceBrick;
	Cutter* HostCutter;
	Cutter* DeviceCutter;
	CudaOctree(Node* Host, Node* Device, BitBricks* HBrick, BitBricks* DBrick, Cutter* Hcutter, Cutter* Dcutter, OctreeNode* HO) {
		HostBuffer = Host;
	    DeviceBuffer = Device;
		HostBrick  = HBrick;
		DeviceBrick = DBrick;
		HostCutter = Hcutter;
		DeviceCutter = Dcutter;
		HostOctree = HO;
	}
};




