
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include"device_functions.h"
#include <iostream>
#include <Windows.h>
#include<stdio.h>
#include<vector>
//#include"./ReadSTLfile/Point3f.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>
//#include <learnopengl/camera.h>
#include"./OpenGL/Mycamera.h"
//#include <learnopengl/model.h>
#include<learnopengl/filesystem.h>

using namespace std;

#define ALIGN(x)	__align__(x)
#define ID_UNDEFI	0xFFFF
#define ID_UNDEFL	0xFFFFFFFF
#define ID_UNDEF64	0xFFFFFFFFFFFFFFFF
#define CHAN_UNDEF	255
#define MAX_CHANNEL  32

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

unsigned int SCR_WIDTH = 1200;
unsigned int SCR_HEIGHT = 900;

// camera
Mycamera camera = Mycamera(glm::vec3(30.0f, 30.0f, 80.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 lightPos = glm::vec3(12.0f, 10.0f, 20.0f);

struct Point3f {
	float x;
	float y;
	float z;
};

/*
struct Octree {
	Point3f origin;         //! The physical center of this node
	Point3f halfDimension;  //! Half the width/height/depth of this node 体素的长宽高的半值
	Octree* children[8]; //! Pointers to child octants
	int accuracy = 0;
	bool exist = true;//该节点是否存在;
	bool sub = false;//是否已被分割

	Octree(Point3f O, Point3f HD) {
		origin = O;
		halfDimension = HD;
		for (int i = 0; i <= 7; ++i) {
			children[i] = nullptr;
		}

		exist = true;
		sub = false; 
	}

};
*/

struct Octree {
	Point3f origin;         //! The physical center of this node
	Point3f halfDimension;  //! Half the width/height/depth of this node 体素的长宽高的半值
	Octree* children[8]; //! Pointers to child octants
	int accuracy = 0;
	bool exist = true;//该节点是否存在;
	bool sub = false;//是否已被分割
	Octree(Point3f O, Point3f HD) {
		origin = O;
		halfDimension = HD;

		for (int i = 0; i <= 7; ++i) {
			children[i] = nullptr;
		}
		exist = true;
		sub = false;
	}

};

struct BoundBox
{
	Point3f Origin;
	float length;//x
	float width;//y
	float height;//z
};

struct Myvector
{
	int buf_len_;//数组容量大小
	int cnt_top_;//数组已经装填的大小
	//Point3f* buf_;
};


BoundBox GetCutterBoundBox(Point3f& Center, Point3f& CutterSize);
Point3f GetCutterPos(float T, float t, Point3f& InitCutPosition, Point3f& origin, Point3f& halfDimension);

__device__  void Push(Myvector* obj, Point3f target, Point3f* buffer)
{
	if (obj->cnt_top_ + 1 == obj->buf_len_) {

		//增加容量
		Point3f* temp;
		cudaMalloc((void**)&(temp), obj->buf_len_ * 2 * sizeof(Point3f));
		obj->buf_len_ = obj->buf_len_ * 2;
		//cudaMemcpyAsync(void *dst, const void *src, size_t count, enum cudaMemcpyKind kind);
		//cudaMemcpy为host函数，不能在此使用
		cudaMemcpyAsync(temp, buffer, (obj->cnt_top_ + 1)* sizeof(Point3f), cudaMemcpyDeviceToDevice);
		cudaFree(buffer);
		buffer = temp;
	}
	buffer[obj->cnt_top_] = target;

	obj->cnt_top_++;

	//printf("%d号装填成功", obj->cnt_top_);
	/*for (size_t i = 0; i < obj.buf_len_; ++i)
	temp[i] = obj.buf_[i];
*/
}

//在GPU中调用，判断刀具包围盒是否与体素相交
__device__ bool IsIntersect(Octree* Voxel, BoundBox& CutterBox) {
	//printf("调用IsIntersect：");
	if (std::abs(Voxel->origin.x - CutterBox.Origin.x) < std::abs(Voxel->halfDimension.x + (CutterBox.length / 2))
		&& std::abs(Voxel->origin.y - CutterBox.Origin.y) < std::abs(Voxel->halfDimension.y + (CutterBox.width / 2))
		&& std::abs(Voxel->origin.z - CutterBox.Origin.z) < std::abs(Voxel->halfDimension.z + (CutterBox.height / 2))
		)
	{
		//printf("相交\n");
		return true;
	}
	else
	{
		//printf("不相交\n");
		return false;
	}

}

//此处可以设置成八个点并行判断，设置同步，等待8个点判断完毕后在返回，输入该节点的指针，根据线程号来计算顶点
__global__  void IsIn(Octree* curr, int i, Point3f& CutPosition, Point3f& CutterSize) {

	Point3f OctreeVer;
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
	{
		curr->sub = true;
	}


	//该顶点在刀具内
	//curr->sub一旦为true，那么该节点必须被分割，经过8个节点的判断后仍然为false，说明该体素八个顶点都在刀具内，不需要被分割，需要被移除
	//curr->sub = false;
	//块内同步函数，同一block内所有线程执行至__syncthreads()处等待全部线程执行完毕后再继续
	//__syncthreads();
}

//串行处理8个点，输入当前要判断的顶点，输出它是否与刀具相交
__device__  bool IsIn_a(Octree* curr, int i, Point3f& CutPosition, Point3f& CutterSize) {

	Point3f OctreeVer;
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
	{
		curr->sub = true;
		return false;
	}
	return true;

	//该顶点在刀具内
	//curr->sub一旦为true，那么该节点必须被分割，经过8个节点的判断后仍然为false，说明该体素八个顶点都在刀具内，不需要被分割，需要被移除
	//curr->sub = false;
	//块内同步函数，同一block内所有线程执行至__syncthreads()处等待全部线程执行完毕后再继续
	//__syncthreads();
}

__device__  void InitParent(int index, Octree* GrandNode) {

	printf("分配内存初始化\n");
	cudaMalloc((void**)&(GrandNode->children[index]), sizeof(Octree));
	//cudaMalloc((void**)&(GrandNode->children[index]->children), 64 * sizeof(Octree));
	GrandNode->children[index]->accuracy = GrandNode->accuracy + 1;

	//printf("allocMemeoty,accuracy:%d\n", GrandNode->children[threadIdx.x]->accuracy);
	//计算子节点的原点
	GrandNode->children[index]->origin.x = GrandNode->origin.x - GrandNode->halfDimension.x * 3 / 4;
	GrandNode->children[index]->origin.x = GrandNode->children[index]->origin.x + ((threadIdx.x & 3) * GrandNode->halfDimension.x) / 2;

	GrandNode->children[index]->origin.y = GrandNode->origin.y - GrandNode->halfDimension.y * 3 / 4;
	GrandNode->children[index]->origin.y = GrandNode->children[index]->origin.y + ((threadIdx.x & 12) / 4 * GrandNode->halfDimension.y) / 2;

	GrandNode->children[index]->origin.z = GrandNode->origin.z - GrandNode->halfDimension.z * 3 / 4;
	GrandNode->children[index]->origin.z = GrandNode->children[index]->origin.z + ((threadIdx.x & 48) / 16 * GrandNode->halfDimension.z) / 2;

	//计算尺寸
	GrandNode->children[index]->halfDimension.x = GrandNode->halfDimension.x / 4;
	GrandNode->children[index]->halfDimension.y = GrandNode->halfDimension.y / 4;
	GrandNode->children[index]->halfDimension.z = GrandNode->halfDimension.z / 4;

	//设定初始参数
	GrandNode->children[index]->exist = true;
	GrandNode->children[index]->sub = false;

	for (int i = 0; i <= 63; ++i) {
		GrandNode->children[index]->children[i] = nullptr;
	}
}
//对节点进行判断，若需要分割则会创建8个线程来分别对8个子节点进行处理，通过线程号进行区别
__global__ void Cutter_Dynamic(Octree* GrandNode, BoundBox& CutterBox, Point3f& CutPosition, Point3f& CutterSize) {

	//if (GrandNode->accuracy > 4) return;
	//为被分割出来的一个子节点分配显存。
	//if(!alloc)
	if ((GrandNode->children[threadIdx.x]) == nullptr)
	{
		cudaMalloc((void**)&(GrandNode->children[threadIdx.x]), sizeof(Octree));
		GrandNode->children[threadIdx.x]->accuracy = GrandNode->accuracy + 1;
		//printf("allocMemeoty,accuracy:%d\n", GrandNode->children[threadIdx.x]->accuracy);
		//计算子节点的原点
		GrandNode->children[threadIdx.x]->origin.x = GrandNode->origin.x - GrandNode->halfDimension.x / 2;
		GrandNode->children[threadIdx.x]->origin.x = GrandNode->children[threadIdx.x]->origin.x + (threadIdx.x & 1) * GrandNode->halfDimension.x;
		GrandNode->children[threadIdx.x]->origin.y = GrandNode->origin.y - GrandNode->halfDimension.y / 2;
		GrandNode->children[threadIdx.x]->origin.y = GrandNode->children[threadIdx.x]->origin.y + (threadIdx.x & 2) * GrandNode->halfDimension.y / 2;
		GrandNode->children[threadIdx.x]->origin.z = GrandNode->origin.z - GrandNode->halfDimension.z / 2;
		GrandNode->children[threadIdx.x]->origin.z = GrandNode->children[threadIdx.x]->origin.z + (threadIdx.x & 4) * GrandNode->halfDimension.z / 4;

		//计算尺寸
		GrandNode->children[threadIdx.x]->halfDimension.x = GrandNode->halfDimension.x / 2;
		GrandNode->children[threadIdx.x]->halfDimension.y = GrandNode->halfDimension.y / 2;
		GrandNode->children[threadIdx.x]->halfDimension.z = GrandNode->halfDimension.z / 2;

		//设定初始参数
		GrandNode->children[threadIdx.x]->exist = true;
		GrandNode->children[threadIdx.x]->sub = false;

		GrandNode->children[threadIdx.x]->children[0] = nullptr;
		GrandNode->children[threadIdx.x]->children[1] = nullptr;
		GrandNode->children[threadIdx.x]->children[2] = nullptr;
		GrandNode->children[threadIdx.x]->children[3] = nullptr;
		GrandNode->children[threadIdx.x]->children[4] = nullptr;
		GrandNode->children[threadIdx.x]->children[5] = nullptr;
		GrandNode->children[threadIdx.x]->children[6] = nullptr;
		GrandNode->children[threadIdx.x]->children[7] = nullptr;
		
	}
	//子节点的精细度+1
	//*printf("精度为%d", GrandNode->children[threadIdx.x]->accuracy);
	//GrandNode->children[threadIdx.x]->children = nullptr;
	if (GrandNode->children[threadIdx.x]->accuracy >= 7 || !(GrandNode->children[threadIdx.x]->exist)) return;

	if (IsIntersect(GrandNode->children[threadIdx.x], CutterBox)) {
		//printf("判断完毕\n");
		for (int i = 0; i <= 7; i++) {

			if (!IsIn_a(GrandNode->children[threadIdx.x], i, CutPosition, CutterSize) )  {
				
				//分割出8个子节点的子节点
				//*printf("分割\n");
				bool alloc = GrandNode->children[threadIdx.x]->sub;//判断其是否已经被分内存
				GrandNode->children[threadIdx.x]->sub = true;
				//printf("分割:精度%d", GrandNode->children[threadIdx.x]->accuracy);
				Cutter_Dynamic << < 1, 8 >> > (GrandNode->children[threadIdx.x], CutterBox, CutPosition, CutterSize);
				return;

			}
			else {
				if (i == 7) {
					//8个点都在内部
					GrandNode->children[threadIdx.x]->exist = false;
					return;
				}
				continue;
			}
		}

	}
	return;
}

__global__ void Cutter_Dynamic_64(Octree* GrandNode, BoundBox& CutterBox, Point3f& CutPosition, Point3f& CutterSize,bool alloc) {

	//为被分割出来的一个子节点分配显存。
	//printf("开始分割：\n");
	//int index = blockIdx.x * 16 + (threadIdx.x) * 4 + (threadIdx.y);
	int index = threadIdx.x;
	//if(!alloc)
    if ((GrandNode->children[index]) == nullptr ){
		//printf("分配内存初始化\n");
		cudaMalloc((void**)&(GrandNode->children[index]), sizeof(Octree));
		GrandNode->children[index]->accuracy = GrandNode->accuracy + 1;
		//printf("allocMemeoty,accuracy:%d\n", GrandNode->children[threadIdx.x]->accuracy);
		//计算子节点的原点
		GrandNode->children[index]->origin.x = GrandNode->origin.x - GrandNode->halfDimension.x * 3 / 4;
		GrandNode->children[index]->origin.x = GrandNode->children[index]->origin.x + ((threadIdx.x & 3) * GrandNode->halfDimension.x) /2;

		GrandNode->children[index]->origin.y = GrandNode->origin.y - GrandNode->halfDimension.y * 3 / 4;
		GrandNode->children[index]->origin.y = GrandNode->children[index]->origin.y + ((threadIdx.x & 12) / 4 * GrandNode->halfDimension.y) /2;

		GrandNode->children[index]->origin.z = GrandNode->origin.z - GrandNode->halfDimension.z * 3 / 4;
		GrandNode->children[index]->origin.z = GrandNode->children[index]->origin.z + ((threadIdx.x & 48) / 16 * GrandNode->halfDimension.z)/2 ;

		//计算尺寸
		GrandNode->children[index]->halfDimension.x = GrandNode->halfDimension.x / 4;
		GrandNode->children[index]->halfDimension.y = GrandNode->halfDimension.y / 4;
		GrandNode->children[index]->halfDimension.z = GrandNode->halfDimension.z / 4;

		//设定初始参数
		GrandNode->children[index]->exist = true;
		GrandNode->children[index]->sub = false;
		for (int i = 0; i <= 63; ++i) {
			GrandNode->children[index]->children[i] = nullptr;
		}
	}
	//子节点的精细度+1
	//printf("精度为%d\n", GrandNode->children[threadIdx.x]->accuracy);
	if (GrandNode->children[index]->accuracy >= 3 || !GrandNode->children[index]->exist) return;

	if (IsIntersect(GrandNode->children[index], CutterBox)) {
		//printf("判断完毕\n");
		for (int i = 0; i <= 7; i++) {
			if (!IsIn_a(GrandNode->children[index], i, CutPosition, CutterSize)) {
				
				//*printf("分割\n");
				bool alloc = GrandNode->children[index]->sub;//判断其是否已经被分内存
				GrandNode->children[index]->sub = true;
				//printf("分割:精度%d\n", GrandNode->children[threadIdx.x]->accuracy);
				dim3 grid(1);
				dim3 block(64);
				Cutter_Dynamic_64 << < grid,block >> > (GrandNode->children[index], CutterBox, CutPosition, CutterSize, alloc);
				return;
			}
			else {
				if (i == 7) {
					//8个点都在内部
					//printf("*****  剔除体素 *****\n");
					GrandNode->children[index]->exist = false;
					return;
				}
				continue;
			}
		}

	}
	return;
}

__device__ 	void GetOctreeVertex(int number, Octree* curr, Point3f& OctreeVertex) {
	OctreeVertex.x = curr->origin.x - curr->halfDimension.x;
	OctreeVertex.x = OctreeVertex.x + (number & 4) / 2 * curr->halfDimension.x;

	OctreeVertex.y = curr->origin.y - curr->halfDimension.y;
	OctreeVertex.y = OctreeVertex.y + (number & 2) * curr->halfDimension.y;

	OctreeVertex.z = curr->origin.z - curr->halfDimension.z;
	OctreeVertex.z = OctreeVertex.z + (number & 1) * 2 * curr->halfDimension.z;

}

__device__ void IteraAddPoints(Octree* curr, Myvector* testVertex, Point3f* buffer) {
	//printf("添加点:\n");
	if (!curr->sub) {
		Point3f OctreeVertex;
		GetOctreeVertex(0, curr, OctreeVertex);

		Point3f halfDimension = curr->halfDimension;
		Push(testVertex, OctreeVertex, buffer); Push(testVertex, curr->halfDimension, buffer);
		GetOctreeVertex(7, curr, OctreeVertex);

		halfDimension.x *= (-1); halfDimension.y *= (-1); halfDimension.z *= (-1);
		Push(testVertex, OctreeVertex, buffer);Push(testVertex, halfDimension, buffer);
	}
	else
	{
		for (int i = 0; i <= 7; ++i) {
			if (curr->children[i]->exist){
				//printf("探索下一个子节点：当前深度%d\n", curr->children[i]->accuracy);
				IteraAddPoints(curr->children[i], testVertex, buffer);
			}
		}
	}

	//printf("传入函数前(%f,", test.x);
//printf("%f,", test.y);
//printf("%f)\n", test.z);
//Push(testVertex, test);
//printf("压入四个点\n");
}

__global__ void AddPoints(Octree* curr, Myvector* testVertex,Point3f* buffer) {

	//printf("开始遍历：\n");
	//testVertex->buf_len_ = 2000000;
	//testVertex->cnt_top_ = 0;
	
	IteraAddPoints(curr, testVertex, buffer);
}
__global__ void TestVector(Myvector* testVertex, Point3f* buffer) {

	//buffer[0].x = 55;
	//buffer[0].y = 55;
	//buffer[0].z = 55;
	for (int i = 0; i < testVertex->cnt_top_; ++i) {
		//testVertex->buf_[i];

		printf("buffer[%d]: ",i);
		printf("(%f,", buffer[i].x);
		printf("%f,",  buffer[i].y);
		printf("%f)\n", buffer[i].z);
	}
}


__global__ void InitRoot(Octree* Root, BoundBox& CutterBox, Point3f& CutterPos, Point3f& CutterSize) {

	bool alloc;
	printf("Root大小：%d",sizeof(Octree));
	printf("RootSub:%d\n", Root->sub);
	if (!Root->sub)
	{
		alloc = false;
		Root->sub = true;
	}
	Root->accuracy = 0;
	Root->exist = true;
	dim3 grid(1);
	dim3 block(64);
	Cutter_Dynamic << < 1, 8 >> > (Root, CutterBox, CutterPos, CutterSize);
	//Cutter_Dynamic_64 <<< grid, block >> > (Root, CutterBox, CutterPos, CutterSize, Root->sub);

}


//在CPU计算得到包围盒，放在共享内存中
BoundBox GetCutterBoundBox(Point3f& Center, Point3f& CutterSize)//暂时不考虑刀具的旋转,输入刀具的位置和大小参数
{
	//暂时不考虑刀具的旋转；
	BoundBox box;

	box.Origin.x = Center.x;
	box.Origin.y = Center.y;
	box.Origin.z = Center.z + (CutterSize.x - CutterSize.y) / 2;

	box.length = 2 * CutterSize.y;
	box.width = 2 * CutterSize.y;
	box.height = CutterSize.y + CutterSize.x;

	return box;
}
Point3f GetCutterPos(float T, float t, Point3f& InitCutPosition, Point3f& origin, Point3f& halfDimension) {

	float part = t / T;

	Point3f res;
	res.x = InitCutPosition.x + part * halfDimension.x * 2 * 3 / 2;
	res.y = InitCutPosition.y + part * halfDimension.y * 2 * 3 / 2;
	res.z = InitCutPosition.z;

	/*return Point3f(InitCutPosition.x + part * halfDimension.x * 2 * 3 / 2,
		InitCutPosition.y + part * halfDimension.y * 2 * 3 / 2,
		InitCutPosition.z);//单位为mm;
	*/
	return res;
}

//使用刀具分割切削八叉树，获得切削后的八叉树节点

int Run_GPU_Octree() {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return 0;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
			std::cout << "Failed to initialize GLAD" << std::endl;
			return 0;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	camera.Right += 100;//调整相机视角

	GLuint VBO[3], VAO[3], EBO;

	glGenVertexArrays(3, VAO);
	glGenBuffers(3, VBO);
	glGenBuffers(1, &EBO);

	Shader ourShader("./res/Octree_DrawPoints.vs", "./res/Octree_DrawPoints.fs", "./res/Octree_DrawPoints.gs");
	


	Point3f CutPosition, HostCutterSize, HostCutterPos;
	BoundBox HostCutterBox;

	Point3f origin, halfDimension;
	//设置工件初始位置
	origin.x = 30.0f; origin.y = 30.0f; origin.z = 30.0f;
	//设置工件尺寸
	halfDimension.x = 20.0f; halfDimension.y = 20.0f; halfDimension.z = 20.0f;
	Octree temp(origin, halfDimension);
	temp.sub = true;//重要
	Octree* HostRoot = &temp;

	CutPosition.x = 20;
	CutPosition.y = 20;
	CutPosition.z = 50;

	HostCutterSize.x = 8;
	HostCutterSize.y = 6;
	HostCutterSize.z = 6;


  

	//开始切割，刀具运动
	
	float  T = 1000;//设为1000
	float delta_t = 2;//步长设为2
	float t = 0;
	HostCutterPos = GetCutterPos(T, t, CutPosition,origin,halfDimension);
	
	Octree* DeviceRoot;
	Point3f* DeviceCutterSize;
	Point3f* DeviceCutterPos;
	BoundBox* DeviceCutterBox;
	

	cudaMalloc((void**)&(DeviceRoot), sizeof(Octree));
	cudaMemcpy(DeviceRoot, HostRoot, sizeof(Octree), cudaMemcpyHostToDevice);
	bool RootSub = false;
	cudaMemcpy(&(DeviceRoot->sub), &RootSub, sizeof(bool), cudaMemcpyHostToDevice);
	cudaMalloc((void**)&(DeviceCutterSize), sizeof(Point3f));
	cudaMalloc((void**)&(DeviceCutterPos), sizeof(Point3f));
	cudaMalloc((void**)&(DeviceCutterBox), sizeof(BoundBox));
	
	
	Myvector* HostVector;
	Myvector* DeviceVector;
	cudaMallocHost((void**)&HostVector, sizeof(Myvector));
	cudaMalloc((void**)&(DeviceVector), sizeof(Myvector));

	const int BufferLen = 40000;

	HostVector->buf_len_ = BufferLen;
	HostVector->cnt_top_ = 0;

	Point3f* D_buffer;//Device端数组
	cudaMalloc((void**)&(D_buffer), BufferLen * sizeof(Point3f));

	Point3f* testvector;//host端的数组
	testvector = (Point3f*)malloc(BufferLen * sizeof(Point3f));


	//for (t = 0 ; t <= T;t+= delta_t) 

	while (!glfwWindowShouldClose(window))
	{
		
		t += delta_t;
		cout << "循环" << t / 2 << "次\n" << endl;
		HostCutterPos = GetCutterPos(T, t, CutPosition, origin, halfDimension);
		//t += delta_t;
		HostCutterBox = GetCutterBoundBox(HostCutterPos, HostCutterSize);
		//cout << "调用Cutter_Dynamic" << endl;
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		ourShader.use();
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);
		ourShader.setMat4("projectionInverse", glm::inverse(projection));
		ourShader.setMat4("viewInverse", glm::inverse(view));
		glm::mat4 model = glm::mat4(1.0f);

		model = glm::translate(model, lightPos); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));	// it's a bit too big for our scene, so scale it down
		ourShader.setMat4("model", model);

		//设置光照参数
		//ourShader.setVec3("lightPos", glm::vec3(60.5f, 60.5f, 30.5f));
		ourShader.setVec3("lightPos", glm::vec3(-80.0f, -80.0f, -80.0f));
		ourShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
		ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.5f, 0.31f));

		processInput(window);

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(ourShader.ID);


		cudaMemcpy(DeviceCutterBox, &HostCutterBox, sizeof(BoundBox), cudaMemcpyHostToDevice);
		cudaMemcpy(DeviceCutterSize, &HostCutterSize, sizeof(Point3f), cudaMemcpyHostToDevice);
		cudaMemcpy(DeviceCutterPos, &HostCutterPos, sizeof(Point3f), cudaMemcpyHostToDevice);
		
		//可以考虑将存储可渲染点的数组分成8个，最后再进行合并
		//Cutter_Dynamic << < 1,8 >> > (DeviceRoot, *DeviceCutterBox, *DeviceCutterPos, *DeviceCutterSize);
		/*dim3 grid(1);
		dim3 block(64);
		Cutter_Dynamic_64 << < grid, block >> > (DeviceRoot, *DeviceCutterBox, *DeviceCutterPos, *DeviceCutterSize);
		*/
		InitRoot << < 1,1 >> > (DeviceRoot, *DeviceCutterBox, *DeviceCutterPos, *DeviceCutterSize);
		cudaDeviceSynchronize();//该方法将停止CPU端线程的执行，直到GPU端完成之前CUDA的任务，包括kernel函数、数据拷贝等

		//此时可以获得分割后的八叉树，之后需要遍历八叉树获得可以被渲染的体素
	
	    //设置数组初始尺寸
	    //创建数组，分配内存
		cudaMemcpy(DeviceVector, HostVector, sizeof(Myvector), cudaMemcpyHostToDevice);

		AddPoints <<<1, 1 >>> (DeviceRoot, DeviceVector, D_buffer);
		cudaDeviceSynchronize();
		//TestVector<<<1,1>>>(DeviceVector);
		
		//Point3f* Mypoints =nullptr;
		//cudaMemcpy(&HostVector, DeviceVector, sizeof(Myvector), cudaMemcpyDeviceToHost);//复制device端的参数

		int PointNum = 0;
		//cudaMemcpy(HostVector, DeviceVector,  sizeof(Myvector), cudaMemcpyDeviceToHost);
		//PointNum = (HostVector->cnt_top_)[0];
		cudaMemcpy(&PointNum, &(DeviceVector->cnt_top_), sizeof(int), cudaMemcpyDeviceToHost);
		cout << "D_buffer:" << endl;
		cout << "获得 " << PointNum << " 个点" << endl;
		
		//TestVector << <1, 1 >> > (DeviceVector, D_buffer);
		//cudaDeviceSynchronize();
		//cudaMallocHost((void**)&testvector, PointNum * sizeof(Point3f));
		//testvector = (Point3f*)malloc(HostVector.cnt_top_ * sizeof(Point3f));

		cudaMemcpy(testvector, D_buffer, PointNum * sizeof(Point3f), cudaMemcpyDeviceToHost);
		cout << "";
	
		//cudaMemcpy用于在主机（Host）和设备（Device）之间往返的传递数据，用法如下：
		//主机到设备：cudaMemcpy(d_A, h_A, nBytes, cudaMemcpyHostToDevice)
		//设备到主机：cudaMemcpy(h_A, d_A, nBytes, cudaMemcpyDeviceToHost)
		//注意：该函数是同步执行函数，在未完成数据的转移操作之前会锁死并一直占有CPU进程的控制权，所以不用再添加cudaDeviceSynchronize()函数

		//if (HostVector->cnt_top_ != 0){
	    //HostVector.buf_ = (Point3f*)malloc((HostVector.cnt_top_) * sizeof(Point3f));
		
				   //顶点绘制
		glBindVertexArray(VAO[2]);//绑定第一个顶点对象Id到顶点对象，顶点对象包含了顶点属性设置，缓冲对象设置等一系列属性
		glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);//绑定缓冲对象
		//设置缓冲区需要存储的顶点集，用于后续传送给GPU
		glBufferData(GL_ARRAY_BUFFER, PointNum * sizeof(Point3f), testvector, GL_STATIC_DRAW);
		//设置顶点位置属性，指定location为0
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(Point3f), (void*)0);//体素的一个顶点
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(Point3f), (void*)(sizeof(Point3f)));//体素的长宽高
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);
		glBindVertexArray(VAO[2]);
		glDrawArrays(GL_POINTS, 0, PointNum);

		//cout << "获得 " << DeviceVector->cnt_top_ << " 个点" << endl;
		glfwSwapBuffers(window);
		glfwPollEvents();

		cudaMemcpy(DeviceVector, HostVector, sizeof(Myvector), cudaMemcpyHostToDevice);
		//free(testvector);
		//cudaFree(D_buffer);
	    //}
	}

	glfwTerminate();

	return 0;
	

	/*for (int i = 0; testVector.cnt_top_ > i; i += 2) {
	cout << "点[" << i << "]位置 (" << (&Mypoints)[i]->x << "," << (&Mypoints)[i]->y << "," << (&Mypoints)[i]->z << ")"
		<< "点[" << i << "]尺寸 (" << (&Mypoints)[i + 1]->x << "," << (&Mypoints)[i + 1]->y << "," << (&Mypoints)[i + 1]->z << ")\n";
}*/

	return 0;

}

