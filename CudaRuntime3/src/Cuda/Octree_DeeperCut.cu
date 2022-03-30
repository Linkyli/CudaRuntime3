#include<../src/Cuda/Octree_DeeperCut.cuh>
#include<../src/Octree/Octree.h>
//#include<../src/Octree/OpenGL_Render_Octree.h>

using namespace std;

Device AllocMemory(const int BrickLength) {
	bool* D_buffer;//Device端数组
	cudaMalloc((void**)&(D_buffer), BrickLength * sizeof(bool));
	Cutter* Device_Cutter;
	cudaMalloc((void**)&(Device_Cutter), sizeof(Cutter));
	return Device(D_buffer, Device_Cutter);
	
}


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

__device__ float3 GetOctreeVertexForBrick(int number, float3& origin, Point3f& halfDimension) {
	float3 OctreeVertex;

	OctreeVertex.x = origin.x - halfDimension.x;
	OctreeVertex.x = OctreeVertex.x + (number & 4) / 2 * halfDimension.x;

	OctreeVertex.y = origin.y - halfDimension.y;
	OctreeVertex.y = OctreeVertex.y + (number & 2) * halfDimension.y;

	OctreeVertex.z = origin.z - halfDimension.z;
	OctreeVertex.z = OctreeVertex.z + (number & 1) * 2 * halfDimension.z;

	return OctreeVertex;
}


__global__  void Device_Deeper_cut(bool* D_buffer, Cutter& Device_Cutter) {
	if (D_buffer[threadIdx.x]) return;

	float3 SubTemp;
	float3 OctreeVertex;
	SubTemp.x = Device_Cutter.SubOrigin.x;
	SubTemp.y = Device_Cutter.SubOrigin.y;
	SubTemp.z = Device_Cutter.SubOrigin.z;

	SubTemp.x = SubTemp.x + (threadIdx.x & 3) * Device_Cutter.halfDimension.x * 2;
	SubTemp.y = SubTemp.y + (threadIdx.x & 12) / 4 * Device_Cutter.halfDimension.y * 2;
	SubTemp.z = SubTemp.z + (threadIdx.x & 48) / 16 * Device_Cutter.halfDimension.z * 2;

	if (IsIntersectForBrick(SubTemp, Device_Cutter)) {
		//printf("相交:\n");
		for (int i = 0; i <= 7; ++i) {
			OctreeVertex = GetOctreeVertexForBrick(i, SubTemp, Device_Cutter.halfDimension);
			if (IsInOfBrick(OctreeVertex, Device_Cutter)) {
				if (i == 7) {
					D_buffer[threadIdx.x] = true;
					break;
				}
				continue;
			}
		}
		//if (IsInOfBrick(SubTemp, Device_Cutter)) {D_buffer[threadIdx.x] = true;}
		//else {bri->brick[k] = true;}
	}
}

__global__  void Device_Deeper_cut512(bool* D_buffer, Cutter& Device_Cutter) {
	if (D_buffer[threadIdx.x]) return;

	float3 SubTemp;
	SubTemp.x = Device_Cutter.SubOrigin.x + (threadIdx.x & 7) * Device_Cutter.halfDimension.x * 2;
	SubTemp.y = Device_Cutter.SubOrigin.y + (threadIdx.x & 56) / 8 * Device_Cutter.halfDimension.y * 2;
	SubTemp.z = Device_Cutter.SubOrigin.z + (threadIdx.x & 448) / 64 * Device_Cutter.halfDimension.z * 2;
	/*
	SubTemp.x = SubTemp.x + (threadIdx.x & 7) * Device_Cutter.halfDimension.x * 2;
	SubTemp.y = SubTemp.y + (threadIdx.x & 56) / 8 * Device_Cutter.halfDimension.y * 2;
	SubTemp.z = SubTemp.z + (threadIdx.x & 448) / 64 * Device_Cutter.halfDimension.z * 2;
	*/
	if (IsIntersectForBrick(SubTemp, Device_Cutter)) {
		//printf("相交:\n");
		if (IsInOfBrick(SubTemp, Device_Cutter)) {
			D_buffer[threadIdx.x] = true;
		}
		//else {bri->brick[k] = true;}
	}
}

__global__  void Device_Deeper_cut4096(bool* D_buffer, Cutter& Device_Cutter) {
	int index = blockIdx.x * 256 + threadIdx.x;
	if (D_buffer[index]) return;

	float3 SubTemp;
	SubTemp.x = Device_Cutter.SubOrigin.x + (index & 15) * Device_Cutter.halfDimension.x * 2;
	SubTemp.y = Device_Cutter.SubOrigin.y + (index & 240) / 16 * Device_Cutter.halfDimension.y * 2;
	SubTemp.z = Device_Cutter.SubOrigin.z + (index & 3840) / 256 * Device_Cutter.halfDimension.z * 2;
	/*
	SubTemp.x = SubTemp.x + (blockIdx.x) * Device_Cutter.halfDimension.x * 2;
	SubTemp.y = SubTemp.y + (threadIdx.x & 15) * Device_Cutter.halfDimension.y * 2;
	SubTemp.z = SubTemp.z + (threadIdx.x & 240) / 16 * Device_Cutter.halfDimension.z * 2;

	//printf("index: %d SubTemp(%d,%d,%d)\n", index, blockIdx.x, (threadIdx.x & 15), (threadIdx.x & 240) / 16);
	
	SubTemp.x = SubTemp.x + (index & 15) * Device_Cutter.halfDimension.x * 2;
	SubTemp.y = SubTemp.y + (index & 240) / 16 * Device_Cutter.halfDimension.y * 2;
	SubTemp.z = SubTemp.z + (index & 3840) / 256 * Device_Cutter.halfDimension.z * 2;
	*/
	if (IsIntersectForBrick(SubTemp, Device_Cutter)) {
		//printf("相交:\n");
		if (IsInOfBrick(SubTemp, Device_Cutter)) {
			D_buffer[index] = true;
		}
		//else {bri->brick[k] = true;}
	}
}




