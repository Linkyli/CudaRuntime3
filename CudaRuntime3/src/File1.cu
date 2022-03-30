#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include"device_functions.h"
#include <iostream>
#include <Windows.h>
#include<stdio.h>
#include<vector>
#include"./ReadSTLfile/Point3f.h"
//#include"./Octree/Octree.h"

using namespace std;

#define ALIGN(x)	__align__(x)
#define ID_UNDEFI	0xFFFF
#define ID_UNDEFL	0xFFFFFFFF
#define ID_UNDEF64	0xFFFFFFFFFFFFFFFF
#define CHAN_UNDEF	255
#define MAX_CHANNEL  32

/*struct ALIGN(16) VDBNode {
    char		mLev;			// Level		Max = 255			1 byte
	char		mFlags;
	char		mPriority;
	char		pad;
	int3		mPos;			// Pos			Max = +/- 4 mil (linear space/range)	12 bytes
	int3		mValue;			// Value		Max = +8 mil		4 bytes
	float3		mVRange;
	__int64 mParent;		// Parent ID						8 bytes
	__int64		mChildList;		// Child List						8 bytes
	__int64		mMask;			// Bitmask starts - Must keep here, even if not USE_BITMASKS
};*/
/*__global__ void GetRenderedCube(Octree* test, )
{

}*/

struct Octree {
	float3 origin;         //! The physical center of this node
	float3 halfDimension;  //! Half the width/height/depth of this node 体素的长宽高的半值
	Octree* children[8]; //! Pointers to child octants
	int accuracy = 0;
	bool exist = true;//该节点是否已被剔除
	bool sub = false;//是否已被分割
};


struct BoundBox
{
	float3 Origin;
	float length;//x
	float width;//y
	float height;//z
};
/*Point3f CutterSize;//放在共享内存中
Point3f CutPosition;
BoundBox CutterBox;
*/
//在CPU计算得到包围盒，放在共享内存中
BoundBox GetCutterBoundBox(float3& Center, float3& CutterSize)//暂时不考虑刀具的旋转,输入刀具的位置和大小参数
{
	//暂时不考虑刀具的旋转；
	BoundBox box;
	box.Origin = Center;
	box.Origin.z = Center.z + (CutterSize.x - CutterSize.y) / 2;

	box.length = 2 * CutterSize.y;
	box.width = 2 * CutterSize.y;
	box.height = CutterSize.y + CutterSize.x;

	return box;
}
struct Myvector
{
	size_t buf_len_;//数组容量大小
	size_t cnt_top_;//数组已经装填的大小
	float3* buf_;
};
__device__  void Push(Myvector& obj, float3 target)
{
	printf("运行\n");

	if (obj.cnt_top_ + 1 == obj.buf_len_) {

		printf("增加容量\n");
		float3* temp;
		cudaMalloc((void**)&(temp), obj.buf_len_ * 1.5);
		//T* tmp = new T[buf_len_ * 1.5];
		obj.buf_len_ = obj.buf_len_ * 1.5;
		for (size_t i = 0; i < obj.buf_len_; ++i)
			temp[i] = obj.buf_[i];

		free(obj.buf_);
		obj.buf_ = temp;
	}

	obj.buf_[++obj.cnt_top_] = target;
	printf("%d号装填成功", obj.cnt_top_);
}

