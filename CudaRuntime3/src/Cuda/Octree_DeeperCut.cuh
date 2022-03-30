#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include"device_functions.h"
#include <iostream>
#include <Windows.h>
#include<stdio.h>
#include<vector>
#include<../src/Octree/Octree.h>
#include"../src/Octree/MyStruct.h">

using namespace std;


//__device__ bool IsIntersectForBrick(float3& origin, Cutter& Device_Cutter);

//__device__ bool IsInOfBrick(float3& OctreeVer, Cutter& Device_Cutter);

//__global__  void Device_Deeper_cut(bool* D_buffer, Cutter& Device_Cutter);

Device AllocMemory(const int BrickLength);
void Init_Data(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos, Point3f& CutterSize, Device& MyDevice);
void Init_Data512(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos, Point3f& CutterSize, Device& MyDevice);
void Init_Data4096(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos, Point3f& CutterSize, Device& MyDevice);


/*
{

	Cutter Host_Cutter;
	

	Host_Cutter.SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 3 / 4,
		curr->origin.y - curr->halfDimension.y * 3 / 4, curr->origin.z - curr->halfDimension.z * 3 / 4);
	Host_Cutter.halfDimension = Point3f(curr->halfDimension.x / 4, curr->halfDimension.y / 4, curr->halfDimension.z / 4);
	Host_Cutter.CutterSize = CutterSize;
	Host_Cutter.CutterPos = CutterPos;
	Host_Cutter.CutterBox = CutterBox;

	Bricks* bri = (Bricks*)curr->bricks;
	bool* D_buffer;//Device端数组
	cudaMalloc((void**)&(D_buffer), 64 * sizeof(bool));
	cudaMemcpy(D_buffer,bri->brick, 64 * sizeof(bool), cudaMemcpyHostToDevice);


	

	Cutter* Device_Cutter;
	cudaMalloc((void**)&(Device_Cutter), sizeof(Cutter));



	Device_Deeper_cut<<<1,64>>>(D_buffer, *Device_CutterBox, *Device_SubOrigin, *Device_halfDimension, *Device_CutterPos);


	cudaMemcpy(bri->brick,D_buffer, 64 * sizeof(bool), cudaMemcpyDeviceToHost);

}
*/

/*
Point3f* Device_SubOrigin;
Point3f* Device_halfDimension;
Point3f* Device_CutterPos;
Point3f* Device_CutterSize;
BoundBox* Device_CutterBox;

cudaMalloc((void**)&(Device_SubOrigin), sizeof(Point3f));
cudaMalloc((void**)&(Device_halfDimension), sizeof(Point3f));
cudaMalloc((void**)&(Device_CutterPos), sizeof(Point3f));
cudaMalloc((void**)&(Device_CutterBox), sizeof(BoundBox));

cudaMemcpy(Device_SubOrigin, &Host_Cutter.SubOrigin, sizeof(Point3f), cudaMemcpyHostToDevice);
cudaMemcpy(Device_halfDimension, &Host_halfDimension, sizeof(Point3f), cudaMemcpyHostToDevice);
cudaMemcpy(Device_CutterPos, &CutterPos, sizeof(Point3f), cudaMemcpyHostToDevice);
cudaMemcpy(Device_CutterBox, &CutterBox, sizeof(BoundBox), cudaMemcpyHostToDevice);
*/