void Init_Data(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos, Point3f& CutterSize, Device& MyDevice) {

	Cutter Host_Cutter;
	const int Length = 64;

	Host_Cutter.SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 3 / 4,
		curr->origin.y - curr->halfDimension.y * 3 / 4, curr->origin.z - curr->halfDimension.z * 3 / 4);
	Host_Cutter.halfDimension = Point3f(curr->halfDimension.x / 4, curr->halfDimension.y / 4, curr->halfDimension.z / 4);
	Host_Cutter.CutterSize = CutterSize;
	Host_Cutter.CutterPos = CutterPos;
	Host_Cutter.CutterBox = CutterBox;

	Bricks* bri = (Bricks*)curr->bricks;
	//bool* D_buffer;//Device端数组
	//cudaMalloc((void**)&(D_buffer), Length * sizeof(bool));
	//cudaMemcpy(D_buffer, bri->brick, Length * sizeof(bool), cudaMemcpyHostToDevice);

	cudaMemcpy(MyDevice.D_buffer, bri->brick, Length * sizeof(bool), cudaMemcpyHostToDevice);

	//Cutter* Device_Cutter;
	//cudaMalloc((void**)&(Device_Cutter), sizeof(Cutter));
	//cudaMemcpy(Device_Cutter, &Host_Cutter, sizeof(Cutter), cudaMemcpyHostToDevice);
	cudaMemcpy(MyDevice.Device_Cutter, &Host_Cutter, sizeof(Cutter), cudaMemcpyHostToDevice);

	Device_Deeper_cut << <1, Length >> > (MyDevice.D_buffer, *MyDevice.Device_Cutter);
	cudaDeviceSynchronize();
	cudaMemcpy(bri->brick, MyDevice.D_buffer, Length * sizeof(bool), cudaMemcpyDeviceToHost);

	//cudaFree(MyDevice.D_buffer);//释放内存
	//cudaFree(MyDevice.Device_Cutter);
}

void Init_Data512(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos, Point3f& CutterSize, Device& MyDevice) {

	Cutter Host_Cutter;
	const int Length = 512;

	Host_Cutter.SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 / 8,
		curr->origin.y - curr->halfDimension.y * 7 / 8, curr->origin.z - curr->halfDimension.z * 7 / 8);
	Host_Cutter.halfDimension = Point3f(curr->halfDimension.x / 8, curr->halfDimension.y / 8, curr->halfDimension.z / 8);
	Host_Cutter.CutterSize = CutterSize;
	Host_Cutter.CutterPos = CutterPos;
	Host_Cutter.CutterBox = CutterBox;

	Bricks* bri = (Bricks*)curr->bricks;
	bool* D_buffer;//Device端数组
	cudaMalloc((void**)&(D_buffer), Length * sizeof(bool));//分配GPU内存
	cudaMemcpy(D_buffer, bri->brick, Length * sizeof(bool), cudaMemcpyHostToDevice);//为GPU内存赋值

	Cutter* Device_Cutter;
	cudaMalloc((void**)&(Device_Cutter), sizeof(Cutter));
	cudaMemcpy(Device_Cutter, &Host_Cutter, sizeof(Cutter), cudaMemcpyHostToDevice);

	Device_Deeper_cut512 << <1, Length >> > (D_buffer, *Device_Cutter);
	cudaDeviceSynchronize();
	cudaMemcpy(bri->brick, D_buffer, Length * sizeof(bool), cudaMemcpyDeviceToHost);

	cudaFree(D_buffer);//释放GPU内存
	cudaFree(Device_Cutter);
}

void Init_Data4096(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos, Point3f& CutterSize, Device& MyDevice) {

	Cutter Host_Cutter;

	const int Length = 4096;

	Host_Cutter.SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 15 / 16,
		curr->origin.y - curr->halfDimension.y * 15 / 16, curr->origin.z - curr->halfDimension.z * 15 / 16);
	Host_Cutter.halfDimension = Point3f(curr->halfDimension.x / 16, curr->halfDimension.y / 16, curr->halfDimension.z / 16);

	Host_Cutter.CutterSize = CutterSize;
	Host_Cutter.CutterPos = CutterPos;
	Host_Cutter.CutterBox = CutterBox;

	Bricks* bri = (Bricks*)curr->bricks;
	//bool* D_buffer;//Device端数组
	//cudaMalloc((void**)&(D_buffer), Length * sizeof(bool));
	cudaMemcpy(MyDevice.D_buffer, bri->brick, Length * sizeof(bool), cudaMemcpyHostToDevice);

	//Cutter* Device_Cutter;
	//cudaMalloc((void**)&(Device_Cutter), sizeof(Cutter));
	cudaMemcpy(MyDevice.Device_Cutter, &Host_Cutter, sizeof(Cutter), cudaMemcpyHostToDevice);

	dim3 dimGrid = (16);
	dim3 dimBlock = (256);
	//Device_Deeper_cut << <dimGrid, dimBlock >> > (D_buffer, *Device_Cutter);
	Device_Deeper_cut4096 << <dimGrid, dimBlock >> > (MyDevice.D_buffer, *MyDevice.Device_Cutter);
	cudaDeviceSynchronize();
	cudaMemcpy(bri->brick, MyDevice.D_buffer, Length * sizeof(bool), cudaMemcpyDeviceToHost);

	//for (int i = 0; i < 64; i++) {cout << bri->brick[i];}

	//cout << endl;
	//cudaFree(D_buffer);
	//cudaFree(Device_Cutter);
}