int main() {
	cudaEvent_t start, stop;
	float elapseTime = 0;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);
	cudaEventRecord(start, 0);

	Run_GPU_Octree();

	cudaEventRecord(stop, 0);
	cudaEventSynchronize(stop);
	cudaEventElapsedTime(&elapseTime, start, stop);
	system("pause");
	return 0;
}



void  processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void  framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}



//printf("ֵ:%d", ret[threadIdx.x]);
//printf("from AplusThree function\n");
//printf(" Octree* test：%d  \n" ,test->accuracy);
//printf("threadIdx.x: %d \n", threadIdx.x);

/*//初始化节点
void InitRootNode(Point3f& CutPosition, Point3f& CutterSize, Octree* Root) {

	Root->origin.x = 30.0f;  //设置初始位置
	Root->origin.y = 30.0f;
	Root->origin.z = 30.0f;

	Root->halfDimension.x = 20.0f;
	Root->halfDimension.y = 20.0f;
	Root->halfDimension.z = 20.0f;

	Root->accuracy = 0;
	Root->sub = true;

	CutPosition = Point3f(20, 20, 50);
	CutterSize = Point3f(8, 6, 6); 

}

//T = 1000; delta_t = 2
void RunCutter(float TT,float Delta_t, Point3f& CutPosition, Point3f& CutterSize, Octree* Root) {

	float  T = TT;//设为1000
	float delta_t = Delta_t;//设为2
	float t = 0;
	Point3f CutterPos = GetCutterPos(T, t,CutPosition,Root);
	while (true) {
		CutterPos = GetCutterPos(T, t, CutPosition, Root);
		t += delta_t;
		BoundBox CutterBox = GetCutterBoundBox(CutterPos, CutterSize);
		Cutter_Dynamic << < 1, 8 >> > (Root, CutterBox, CutterPos, CutterSize);//可以考虑将存储可渲染点的数组分成8个，最后再进行合并

	}
	
}
*/