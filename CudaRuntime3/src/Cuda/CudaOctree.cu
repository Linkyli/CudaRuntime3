#include<../src/Cuda/Octree_DeeperCut.cuh>

using namespace std;



CudaOctree AllocMemoryForCudaOctreeForThreeLevel(Point3f& origin, Point3f& halfDimension) {

	OctreeNode* Root;
	Node* OctreeHostBuffer;
	Node* OctrerDevieBuffer;//Device端数组
	BitBricks* HostBrick;
	BitBricks* DeviceBrick;
	Cutter* HostCutter;
	Cutter* DeviceCutter;

	Root = (OctreeNode*)malloc(8 * sizeof(OctreeNode));
	OctreeHostBuffer = (Node*)malloc(8 * BrickLength * sizeof(Node));
	HostBrick = (BitBricks*)malloc(8 * BrickLength * sizeof(BitBricks));
	HostCutter = (Cutter*)malloc(sizeof(Cutter));

	cudaMalloc((void**)&(OctrerDevieBuffer), 8 * BrickLength * sizeof(Node));
	cudaMalloc((void**)&(DeviceBrick), 8 * BrickLength * sizeof(BitBricks));
	cudaMalloc((void**)&(DeviceCutter), sizeof(Cutter));

	Point3f NodeSubOrigin = Point3f(origin.x - halfDimension.x * 1 / 2,
		origin.y - halfDimension.y * 1 / 2, origin.z - halfDimension.z * 1 / 2);
	Point3f NodeHalfDimension = Point3f(halfDimension.x / 2, halfDimension.y / 2, halfDimension.z / 2);
	for (int i = 0; i < 8; i++) {
		Root[i].Nodes = &OctreeHostBuffer[i * BrickLength];
		Root[i].Origin.x = NodeSubOrigin.x + (i & 4) / 2 * NodeHalfDimension.x;
		Root[i].Origin.y = NodeSubOrigin.y + (i & 2) * NodeHalfDimension.y;
		Root[i].Origin.z = NodeSubOrigin.z + (i & 1) * 2 * NodeHalfDimension.z;
		Root[i].HalfDimension = NodeHalfDimension;
	}

	Point3f SubOrigin = Point3f(origin.x - halfDimension.x * 15 / 16,
		origin.y - halfDimension.y * 15 / 16, origin.z - halfDimension.z * 15 / 16);

	Point3f HalfDimension = Point3f(halfDimension.x / 32, halfDimension.y / 32, halfDimension.z / 32);//每个Node的尺寸
	HostCutter->SubOrigin = SubOrigin;
	HostCutter->halfDimension = HalfDimension;

	return CudaOctree(OctreeHostBuffer, OctrerDevieBuffer, HostBrick, DeviceBrick, HostCutter, DeviceCutter, Root);
}

CudaOctree AllocMemoryForCudaOctree(Point3f& origin, Point3f& halfDimension) {

	Node* OctreeHostBuffer;
	Node* OctrerDevieBuffer;//Device端数组
	BitBricks* HostBrick;
	BitBricks* DeviceBrick;
	Cutter* HostCutter;
	Cutter* DeviceCutter;

	OctreeHostBuffer = (Node*)malloc(BrickLength * sizeof(Node));
	HostBrick = (BitBricks*)malloc(BrickLength * sizeof(BitBricks));
	HostCutter = (Cutter*)malloc(sizeof(Cutter));

	cudaMalloc((void**)&(OctrerDevieBuffer), BrickLength * sizeof(Node));
	cudaMalloc((void**)&(DeviceBrick), BrickLength * sizeof(BitBricks));
	cudaMalloc((void**)&(DeviceCutter), sizeof(Cutter));

	Point3f SubOrigin = Point3f(origin.x - halfDimension.x * 15 / 16,
		origin.y - halfDimension.y * 15 / 16, origin.z - halfDimension.z * 15 / 16);

	Point3f HalfDimension = Point3f(halfDimension.x / 16, halfDimension.y / 16, halfDimension.z / 16);
	HostCutter->SubOrigin = SubOrigin;
	HostCutter->halfDimension = HalfDimension;

	return CudaOctree(OctreeHostBuffer, OctrerDevieBuffer, HostBrick, DeviceBrick, HostCutter, DeviceCutter,nullptr);
}