//在GPU中调用，判断刀具包围盒是否与体素相交
__device__ bool IsIntersect(Octree* Voxel, BoundBox& CutterBox){
	if (std::abs(Voxel->origin.x - CutterBox.Origin.x) < std::abs(Voxel->halfDimension.x + (CutterBox.length / 2))
		&& std::abs(Voxel->origin.y - CutterBox.Origin.y) < std::abs(Voxel->halfDimension.y + (CutterBox.width / 2))
		&& std::abs(Voxel->origin.z - CutterBox.Origin.z) < std::abs(Voxel->halfDimension.z + (CutterBox.height / 2))
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

//此处可以设置成八个点并行判断，设置同步，等待8个点判断完毕后在返回，输入该节点的指针，根据线程号来计算顶点
__global__  void IsIn(Octree* curr, int i, Point3f& CutPosition, Point3f& CutterSize) {

	float3 OctreeVer;
	OctreeVer.x = curr->origin.x - curr->halfDimension.x;
	OctreeVer.x = OctreeVer.x + (threadIdx.x & 4) / 2 * curr->halfDimension.x;

	OctreeVer.y = curr->origin.y - curr->halfDimension.y;
	OctreeVer.y = OctreeVer.y + (threadIdx.x & 2) * curr->halfDimension.y;

	OctreeVer.z = curr->origin.z - curr->halfDimension.z;
	OctreeVer.z = OctreeVer.z + (threadIdx.x & 1) * 2 * curr->halfDimension.z;

	//return OctreeVer;

	float X = abs(OctreeVer.x - CutPosition.x);
	float Y = abs(OctreeVer.y - CutPosition.y);
	float Z = (OctreeVer.z - CutPosition.z);

	//该顶点不在刀具内->需要被分割->curr->sub = true
	if (X > CutterSize.y || Y > CutterSize.y || Z > CutterSize.x || 
		(X * X + Y * Y + Z * Z) >= (CutterSize.y * CutterSize.y)) 
	{  curr->sub = true; }


	//该顶点在刀具内
	//curr->sub一旦为true，那么该节点必须被分割，经过8个节点的判断后仍然为false，说明该体素八个顶点都在刀具内，不需要被分割，需要被移除
	//curr->sub = false;
	//块内同步函数，同一block内所有线程执行至__syncthreads()处等待全部线程执行完毕后再继续
	//__syncthreads();
}



//串行处理8个点，输入当前要判断的顶点，输出它是否与刀具相交
__device__  bool IsIn_a(Octree* curr,int i, Point3f &CutPosition, Point3f& CutterSize) {
	
	float3 OctreeVer;
	OctreeVer.x = curr->origin.x - curr->halfDimension.x;
	OctreeVer.x = OctreeVer.x + (i & 4) / 2 * curr->halfDimension.x;

	OctreeVer.y = curr->origin.y - curr->halfDimension.y;
	OctreeVer.y = OctreeVer.y + (i & 2) * curr->halfDimension.y;

	OctreeVer.z = curr->origin.z - curr->halfDimension.z;
	OctreeVer.z = OctreeVer.z + (i & 1) * 2 * curr->halfDimension.z;

	float X = abs(OctreeVer.x - CutPosition.x);
	float Y = abs(OctreeVer.y - CutPosition.y);
	float Z = (OctreeVer.z - CutPosition.z);

	//该顶点不在刀具内->需要被分割->curr->sub = true
	if (X > CutterSize.y || Y > CutterSize.y || Z > CutterSize.x || 
		(X * X + Y * Y + Z * Z) >= (CutterSize.y * CutterSize.y)) 
	{   curr->sub = true;
		return false;
	}
	return true;

	//该顶点在刀具内
	//curr->sub一旦为true，那么该节点必须被分割，经过8个节点的判断后仍然为false，说明该体素八个顶点都在刀具内，不需要被分割，需要被移除
	//curr->sub = false;
	//块内同步函数，同一block内所有线程执行至__syncthreads()处等待全部线程执行完毕后再继续
	//__syncthreads();
}

//对节点进行判断，若需要分割则会创建8个线程来分别对8个子节点进行处理，通过线程号进行区别
__global__ void Cutter_Dynamic(Octree* GrandNode, BoundBox& CutterBox, Point3f& CutPosition, Point3f& CutterSize) {

	//为被分割出来的一个子节点分配显存。
	cudaMalloc((void**)&(GrandNode->children[threadIdx.x]), sizeof(Octree));
	//子节点的精细度+1
	GrandNode->children[threadIdx.x]->accuracy = GrandNode->accuracy + 1;

	if (IsIntersect(GrandNode->children[threadIdx.x], CutterBox)) {
		for (int i = 0; i <= 7; i++) {
			if (!IsIn_a(GrandNode->children[threadIdx.x],i, CutPosition, CutterSize)) {
				//有一个节点在外部 -> 相交 ->分割，或许存在误判，后期再改进
				if (GrandNode->children[threadIdx.x]->accuracy <= 3) {

					//分割出8个子节点的子节点
					Cutter_Dynamic <<< 1, 8 >>> (GrandNode->children[threadIdx.x],CutterBox, CutPosition,CutterSize);
					return;
				}
			}
		}
		//8个点都在内部
		GrandNode->children[threadIdx.x]->exist = false;
	}

}


//判断8个顶点是否在刀具内
//IsIn << <1, 8 >> > (GrandNode->children[threadIdx.x]);
//printf(" children[%d]->accuracy  = %d\n", threadIdx.x, test->children[threadIdx.x]->accuracy);

__global__ void TestOctree(int* ret, Octree* test)
{
	//ret[threadIdx.x] += 3;

	cudaMalloc((void**)&(test->children[threadIdx.x]) , sizeof(Octree));

	test->children[threadIdx.x]->accuracy = test->accuracy + 1;
	//test->children[threadIdx.x]->sub = true;

	//printf(" children[%d]->accuracy  = %d\n", threadIdx.x, test->children[threadIdx.x]->accuracy);

	if (test->children[threadIdx.x]->accuracy <= 1) {
		TestOctree <<< 1, 8 >> > (ret, test->children[threadIdx.x]);
	}

}

//printf("ֵ:%d", ret[threadIdx.x]);
//printf("from AplusThree function\n");
//printf(" Octree* test：%d  \n" ,test->accuracy);
//printf("threadIdx.x: %d \n", threadIdx.x);

__global__ void AplusB(int* ret, int a, int b, Octree* testB)
{
	ret[threadIdx.x] += a + b + threadIdx.x;
	Octree* test = testB;
	test->exist = false;
	test->accuracy = 1;

	TestOctree <<< 1, 8>>> (ret, test);
	//printf("ֵ:%d", ret[threadIdx.x]);
}
__device__ 	float3 GetOctreeVertex(int number, Octree* curr) {
	float3 OctreeVertex;
	OctreeVertex.x = curr->origin.x - curr->halfDimension.x;
	OctreeVertex.x = OctreeVertex.x + (number & 4) / 2 * curr->halfDimension.x;

	OctreeVertex.y = curr->origin.y - curr->halfDimension.y;
	OctreeVertex.y = OctreeVertex.y + (number & 2) * curr->halfDimension.y;

	OctreeVertex.z = curr->origin.z - curr->halfDimension.z;
	OctreeVertex.z = OctreeVertex.z + (number & 1) * 2 * curr->halfDimension.z;

	return OctreeVertex;
}
__device__ void IteraAddPoints(Octree* curr, Myvector& testVertex) {
	printf("添加点:\n");
	if (!curr->sub) {
		float3 temp;
		temp = GetOctreeVertex(0, curr);

		float3 halfDimension = float3(curr->halfDimension);

		Push(testVertex, temp); Push(testVertex, curr->halfDimension);
		//testVertex.push_back(temp); testVertex.push_back(curr->halfDimension);
		temp = GetOctreeVertex(7, curr);
		halfDimension.x *= (-1);
		halfDimension.y *= (-1);
		halfDimension.z *= (-1);
		Push(testVertex, temp); Push(testVertex, curr->halfDimension);
		//testVertex.push_back(temp); testVertex.push_back(curr->halfDimension * (-1));
	}
	else
	{
		//if (!curr->sub) return;//判断是否存在子节点
		for (auto child : curr->children) {
			if (child->exist) IteraAddPoints(child, testVertex);

		}
	}


}
__global__ void AddPoints(Octree* curr, Myvector& testVertex) {
	IteraAddPoints(curr, testVertex);
}




void test()
{
	int ret[5] = { 1,2,3,4,5 };
	int a[5] = { 0 };
	int* dev_ret;
	cudaError_t cudaStatus;
	cudaStatus = cudaMalloc((void**)&dev_ret, sizeof(int) * 5);
	cudaStatus = cudaMemcpy(dev_ret, ret, sizeof(int) * 5, cudaMemcpyHostToDevice);

	Octree* testB;

    cudaStatus = cudaMalloc((void**)&testB, sizeof(Octree));

	///Push(testVector,Mydim);
	
	AplusB <<< 1, 1 >>> (dev_ret, 10, 100,testB);

	Myvector testVector;
	cudaMalloc((void**)&(testVector.buf_), 10 * sizeof(float3));
	testVector.buf_len_ = 10;//
	testVector.cnt_top_ = 0;
	//float3 Mydim;
	//Push(testVector, Mydim);

	AddPoints << < 1, 1 >> > (testB, testVector);
	//cout << testB->accuracy << endl;

	cudaStatus = cudaMemcpy(a, dev_ret, sizeof(int) * 5, cudaMemcpyDeviceToHost);
	cudaDeviceSynchronize();


	for (int i = 0; i < 5; i++)
	{
		cout << "A+B = " << a[i] << endl;
	}


	cudaFree(dev_ret);
}


int main()
{
	cudaEvent_t start, stop;
	float elapseTime = 0;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);
	cudaEventRecord(start, 0);

	test();

	cudaEventRecord(stop, 0);
	cudaEventSynchronize(stop);
	cudaEventElapsedTime(&elapseTime, start, stop);
	cout << elapseTime << " ms" << endl;


	system("pause");
	return 0;

}