#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include"device_functions.h"
#include <iostream>
#include <Windows.h>
#include<stdio.h>
#include<vector>
#include<../src/Octree/Octree.h>
#include<../src/Octree/MyStruct.h>
//#include<../src/Octree/OpenGL_Render_Octree.h>

using namespace std;

__device__ bool IsIntersectForBrick(float3& origin, Cutter& Device_Cutter) {
	if (abs(origin.x - Device_Cutter.CutterBox.Origin.x) < abs(Device_Cutter.halfDimension.x + (Device_Cutter.CutterBox.length / 2))
		&& abs(origin.y - Device_Cutter.CutterBox.Origin.y) < abs(Device_Cutter.halfDimension.y + (Device_Cutter.CutterBox.width / 2))
		&& abs(origin.z - Device_Cutter.CutterBox.Origin.z) < abs(Device_Cutter.halfDimension.z + (Device_Cutter.CutterBox.height / 2))
		)
	{
		//cout << "相交" << endl;
		return true;
	}
	else
	{
		//cout << "不相交" << endl;
		return false;
	}
}

__device__ bool IsInOfBrick(float3& OctreeVer, Cutter& Device_Cutter) {
	float X = abs(OctreeVer.x - Device_Cutter.CutterPos.x);
	float Y = abs(OctreeVer.y - Device_Cutter.CutterPos.y);
	float Z = (OctreeVer.z - Device_Cutter.CutterPos.z);

	//该顶点不在刀具内->需要被分割->curr->sub = true
	if (X > Device_Cutter.CutterSize.y || Y > Device_Cutter.CutterSize.y || Z > Device_Cutter.CutterSize.x ||
		(X * X + Y * Y + Z * Z) >= (Device_Cutter.CutterSize.y * Device_Cutter.CutterSize.y))
	{
		return false;
	}

	return true;
}


__global__  void Device_Deeper_cut(bool* D_buffer, Cutter& Device_Cutter) {
	if (D_buffer[threadIdx.x]) return;

	float3 SubTemp;
	SubTemp.x = Device_Cutter.SubOrigin.x;
	SubTemp.y = Device_Cutter.SubOrigin.y;
	SubTemp.z = Device_Cutter.SubOrigin.z;

	SubTemp.x = SubTemp.x + (threadIdx.x & 3) * Device_Cutter.halfDimension.x / 2;
	SubTemp.y = SubTemp.y + (threadIdx.x & 12) / 4 * Device_Cutter.halfDimension.y / 2;
	SubTemp.z = SubTemp.z + (threadIdx.x & 48) / 16 * Device_Cutter.halfDimension.z / 2;

	if (IsIntersectForBrick(SubTemp, Device_Cutter)) {

		if (IsInOfBrick(SubTemp, Device_Cutter)) {
			D_buffer[threadIdx.x] = true;
		}
		//else {bri->brick[k] = true;}
	}
}