__device__ bool IsIntersectForCuda(float3& origin, Point3f& halfDimension, Cutter* Device_Cutter) {
	if (abs(origin.x - Device_Cutter->CutterBox.Origin.x) < abs(halfDimension.x + (Device_Cutter->CutterBox.length / 2))
		&& abs(origin.y - Device_Cutter->CutterBox.Origin.y) < abs(halfDimension.y + (Device_Cutter->CutterBox.width / 2))
		&& abs(origin.z - Device_Cutter->CutterBox.Origin.z) < abs(halfDimension.z + (Device_Cutter->CutterBox.height / 2))
		)
	{
		//cout << "相交" << endl
		return true;
	}
	else
	{
		//cout << "不相交" << endl;
		return false;
	}
}

__device__ bool IsIntersectForCudaBrick(float3& origin, float3& halfDimension, Cutter* Device_Cutter) {
	if (abs(origin.x - Device_Cutter->CutterBox.Origin.x) < abs(halfDimension.x + (Device_Cutter->CutterBox.length / 2))
		&& abs(origin.y - Device_Cutter->CutterBox.Origin.y) < abs(halfDimension.y + (Device_Cutter->CutterBox.width / 2))
		&& abs(origin.z - Device_Cutter->CutterBox.Origin.z) < abs(halfDimension.z + (Device_Cutter->CutterBox.height / 2))
		)
	{
		//cout << "相交" << endl
		return true;
	}
	else
	{
		//cout << "不相交" << endl;
		return false;
	}
}
__device__ bool IsInOfCudaNode(float3& OctreeVer, Cutter& Device_Cutter) {
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

__device__ float3 GetOctreeVertexForCuda(int number, float3& origin, Point3f& halfDimension) {
	float3 OctreeVertex;

	OctreeVertex.x = origin.x - halfDimension.x;
	OctreeVertex.x = OctreeVertex.x + (number & 4) / 2 * halfDimension.x;

	OctreeVertex.y = origin.y - halfDimension.y;
	OctreeVertex.y = OctreeVertex.y + (number & 2) * halfDimension.y;

	OctreeVertex.z = origin.z - halfDimension.z;
	OctreeVertex.z = OctreeVertex.z + (number & 1) * 2 * halfDimension.z;

	return OctreeVertex;
}

__device__ float3 GetBrickVertexForCuda(int number, float3& origin, float3& halfDimension) {
	float3 OctreeVertex;

	OctreeVertex.x = origin.x - halfDimension.x;
	OctreeVertex.x = OctreeVertex.x + (number & 4) / 2 * halfDimension.x;

	OctreeVertex.y = origin.y - halfDimension.y;
	OctreeVertex.y = OctreeVertex.y + (number & 2) * halfDimension.y;

	OctreeVertex.z = origin.z - halfDimension.z;
	OctreeVertex.z = OctreeVertex.z + (number & 1) * 2 * halfDimension.z;

	return OctreeVertex;
}

__global__ void DeeperCutForCuda(float3 SubOrigin, float3 halfDimension, int NodeIndex, BitBricks* DeviceBricks, Cutter* Device_Cutter) {
	
	//int index = threadIdx.x ;
	int RealIndex;
	float3 SubTemp;
	int Forward;

	BYTE mask;
	//BYTE temp = DeviceBricks[NodeIndex].brick[BrickIndex];
	//mask = mask >> 4;
	//printf("%d号的index：%d\n", NodeIndex, RealIndex);
	for (int i = 0; i < 8;i++) {
	
		RealIndex = threadIdx.x * 8 + i;
		Forward =RealIndex % 8;
		mask = 1;//使用或|,当所在bit位置为1时则代表已经被切削
		mask = mask << i;

		SubTemp.x = SubOrigin.x + (RealIndex & 15) * halfDimension.x * 2;
		SubTemp.y = SubOrigin.y + (RealIndex & 240) / 16 * halfDimension.y * 2;
		SubTemp.z = SubOrigin.z + (RealIndex & 3840) / 256 * halfDimension.z * 2;

	   if (IsInOfCudaNode(SubTemp, *Device_Cutter)) {
		   DeviceBricks[NodeIndex].brick[threadIdx.x] = (DeviceBricks[NodeIndex].brick[threadIdx.x] | mask);
	   }
	}

}

__global__ void DeeperCutForCuda2(float3 SubOrigin, float3 halfDimension, int NodeIndex, BitBricks* DeviceBricks, Cutter* Device_Cutter) {

	int index = blockIdx.x * 256 + threadIdx.x;
	int RealIndex;
	float3 SubTemp;
	int Forward;

	BYTE mask;
	//BYTE temp = DeviceBricks[NodeIndex].brick[BrickIndex];
	//mask = mask >> 4;
	//printf("%d号的index：%d\n", NodeIndex, RealIndex);
	for (int i = 0; i < 8; i++) {

		RealIndex = index * 8 + i;
		Forward = RealIndex % 8;
		mask = 1;//使用或|,当所在bit位置为1时则代表已经被切削
		mask = mask << i;

		SubTemp.x = SubOrigin.x + (RealIndex & 31) * halfDimension.x * 2;
		SubTemp.y = SubOrigin.y + (RealIndex & 992) / 32 * halfDimension.y * 2;
		SubTemp.z = SubOrigin.z + (RealIndex & 31744) / 1024 * halfDimension.z * 2;

		if (IsInOfCudaNode(SubTemp, *Device_Cutter)) {
			DeviceBricks[NodeIndex].brick[index] = (DeviceBricks[NodeIndex].brick[index] | mask);
		}
	}

}


__global__  void DynamicCut(Node* OctreeDeviceBuffer, BitBricks* DeviceBricks, Cutter* CurrCutter,Point3f& SubOrigin) {

	int index = blockIdx.x * 256 + threadIdx.x;
	
	if (OctreeDeviceBuffer[index].exist) return;//true代表已经被切削掉

	float3 OctreeVertex;
	float3 SubTemp;
	SubTemp.x = SubOrigin.x + (index & 15) * CurrCutter->halfDimension.x * 2;        //CurrCutter->
	SubTemp.y = SubOrigin.y + (index & 240) / 16 * CurrCutter->halfDimension.x * 2;  //CurrCutter->
	SubTemp.z = SubOrigin.z + (index & 3840) / 256 * CurrCutter->halfDimension.x * 2;//CurrCutter->

	if (IsIntersectForCuda(SubTemp, CurrCutter->halfDimension, CurrCutter)) {
		//printf("相交:\n");
		for (int i = 0; i <= 7; ++i) {
			OctreeVertex = GetOctreeVertexForCuda(i, SubTemp, CurrCutter->halfDimension);
			if (IsInOfCudaNode(OctreeVertex, *CurrCutter)) {
				if (i == 7) {
					OctreeDeviceBuffer[index].exist = true;
					return;
				}
				continue;
			}
			else {//创建线程执行更深层次的切削
				OctreeDeviceBuffer[index].Sub = true;
				float3 BrickOrigin;
				float3 BrickHalfDimension;

				BrickHalfDimension.x = CurrCutter->halfDimension.x / 16;
				BrickHalfDimension.y = CurrCutter->halfDimension.y / 16;
				BrickHalfDimension.z = CurrCutter->halfDimension.z / 16;

				BrickOrigin.x = SubTemp.x - CurrCutter->halfDimension.x * 15 / 16;
				BrickOrigin.y = SubTemp.y - CurrCutter->halfDimension.y * 15 / 16;
				BrickOrigin.z = SubTemp.z - CurrCutter->halfDimension.z * 15 / 16;
				dim3 dimGrid = (1);
				dim3 dimBlock = (512);
				//printf("调用深度切削");
				DeeperCutForCuda << <dimGrid, dimBlock >> > (BrickOrigin, BrickHalfDimension, index, DeviceBricks, CurrCutter);
				break;
			}
		}
		//if (IsInOfBrick(SubTemp, Device_Cutter)) {D_buffer[threadIdx.x] = true;}
		//else {bri->brick[k] = true;}
	}

}

__global__  void DynamicCut2(Node* OctreeDeviceBuffer, BitBricks* DeviceBricks, Cutter* CurrCutter, Point3f& SubOrigin) {

	int index = blockIdx.x * 256 + threadIdx.x;

	if (OctreeDeviceBuffer[index].exist) return;//true代表已经被切削掉

	float3 OctreeVertex;
	float3 SubTemp;
	SubTemp.x = CurrCutter->SubOrigin.x + (index & 15) * CurrCutter->halfDimension.x * 2;        //CurrCutter->
	SubTemp.y = CurrCutter->SubOrigin.y + (index & 240) / 16 * CurrCutter->halfDimension.x * 2;  //CurrCutter->
	SubTemp.z = CurrCutter->SubOrigin.z + (index & 3840) / 256 * CurrCutter->halfDimension.x * 2;//CurrCutter->

	if (IsIntersectForCuda(SubTemp, CurrCutter->halfDimension, CurrCutter)) {
		//printf("相交:\n");
		for (int i = 0; i <= 7; ++i) {
			OctreeVertex = GetOctreeVertexForCuda(i, SubTemp, CurrCutter->halfDimension);
			if (IsInOfCudaNode(OctreeVertex, *CurrCutter)) {
				if (i == 7) {
					OctreeDeviceBuffer[index].exist = true;
					return;
				}
				continue;
			}
			else {//创建线程执行更深层次的切削
				OctreeDeviceBuffer[index].Sub = true;
				float3 BrickOrigin;
				float3 BrickHalfDimension;

				BrickHalfDimension.x = CurrCutter->halfDimension.x / 32;
				BrickHalfDimension.y = CurrCutter->halfDimension.y / 32;
				BrickHalfDimension.z = CurrCutter->halfDimension.z / 32;

				BrickOrigin.x = SubTemp.x - CurrCutter->halfDimension.x * 31 / 32;
				BrickOrigin.y = SubTemp.y - CurrCutter->halfDimension.y * 31 / 32;
				BrickOrigin.z = SubTemp.z - CurrCutter->halfDimension.z * 31 / 32;
				dim3 dimGrid = (16);
				dim3 dimBlock = (256);
				//printf("调用深度切削");
				DeeperCutForCuda2 << <dimGrid, dimBlock >> > (BrickOrigin, BrickHalfDimension, index, DeviceBricks, CurrCutter);
				break;
			}
		}
		//if (IsInOfBrick(SubTemp, Device_Cutter)) {D_buffer[threadIdx.x] = true;}
		//else {bri->brick[k] = true;}
	}

}



Point3f GetOctreeVertexForNode(int i,int number, CudaOctree& CudaBuffer) {
	Point3f OctreeVertex;

	OctreeVertex.x = CudaBuffer.HostOctree[i].Origin.x - CudaBuffer.HostOctree[i].HalfDimension.x;
	OctreeVertex.x = OctreeVertex.x + (number & 4) / 2 * CudaBuffer.HostOctree[i].HalfDimension.x;

	OctreeVertex.y = CudaBuffer.HostOctree[i].Origin.y - CudaBuffer.HostOctree[i].HalfDimension.y;
	OctreeVertex.y = OctreeVertex.y + (number & 2) * CudaBuffer.HostOctree[i].HalfDimension.y;

	OctreeVertex.z = CudaBuffer.HostOctree[i].Origin.z - CudaBuffer.HostOctree[i].HalfDimension.z;
	OctreeVertex.z = OctreeVertex.z + (number & 1) * 2 * CudaBuffer.HostOctree[i].HalfDimension.z;

	return OctreeVertex;
}

bool IsIntersectForNode(int i,CudaOctree& CudaBuffer, BoundBox& Cutter) {
	if (std::abs(CudaBuffer.HostOctree[i].Origin.x - Cutter.Origin.x) < std::abs(CudaBuffer.HostOctree[i].HalfDimension.x + (Cutter.length / 2))
		&& std::abs(CudaBuffer.HostOctree[i].Origin.y - Cutter.Origin.y) < std::abs(CudaBuffer.HostOctree[i].HalfDimension.y + (Cutter.width / 2))
		&& std::abs(CudaBuffer.HostOctree[i].Origin.z - Cutter.Origin.z) < std::abs(CudaBuffer.HostOctree[i].HalfDimension.z + (Cutter.height / 2))
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
void PrepareCut(CudaOctree& CudaBuffer, Point3f& CutterPos, Point3f& CutterSize, BoundBox& CutterBox) {
	
	//初始化刀具等信息
	CudaBuffer.HostCutter->CutterBox = CutterBox;
	CudaBuffer.HostCutter->CutterPos = CutterPos;
	CudaBuffer.HostCutter->CutterSize = CutterSize;
	cudaMemcpy(CudaBuffer.DeviceCutter, CudaBuffer.HostCutter, sizeof(Cutter), cudaMemcpyHostToDevice);
	
	//创建4096个线程
	dim3 dimGrid = (16);
	dim3 dimBlock = (256);
	DynamicCut2 << <dimGrid, dimBlock >> > (CudaBuffer.DeviceBuffer, CudaBuffer.DeviceBrick, CudaBuffer.DeviceCutter, CudaBuffer.DeviceCutter->SubOrigin);
	/*
	cudaDeviceSynchronize();
	//将Node数组的信息复制返回给Host端
	cudaMemcpy(CudaBuffer.HostBuffer, CudaBuffer.DeviceBuffer, BrickLength * sizeof(Node), cudaMemcpyDeviceToHost);
	//将brick数组的信息复制返回给host端
	cudaMemcpy(CudaBuffer.HostBrick, CudaBuffer.DeviceBrick, BrickLength * sizeof(BitBricks), cudaMemcpyDeviceToHost);
	*/
}

void PrepareCutForThreeLevel(CudaOctree& CudaBuffer, Point3f& CutterPos, Point3f& CutterSize, BoundBox& CutterBox) {

	//初始化刀具等信息
	CudaBuffer.HostCutter->CutterBox = CutterBox;
	CudaBuffer.HostCutter->CutterPos = CutterPos;
	CudaBuffer.HostCutter->CutterSize = CutterSize;
	cudaMemcpy(CudaBuffer.DeviceCutter, CudaBuffer.HostCutter, sizeof(Cutter), cudaMemcpyHostToDevice);

	//创建4096个线程
	dim3 dimGrid = (16);
	dim3 dimBlock = (256);

	Point3f OctreeVertex;
	
	for (int i = 0; i < 8; i++) {
		if (IsIntersectForNode(i, CudaBuffer, CutterBox)) {
			DynamicCut<<<dimGrid,dimBlock>>>(&CudaBuffer.DeviceBuffer[i * 4096],& CudaBuffer.DeviceBrick[i * 4096], CudaBuffer.DeviceCutter, CudaBuffer.HostOctree[i].Origin);
		}
	}


	
	/*
	cudaDeviceSynchronize();
	//将Node数组的信息复制返回给Host端
	cudaMemcpy(CudaBuffer.HostBuffer, CudaBuffer.DeviceBuffer, BrickLength * sizeof(Node), cudaMemcpyDeviceToHost);
	//将brick数组的信息复制返回给host端
	cudaMemcpy(CudaBuffer.HostBrick, CudaBuffer.DeviceBrick, BrickLength * sizeof(BitBricks), cudaMemcpyDeviceToHost);
	*/
}


/*
if (IsIntersectForCudaBrick(SubTemp, halfDimension, Device_Cutter)) {
	for (int i = 0; i < 8; i++) {
		OctreeVertex = GetBrickVertexForCuda(i, SubTemp, halfDimension);
		if (IsInOfCudaNode(SubTemp, *Device_Cutter)) {
			if (i == 7) {
				DeviceBricks[NodeIndex].brick[BrickIndex] = DeviceBricks[NodeIndex].brick[BrickIndex] | mask;
			}
		}
		else {
			break;
		}
	}

}
*/

