#pragma once
#include<../src/Octree/Octree.h>

using namespace std;




struct BitBricks {
	BYTE brick[BrickLength] = {0};
	//bool brick[4096];
	//long long int brick[128];
};

struct Node {
	bool exist;//标记Node节点是否存在
	bool Sub;//标记Node节点是否已被分割
};
struct OctreeNode {
	Node* Nodes;
	Point3f Origin;//对其计算得到Cutter的halfDimension;
	Point3f HalfDimension;//对其计算得到Cutter的halfDimension;
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
	Point3f SubOrigin;//初始坐标，配合线程号计算得到Node的坐标
	Point3f halfDimension;//Node的尺寸
	Point3f CutterPos;//刀具的位置
	Point3f CutterSize;//刀具尺寸
	BoundBox CutterBox;//刀具的包围盒
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
	Point3f halfDimension;  //! Half the width/height/depth of this node 体素的长宽高的半值
	//Octree* children[8]; //! Pointers to child octants
	bool exist = true;//该节点是否已被剔除
	void* bricks = nullptr;
};

struct Bricks {
	bool brick[BrickLength] = { false };
	//int RenderedBrick = 0;//用于记录有多少brick位于可渲染列表中，在渲染未被整理前它很可能十分接近BrickLength，整理后会发生较大改变；
	bool Rendered[BrickLength] = { false };//记录该brick是否被加入渲染列表；
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




