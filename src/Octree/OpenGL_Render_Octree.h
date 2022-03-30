#pragma once
#include"Octree.h"
#include"../src/Octree/MyStruct.h">
#include<stack>
#include<vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>
//#include <learnopengl/camera.h>
#include"../OpenGL/Mycamera.h"
//#include <learnopengl/model.h>
#include<learnopengl/filesystem.h>

#include <unordered_map>
#include<map>
#include<iostream>
#include<../src/Cuda/Octree_DeeperCut.cuh>
#include<../src/Cuda/CudaOctree.cuh>
#include<../src/ReadSTLfile/ReadPath.h>
#include<bitset>
#include<math.h>
//#include<../src/Cuda/Header.cuh>
using namespace std;

class HashFunctionOfPoint3f {
public:
	size_t operator()(const Point3f& p) const
	{
		return size_t((p.x  + p.y * 2 + p.z * 3 ));
	}

	bool operator()(const Point3f& a, const Point3f& b) const
	{
		if (((a.x - b.x) >= 0.0000001)) return false;
		if (((a.y - b.y) >= 0.0000001)) return false;
		if (((a.z - b.z) >= 0.0000001)) return false;
		return true;
	}
};

class HashFunctionOfPoint3i {
public:
	size_t operator()(const Point3i& p) const
	{
		return size_t((p.x + p.y  + p.z ));
	}

	bool operator()(const Point3i& a, const Point3i& b) const
	{
        if(a.x == b.x && a.y == b.y && a.z==b.z) return true;
	}
};




/*
void Init_Data(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos, Point3f& CutterSize) {

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
	cudaMemcpy(D_buffer, bri->brick, 64 * sizeof(bool), cudaMemcpyHostToDevice);




	Cutter* Device_Cutter;
	cudaMalloc((void**)&(Device_Cutter), sizeof(Cutter));

	Device_Deeper_cut << <1, 64 >> > (D_buffer, *Device_Cutter);

	cudaMemcpy(bri->brick, D_buffer, 64 * sizeof(bool), cudaMemcpyDeviceToHost);

}
*/
class OctreeCut_2 {
private:

	std::unordered_map<Point3f, int, HashFunctionOfPoint3f>  positionToVertex;//位置对应顶点索引
	std::unordered_map<Point3i, int, HashFunctionOfPoint3i>  vertexToFace;//三个顶点索引对应一个三角面片
	int TempVertex[8] = {};//存储一个节点的8个顶点的索引,用于分割体素
	Point3f TestTempVertex[8] = {};//存储一个节点的8个顶点的位置,用于分割体素
	ReadPath Path;

public:

	Octree* Head;//工件的初始值
	Point3f C;
	Point3f CutterSize;//H，R，r;目前认为R==r
	float Accuracy;
	//int Iteration;
	int CubeID = 0;
	int VoxelNum = 1;
	int BoolNum = 0;
	vector<Point3f> vertices;//点的位置集，
	vector<Point3i> faces;//点的索引集，三个索引组成一个面

	vector<Point3f> testVertex;

	stack<Octree*>  OctreeLine;//存储未被判断的体素节点
	stack<Octree*> RenderedCube;
	stack<Octree*> BrickChildToBeDelete;

	OctreeCut_2(Octree& copy, Point3f Center, Point3f CS) :C(Center), CutterSize(CS) {
		Head = new Octree(copy);
		
	}

	//获取顶点在数组中的索引
	int getVertexIndex(const Point3f& position)
	{
		auto it = positionToVertex.find(position);
        
        if (it != positionToVertex.end())
        {
        	return it->second;
        }
        

		int ret = static_cast<int>((vertices.size()));
		//获取当前vertices的大小并作为新顶点的下标,每三个float代表一个顶点位置

		Point3f NewPosition = position;//创建一个VertexRecord对象并初始其位置为position
		//推入位置的三个参数，后面再设置顶点属性
		vertices.push_back(NewPosition);

		positionToVertex[position] = ret;

		return ret;
	}
	
	//将vec3中的数字排序
	Point3i SortVec3(Point3i vec3) {//输入三个顶点的索引，将其排序
		Point3i res;
		if (vec3.x > vec3.y) {
			if (vec3.x > vec3.z) {
				res.x = vec3.x;
				if (vec3.z > vec3.y) {
					res.y = vec3.z;
					res.z = vec3.y;
				}
				else {
					res.y = vec3.y;
					res.z = vec3.z;
				}
			}
			else {
				res.x = vec3.z;
				res.y = vec3.x;
				res.z = vec3.y;
			}
		}
		else {
			if (vec3.y > vec3.z) {
				res.x = vec3.y;
				if (vec3.x > vec3.z) {
					res.y = vec3.x;
					res.z = vec3.z;
				}
				else {
					res.y = vec3.z;
					res.z = vec3.x;
				}
			}
			else {
				res.x = vec3.z;
				res.y = vec3.y;
				res.z = vec3.x;
			}
		}

		return res;
	}

	//获取面片在面片数组中的索引，面片不存在时向faces添加面片
	int getFaceIndex(Point3i SortedvertexPair)
	{
		//auto sortedVertexPair = vertexPair.x < vertexPair.y ? vertexPair : vertexPair.yx();
		auto it = vertexToFace.find(SortedvertexPair);
		if (it != vertexToFace.end())
		{
			return it->second;
		}

		int ret = static_cast<int>((faces.size()));

	   //vertexPair需要时xyz从大到小

		faces.push_back(SortedvertexPair);
		vertexToFace[SortedvertexPair] = ret;

		return ret;
	}

	//向faces数组中添加面片，和getFaceIndex()函数功能有重复，可以考虑删除该函数
	void addTriangle(Point3i SortedvertexPair) {

		auto it = vertexToFace.find(SortedvertexPair);
		if (it != vertexToFace.end())
		{
			return;
		};
		//vertexPair需要时xyz从小到大
		faces.push_back(SortedvertexPair);
		vertexToFace[SortedvertexPair] = (static_cast<int>((faces.size()))) - 1;
		return;

	}

	//输入整数和节点指针，获取节点体素的某个顶点的位置
	Point3f GetOctreeVertex(int number, Octree* curr) {
		Point3f OctreeVertex;
		/*if (((number & 1) == 1)) { OctreeVertex.z = curr->origin.z + curr->halfDimension.z; }//Z轴正方向（上方）
		else { OctreeVertex.z = curr->origin.z - curr->halfDimension.z; }

		if (((number & 2) == 2)) { OctreeVertex.y = curr->origin.y + curr->halfDimension.y; }//Y轴正方向（前方）
		else { OctreeVertex.y = curr->origin.y - curr->halfDimension.y; }

		if (((number & 4) == 4)) { OctreeVertex.x = curr->origin.x + curr->halfDimension.x; }//Z轴正方向（右方）
		else { OctreeVertex.x = curr->origin.x - curr->halfDimension.x; }

		return OctreeVertex;
		*/


		OctreeVertex.x = curr->origin.x - curr->halfDimension.x;
		OctreeVertex.x = OctreeVertex.x + (number & 4) / 2 * curr->halfDimension.x;

		OctreeVertex.y = curr->origin.y - curr->halfDimension.y;
		OctreeVertex.y = OctreeVertex.y + (number & 2) * curr->halfDimension.y;

		OctreeVertex.z = curr->origin.z - curr->halfDimension.z;
		OctreeVertex.z = OctreeVertex.z + (number & 1) * 2 * curr->halfDimension.z;

		return OctreeVertex;
	}

	Point3f GetOctreeVertexForBrick(int number, Point3f& origin,Point3f& halfDimension) {
		Point3f OctreeVertex;

		OctreeVertex.x = origin.x - halfDimension.x;
		OctreeVertex.x = OctreeVertex.x + (number & 4) / 2 * halfDimension.x;

		OctreeVertex.y = origin.y - halfDimension.y;
		OctreeVertex.y = OctreeVertex.y + (number & 2) * halfDimension.y;

		OctreeVertex.z = origin.z - halfDimension.z;
		OctreeVertex.z = OctreeVertex.z + (number & 1) * 2 * halfDimension.z;

		return OctreeVertex;
	}
	//添加面片索引进入faces，用于按索引绘制
	void addCube(Octree* curr) {
		//if (curr->rendered) return;
		Point3f temp;
		for (int i = 0; i <= 7; i++) {
			temp = GetOctreeVertex(i, curr);
			TempVertex[i] = getVertexIndex(temp);
		}

		//SortVec3()
		//getFaceIndex()
		
		/*addTriangle(Point3i(TempVertex[0], TempVertex[1], TempVertex[3]));
		addTriangle(Point3i(TempVertex[0], TempVertex[2], TempVertex[3]));
	    addTriangle(Point3i(TempVertex[0], TempVertex[2], TempVertex[4]));
		addTriangle(Point3i(TempVertex[2], TempVertex[4], TempVertex[6]));
		addTriangle(Point3i(TempVertex[0], TempVertex[4], TempVertex[5]));
		addTriangle(Point3i(TempVertex[2], TempVertex[1], TempVertex[5]));
		addTriangle(Point3i(TempVertex[1], TempVertex[5], TempVertex[7]));
		addTriangle(Point3i(TempVertex[1], TempVertex[3], TempVertex[7]));
		addTriangle(Point3i(TempVertex[2], TempVertex[3], TempVertex[6]));
		addTriangle(Point3i(TempVertex[3], TempVertex[6], TempVertex[7]));
	    addTriangle(Point3i(TempVertex[5], TempVertex[6], TempVertex[7]));
		addTriangle(Point3i(TempVertex[4], TempVertex[5], TempVertex[6]));*/

		faces.push_back(Point3i(TempVertex[0], TempVertex[1], TempVertex[3]));//不考虑面片的正反面，建议不要开启遮挡剔除（深度测试？）
		faces.push_back(Point3i(TempVertex[3], TempVertex[2], TempVertex[0]));

		faces.push_back(Point3i(TempVertex[0], TempVertex[4], TempVertex[6]));//之后还要考虑添加的法向量的正负
		faces.push_back(Point3i(TempVertex[6], TempVertex[2], TempVertex[0]));
		
		faces.push_back(Point3i(TempVertex[1], TempVertex[5], TempVertex[7]));//
		faces.push_back(Point3i(TempVertex[7], TempVertex[3], TempVertex[1]));
		
		faces.push_back(Point3i(TempVertex[0], TempVertex[4], TempVertex[5]));//
		faces.push_back(Point3i(TempVertex[5], TempVertex[1], TempVertex[0]));
		
		faces.push_back(Point3i(TempVertex[2], TempVertex[6], TempVertex[7]));//
		faces.push_back(Point3i(TempVertex[7], TempVertex[3], TempVertex[2]));
		
		faces.push_back(Point3i(TempVertex[4], TempVertex[5], TempVertex[7]));//
		faces.push_back(Point3i(TempVertex[7], TempVertex[6], TempVertex[4]));
		

		/*faces.push_back(SortVec3(Point3i(TempVertex[0], TempVertex[1], TempVertex[3])));//不考虑面片的正反面，建议不要开启遮挡剔除（深度测试？）
		faces.push_back(SortVec3(Point3i(TempVertex[0], TempVertex[2], TempVertex[3])));
		faces.push_back(SortVec3(Point3i(TempVertex[0], TempVertex[2], TempVertex[4])));//之后还要考虑添加的法向量的正负
		faces.push_back(SortVec3(Point3i(TempVertex[2], TempVertex[4], TempVertex[6])));
		faces.push_back(SortVec3(Point3i(TempVertex[0], TempVertex[4], TempVertex[5])));//
		faces.push_back(SortVec3(Point3i(TempVertex[2], TempVertex[1], TempVertex[5])));
		faces.push_back(SortVec3(Point3i(TempVertex[1], TempVertex[5], TempVertex[7])));//
		faces.push_back(SortVec3(Point3i(TempVertex[1], TempVertex[3], TempVertex[7])));
		faces.push_back(SortVec3(Point3i(TempVertex[2], TempVertex[3], TempVertex[6])));//
		faces.push_back(SortVec3(Point3i(TempVertex[3], TempVertex[6], TempVertex[7])));
		faces.push_back(SortVec3(Point3i(TempVertex[5], TempVertex[6], TempVertex[7])));//
		faces.push_back(SortVec3(Point3i(TempVertex[4], TempVertex[5], TempVertex[6])));*/


		//faces.push_back(TempVertex[0]); faces.push_back(TempVertex[3]); faces.push_back(TempVertex[1]);//左
		//faces.push_back(TempVertex[0]); faces.push_back(TempVertex[2]); faces.push_back(TempVertex[3]);

		//faces.push_back(TempVertex[0]); faces.push_back(TempVertex[4]); faces.push_back(TempVertex[2]);//下
		//faces.push_back(TempVertex[2]); faces.push_back(TempVertex[4]); faces.push_back(TempVertex[6]);

		//faces.push_back(TempVertex[0]); faces.push_back(TempVertex[5]); faces.push_back(TempVertex[4]);//后
		//faces.push_back(TempVertex[0]); faces.push_back(TempVertex[1]); faces.push_back(TempVertex[5]);

		//faces.push_back(TempVertex[1]); faces.push_back(TempVertex[7]); faces.push_back(TempVertex[5]);//上
		//faces.push_back(TempVertex[1]); faces.push_back(TempVertex[3]); faces.push_back(TempVertex[7]);

		//faces.push_back(TempVertex[2]); faces.push_back(TempVertex[6]); faces.push_back(TempVertex[3]);//前
		//faces.push_back(TempVertex[3]); faces.push_back(TempVertex[6]); faces.push_back(TempVertex[7]);

		//faces.push_back(TempVertex[5]); faces.push_back(TempVertex[7]); faces.push_back(TempVertex[6]);//右
		//faces.push_back(TempVertex[4]); faces.push_back(TempVertex[5]); faces.push_back(TempVertex[6]);

		//curr->rendered = true;
		return;

	}

	//添加面片的三个顶点进入testvertices数组，用于按顶点绘制，目前使用的绘制方法
	void addtestCube(Octree* curr) {

		Point3f temp;
		for (int i = 0; i <= 7; i++) {
			temp = GetOctreeVertex(i, curr);
			TestTempVertex[i] = temp;
		}
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[1]); testVertex.push_back(TestTempVertex[3]);
		testVertex.push_back(TestTempVertex[3]); testVertex.push_back(TestTempVertex[2]); testVertex.push_back(TestTempVertex[0]);

		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[4]); testVertex.push_back(TestTempVertex[6]);
		testVertex.push_back(TestTempVertex[6]); testVertex.push_back(TestTempVertex[2]); testVertex.push_back(TestTempVertex[0]);

		testVertex.push_back(TestTempVertex[1]); testVertex.push_back(TestTempVertex[5]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[3]); testVertex.push_back(TestTempVertex[1]);

		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[4]); testVertex.push_back(TestTempVertex[5]);
		testVertex.push_back(TestTempVertex[5]); testVertex.push_back(TestTempVertex[1]); testVertex.push_back(TestTempVertex[0]);

		testVertex.push_back(TestTempVertex[2]); testVertex.push_back(TestTempVertex[6]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[3]); testVertex.push_back(TestTempVertex[2]);

		testVertex.push_back(TestTempVertex[4]); testVertex.push_back(TestTempVertex[5]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[6]); testVertex.push_back(TestTempVertex[4]);
	




	}

	//获取刀具的AABB包围盒，用于判断是否和某个体素节点相交
	BoundBox GetCutterBoundBox(Point3f& Center)//暂时不考虑刀具的旋转
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

	//判断体素节点是否与刀具相交，体素节点完全位于包围盒内也认为是相交
	bool IsIntersect(Octree* Voxel, BoundBox& Cutter) {
		if (std::abs(Voxel->origin.x - Cutter.Origin.x) < std::abs(Voxel->halfDimension.x + (Cutter.length / 2))
			&& std::abs(Voxel->origin.y - Cutter.Origin.y) < std::abs(Voxel->halfDimension.y + (Cutter.width / 2))
			&& std::abs(Voxel->origin.z - Cutter.Origin.z) < std::abs(Voxel->halfDimension.z + (Cutter.height / 2))
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

	bool IsIntersectForBrick(Point3f& origin, Point3f halfDimension, BoundBox& Cutter) {
		if (std::abs(origin.x - Cutter.Origin.x) < std::abs(halfDimension.x + (Cutter.length / 2))
			&& std::abs(origin.y - Cutter.Origin.y) < std::abs(halfDimension.y + (Cutter.width / 2))
			&& std::abs(origin.z - Cutter.Origin.z) < std::abs(halfDimension.z + (Cutter.height / 2))
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


	void ReadPathFile(const char* cfilename ) {
		Path.ReadPathfile(cfilename);
	}
	Point3f GetFileCutterPos(int Index) {
	
		if (Index >= Path.CutterPath.size()) {
			Index = Path.CutterPath.size() - 1;
		}
		return Path.CutterPath[Index];
	}
	//获取刀具在某个时间微元的位置，里面定义了刀具的轨迹
	Point3f GetCutterPos(float T, float t) {

		float part = t / T;

		return Point3f(C.x + part * Head->halfDimension.x * 2 * 3 / 2, C.y + part * Head->halfDimension.y * 2 * 3 / 2, C.z);//单位为mm;

	}

	Point3f GetCutterStarPos(float T, float t) {
		
			const float z = C.z;
			Point3f temp;
			if (t <= 200) {
				temp =  Point3f(-0.6f * (float)t / 200.0f , (0.8f - 1.6f * (float)t / 200.0f), z);
			}
			else if (t <= 400) {
				temp = Point3f((-0.6f + 1.4f * (float)(t - 200) / 200.0f),(-0.8f + 1.0f * (float)(t - 200) / 200.0f) , z);
			}
			else  if (t <= 600) {
				temp = Point3f((0.8f - 1.6f * (float)(t - 400) / 200.0f), 0.2f, z);
			}
			else if (t <= 800) {
				temp = Point3f((-0.8f + 1.4f * (float)(t - 600) / 200.0f), (0.2f - 1.0f * (float)(t - 600) / 200.0f), z);
			}
			else if (t <= 1000) {
				temp = Point3f((0.6f - 0.6f * (float)(t - 800) / 200.0f) ,(-0.8f + 1.6f * (float)(t - 800) / 200.0f) , z);
			}
			else  {
				temp = Point3f(0 + Head->halfDimension.x, (float)t * 5 + Head->halfDimension.y / 200.0f, z);
			}
			temp.x = C.x + temp.x * Head->halfDimension.x;
			temp.y = C.y + temp.y * Head->halfDimension.y;
			return temp;

			
			//temp.x = C.x + temp.x * Head->halfDimension.x;

		/*
		float part = t / T;
		//0~10s，从(15,50)->(85,50)
		if (t <= 200) {
			return Point3f(t * 7, 0, 0) + C;
		}
		//10~20s,从(85,50)->(30,10)
		else if (t <= 400) {
			t -= 200;
			return Point3f(t * -5.5, t * -4, 0) + Point3f(85, 50, C.z);
		}
		//20s~30s,从(30,10)->(50,75)
		else if (t <= 600) {
			t -= 20;
			return Point3f(t * 2, t * 6.5, 0) + Point3f(30, 10, C.z);
		}
		//30s~40s,从(50,75)->(70,10)
		else if (t <= 800) {
			t -= 30;
			return Point3f(t * 2, t * -6.5, 0) + Point3f(50, 75, C.z);
		}
		//40s~50s,从(70,10)->(15,50)
		else {
			t -= 40;
			return Point3f(t * -5.5, t * 4, 0) + Point3f(70, 10, C.z);
		}
		*/
	}
	//判断体素的某个顶点是否在刀具内部
	bool IsIn(Point3f& OctreeVer, Point3f CutPosition) {

		float X = abs(OctreeVer.x - CutPosition.x);
		float Y = abs(OctreeVer.y - CutPosition.y);
		float Z =    (OctreeVer.z - CutPosition.z);
		//if (X > CutterSize.y) return false;
		//if (Y > CutterSize.y) return false;
		bool IntheCircle = ((X * X + Y * Y) <= CutterSize.y * CutterSize.y);//在球头铣刀竖直方向内
		
		if (IntheCircle && (OctreeVer.z > CutPosition.z)) return true;

		bool InTheSphere = (X * X + Y * Y + Z * Z) <= (CutterSize.y * CutterSize.y);//在球头铣刀半球内
		//if (Z > CutterSize.x) return false;
		//if (OctreeVer.z <= CutPosition.z) return true;
		if (InTheSphere) return true;

		return false;

	}

    //用于静态显示切削
	void Cutter(Point3f& CutterPos, int& Itera, BoundBox CutterBox) {


		if (OctreeLine.empty()) return;

		Octree* curr = OctreeLine.top();
		OctreeLine.pop();
		if (curr == nullptr || curr->exist == false) { return; };

		//if (Itera > Iteration || curr->accuracy > Iteration ) 
		if (curr->accuracy > Itera)
		{
			addtestCube(curr);
			//addCube(curr);//迭代次数超过最大次数，迭代分割结束,将该体素添加进面片集合，然后返回
			return;
		}
		//else Itera += 1;//当前迭代次数加1

		if (IsIntersect(curr, CutterBox)) {
			//此时体素节点可能完全位于刀具内部，需要将其清除，而与刀具相交的体素节点则需要再次分割

			Point3f OctreeVertex;
			Point3f SubHalfDimension((curr->halfDimension.x / 2), (curr->halfDimension.y / 2), (curr->halfDimension.z / 2));

			float Xoffset;
			float Yoffset;
			float Zoffset;
			//判断体素的8个点与刀具的空间关系
			for (int i = 0; i <= 7; ++i) {

				OctreeVertex = GetOctreeVertex(i, curr);

				if (IsIn(OctreeVertex, CutterPos)) {
					if (i == 7) {
						//体素的8个顶点全部都被判断在刀具内，说明该体素节点可剔除
						curr->exist = false;//节点剔除
					}
					continue;
				}
				else //只要有一个点在刀具外（无论是刀具全在体素中还是刀具与体素部分相交）就说明该体素节点需要被分割
				{
					if (curr->children[0] == nullptr) {
						curr->SubDivision();//如果该体素还没有分割过，那么将其分解为8个子节点
					}
					for (auto child : curr->children) {
						OctreeLine.push(child);//将体素节点压入栈
						Cutter(CutterPos, Itera, CutterBox);
					}

					break;
				}

			}
		}
		else {
			addtestCube(curr);
			//addCube(curr);//不与刀具相交,将该体素添加进面片集合，然后返回
			return;
		}

		return;

	};
	void ClearFaces() {
		faces.clear();
	}

	//切削循环
	void Loop(Point3f& CutterPos, int& Itera, BoundBox CutterBox) {

		OctreeLine.push(Head);
		BoolNum = 0;
		Device  MyDevice;
		//MyDevice = AllocMemory(BrickLength);
		//Cutter_Dynamic(CutterPos, Itera, CutterBox, MyDevice);
		byte InitCode = -1;
		bool IsSurface = true;
		AddSurfacePoint_Cutter_Dynamic(CutterPos, Itera, CutterBox,InitCode,IsSurface);
	    //AddPoint_Cutter_Dynamic(CutterPos, Itera, CutterBox, MyDevice);
		//IteraAddCube(Head);
		//IteraAddtestCube(Head);
		//AddPoints(Head);
		//AddPointsForBrick512(Head, Itera);
		//AddPointsForBrick4096(Head, Itera);
		//AddPointsForBrick(Head, Itera);
	}


	//迭代添加面片进入testvertices数组中用来绘制被切削的工件
    void IteraAddtestCube(Octree* curr) {

	    if (!curr->sub)
	    {
		Point3f temp;
		for (int i = 0; i <= 7; i++) {
			temp = GetOctreeVertex(i, curr);
			TestTempVertex[i] = temp;
		}   
		/*
		//面片信息                               //拓扑信息
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[5]);
		testVertex.push_back(TestTempVertex[1]); testVertex.push_back(TestTempVertex[7]); 
		testVertex.push_back(TestTempVertex[3]); testVertex.push_back(TestTempVertex[2]);
		testVertex.push_back(TestTempVertex[3]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[2]); testVertex.push_back(TestTempVertex[6]);
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[1]);
		//面片信息                               //拓扑信息
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[5]);
		testVertex.push_back(TestTempVertex[4]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[6]); testVertex.push_back(TestTempVertex[2]);
		testVertex.push_back(TestTempVertex[6]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[2]); testVertex.push_back(TestTempVertex[3]);
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[4]);
		//面片信息                               //拓扑信息
		testVertex.push_back(TestTempVertex[1]); testVertex.push_back(TestTempVertex[0]);
		testVertex.push_back(TestTempVertex[5]); testVertex.push_back(TestTempVertex[4]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[3]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[2]);
		testVertex.push_back(TestTempVertex[3]); testVertex.push_back(TestTempVertex[0]);
		testVertex.push_back(TestTempVertex[1]); testVertex.push_back(TestTempVertex[5]);
		//面片信息                               //拓扑信息
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[6]);
		testVertex.push_back(TestTempVertex[4]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[5]); testVertex.push_back(TestTempVertex[1]);
		testVertex.push_back(TestTempVertex[5]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[1]); testVertex.push_back(TestTempVertex[3]);
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[4]);
		//面片信息                               //拓扑信息
		testVertex.push_back(TestTempVertex[2]); testVertex.push_back(TestTempVertex[0]);
		testVertex.push_back(TestTempVertex[6]); testVertex.push_back(TestTempVertex[4]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[3]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[1]);
		testVertex.push_back(TestTempVertex[3]); testVertex.push_back(TestTempVertex[0]);
		testVertex.push_back(TestTempVertex[2]); testVertex.push_back(TestTempVertex[6]);
		//面片信息                               //拓扑信息
		testVertex.push_back(TestTempVertex[4]); testVertex.push_back(TestTempVertex[0]);
		testVertex.push_back(TestTempVertex[5]); testVertex.push_back(TestTempVertex[1]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[6]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[2]);
		testVertex.push_back(TestTempVertex[6]); testVertex.push_back(TestTempVertex[0]);
		testVertex.push_back(TestTempVertex[4]); testVertex.push_back(TestTempVertex[5]);
		 */
		                          
		testVertex.push_back(TestTempVertex[0]);testVertex.push_back(TestTempVertex[1]);testVertex.push_back(TestTempVertex[3]);
		testVertex.push_back(TestTempVertex[3]);testVertex.push_back(TestTempVertex[2]);testVertex.push_back(TestTempVertex[0]);
                      
		testVertex.push_back(TestTempVertex[0]);testVertex.push_back(TestTempVertex[4]);testVertex.push_back(TestTempVertex[6]);
		testVertex.push_back(TestTempVertex[6]);testVertex.push_back(TestTempVertex[2]);testVertex.push_back(TestTempVertex[0]);
                         
		testVertex.push_back(TestTempVertex[1]);testVertex.push_back(TestTempVertex[5]);testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[7]);testVertex.push_back(TestTempVertex[3]);testVertex.push_back(TestTempVertex[1]);
                          
		testVertex.push_back(TestTempVertex[0]);testVertex.push_back(TestTempVertex[4]);testVertex.push_back(TestTempVertex[5]);
		testVertex.push_back(TestTempVertex[5]);testVertex.push_back(TestTempVertex[1]);testVertex.push_back(TestTempVertex[0]);
                        
		testVertex.push_back(TestTempVertex[2]);testVertex.push_back(TestTempVertex[6]);testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[7]);testVertex.push_back(TestTempVertex[3]);testVertex.push_back(TestTempVertex[2]);
                         
		testVertex.push_back(TestTempVertex[4]);testVertex.push_back(TestTempVertex[5]);testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[7]);testVertex.push_back(TestTempVertex[6]);testVertex.push_back(TestTempVertex[4]);

	    }
	    else 
		{
		    //if (!curr->sub) return;//判断是否存在子节点
		    for (auto child : curr->children) {
			  if (child->exist) IteraAddtestCube(child);
			  
		    }
	    }
	return;

}
	void IteraAddCube(Octree* curr) {

		if (!curr->sub)
		{
			Point3f temp;
			for (int i = 0; i <= 7; i++) {
				temp = GetOctreeVertex(i, curr);
				TempVertex[i] = getVertexIndex(temp);
			}
			faces.push_back(Point3i(TempVertex[0], TempVertex[1], TempVertex[3]));//不考虑面片的正反面，建议不要开启遮挡剔除（深度测试？）
			faces.push_back(Point3i(TempVertex[3], TempVertex[2], TempVertex[0]));
			faces.push_back(Point3i(TempVertex[0], TempVertex[4], TempVertex[6]));//之后还要考虑添加的法向量的正负
			faces.push_back(Point3i(TempVertex[6], TempVertex[2], TempVertex[0]));
			faces.push_back(Point3i(TempVertex[1], TempVertex[5], TempVertex[7]));//
			faces.push_back(Point3i(TempVertex[7], TempVertex[3], TempVertex[1]));
			faces.push_back(Point3i(TempVertex[0], TempVertex[4], TempVertex[5]));//
			faces.push_back(Point3i(TempVertex[5], TempVertex[1], TempVertex[0]));
			faces.push_back(Point3i(TempVertex[2], TempVertex[6], TempVertex[7]));//
			faces.push_back(Point3i(TempVertex[7], TempVertex[3], TempVertex[2]));
			faces.push_back(Point3i(TempVertex[4], TempVertex[5], TempVertex[7]));//
			faces.push_back(Point3i(TempVertex[7], TempVertex[6], TempVertex[4]));
		}
		else
		{
			//if (!curr->sub) return;//判断是否存在子节点
			for (auto child : curr->children) {
				if (child->exist) IteraAddCube(child);

			}
		}
		return;

	}


	void AddPoints(Octree* curr) {
		testVertex.clear();
		IteraAddPoints(curr);
	}
	void IteraAddPoints(Octree* curr) {
		if (!curr->sub) {
			Point3f temp;
			temp = GetOctreeVertex(0, curr);
			testVertex.push_back(temp); testVertex.push_back(curr->halfDimension);
			temp = GetOctreeVertex(7, curr);
			testVertex.push_back(temp); testVertex.push_back(curr->halfDimension * (-1));
		}
		else
		{
			//if (!curr->sub) return;//判断是否存在子节点
			for (auto child : curr->children) {
				if (child->exist) IteraAddPoints(child);

			}
		}
	}

	//可能需要的的：针对拥有brick结构的八叉树，输入八叉树头部节点和最大迭代次数，遍历八叉树然后加入可渲染队列，针对的bricks容量为64
	void AddPointsForBrick(Octree* curr, int& Itera) {
		testVertex.clear();//清空当前可渲染队列
		IteraAddPointsForBrick(curr, Itera);
	}

	//可能需要的：递归函数，通过深入八叉树节点的下一个子节点，达到遍历八叉树的目的，对于不拥有brick的非叶子节点，将其代表的体素直接加入可渲染队列
	//对于拥有bricks结构的叶子节点，获取其指向的bricks对象，并遍历bricks，获取需要渲染的体素将其加入可渲染队列，针对的bricks容量为64；
	void IteraAddPointsForBrick(Octree* curr, int& Itera) {
		if (!curr->sub) {
			if (curr->accuracy > Itera && curr->exist)
			{
				Bricks* Temp = (Bricks*)curr->bricks;
				Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 3 / 4,
					curr->origin.y - curr->halfDimension.y * 3 / 4, curr->origin.z - curr->halfDimension.z * 3 / 4);
				Point3f SubTemp;
				Point3f halfDimension = Point3f(curr->halfDimension.x /4, curr->halfDimension.y / 4, curr->halfDimension.z / 4);
				Point3f OctreeVertex;
				for (int i = 0; i < BrickLength; ++i) {
					if (!Temp->brick[i]) {
						SubTemp = SubOrigin;

						SubTemp.x = SubTemp.x + (i & 3) * curr->halfDimension.x / 2  ;
						SubTemp.y = SubTemp.y + (i & 12) / 4 * curr->halfDimension.y / 2  ;
						SubTemp.z = SubTemp.z + (i & 48) / 16 * curr->halfDimension.z  / 2;
						/*
						SubTemp.x = SubTemp.x + (i & 3) * curr->halfDimension.x / 2;
						SubTemp.y = SubTemp.y + (i & 12) / 4 * curr->halfDimension.y / 2;
						SubTemp.z = SubTemp.z + (i & 48) / 16 * curr->halfDimension.z / 2;
						*/
						OctreeVertex = GetOctreeVertexForBrick(0, SubTemp, halfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension);

						OctreeVertex = GetOctreeVertexForBrick(7, SubTemp, halfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension * -1);
					}
				}
				
			}
			else {
				Point3f temp;
				temp = GetOctreeVertex(0, curr);
				testVertex.push_back(temp); testVertex.push_back(curr->halfDimension);
				temp = GetOctreeVertex(7, curr);
				testVertex.push_back(temp); testVertex.push_back(curr->halfDimension * (-1));
			}
		}
		else
		{
			//if (!curr->sub) return;//判断是否存在子节点
			for (auto child : curr->children) {
				if (child && (child->exist)) IteraAddPointsForBrick(child, Itera);//

			}
		}
	}


	//可能需要的：最初应用bricks结构时使用的针对bricks结构的分割函数，针对的bricks容量较小，只有64，目前使用的bricks容量为512
	void Deeper_cut(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos) {

		Bricks* bri = (Bricks*)curr->bricks;
		//Octree* temp = (Octree*)bri->brick;
		Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 3 / 4,
			curr->origin.y - curr->halfDimension.y * 3 / 4, curr->origin.z - curr->halfDimension.z * 3 / 4);
		Point3f SubTemp;
		Point3f halfDimension = Point3f(curr->halfDimension.x / 4, curr->halfDimension.y / 4, curr->halfDimension.z / 4);
		//cout << "精度为:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";
		for (int k = 0; k < BrickLength; ++k) {
			if (bri->brick[k]) continue;
			SubTemp = SubOrigin;
			SubTemp.x = SubTemp.x + (k & 3) * curr->halfDimension.x / 2;
			SubTemp.y = SubTemp.y + (k & 12) / 4 * curr->halfDimension.y / 2;
			SubTemp.z = SubTemp.z + (k & 48) / 16 * curr->halfDimension.z / 2;

			Point3f OctreeVertex;
			//或许可以通过判断体素中心与刀具中心的距离来判断其是否应该被剔除，看体素中心是否在刀具内部
			if (IsIntersectForBrick(SubTemp, halfDimension, CutterBox)) {

				for (int i = 0; i <= 7; ++i) {
					OctreeVertex = GetOctreeVertexForBrick(i, SubTemp, halfDimension);
					if (IsIn(OctreeVertex, CutterPos)) {
						if (i == 7) {
							bri->brick[k] = true;
							break;
						}
						continue;
					}
				}
				//if (IsIn(SubTemp, CutterPos)) {bri->brick[k] = true;}
				//else {bri->brick[k] = true;}
			}
			//else{bri->brick[k] = true;}
		}
	}
	
	//可能需要的：将将可渲染体素加入可渲染队列，针对的bricks容量为512
	void AddPointsForBrick512(Octree* curr, int& Itera) {
		testVertex.clear();
		IteraAddPointsForBrick512(curr, Itera);
	}

	void IteraAddPointsForBrick512(Octree* curr, int& Itera) {
		if (!curr->sub) {
			if (curr->accuracy > Itera && curr->bricks)
			{
				//cout << "开始";
				Bricks* Temp = (Bricks*)curr->bricks;
				float precision = 0.125;

				Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 * precision,
					curr->origin.y - curr->halfDimension.y * 7 * precision, curr->origin.z - curr->halfDimension.z * 7 * precision);
				Point3f SubTemp;

				Point3f halfDimension = Point3f(curr->halfDimension.x * precision, curr->halfDimension.y * precision, curr->halfDimension.z * precision);
				Point3f OctreeVertex;

				for (int i = 0; i < BrickLength; ++i) {

					if (!Temp->brick[i]) {
						SubTemp = SubOrigin;

						SubTemp.x = SubTemp.x + (i & 7) * curr->halfDimension.x / 4;
						SubTemp.y = SubTemp.y + (i & 56) / 8 * curr->halfDimension.y / 4;
						SubTemp.z = SubTemp.z + (i & 448) / 64 * curr->halfDimension.z / 4;

						OctreeVertex = GetOctreeVertexForBrick(0, SubTemp, halfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension);
						OctreeVertex = GetOctreeVertexForBrick(7, SubTemp, halfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension * -1);
						//cout << "添加成功";
					}
				}

			}
			else {
				Point3f temp;
				temp = GetOctreeVertex(0, curr);
				testVertex.push_back(temp); testVertex.push_back(curr->halfDimension);
				temp = GetOctreeVertex(7, curr);
				testVertex.push_back(temp); testVertex.push_back(curr->halfDimension * (-1));
			}
		}
		else
		{
			//if (!curr->sub) return;//判断是否存在子节点
			for (auto child : curr->children) {
				//if (child->exist) IteraAddPointsForBrick512(child, Itera);
				if (child && (child->exist)) IteraAddPointsForBrick512(child, Itera);

			}
		}
	}


	//可能需要的：对达到分割极限的体素再次分割，使用bricks结构，输入已经达到分割极限的八叉树叶子节点，遍历其所指向的bricks结构，
    //判断该bricks中代表的每个是否处于刀具内部，在刀具内部则将该位置的bool值设为true，代表不需要被渲染；
    //bricks结构中的rendered数组目前用于动态更新可渲染队列，你应该还不需要用到
	void Deeper_cut512(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos) {
		Bricks* bri = (Bricks*)curr->bricks;
		//Octree* temp = (Octree*)bri->brick;

		float precision = 0.125;

		Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 * precision,
			curr->origin.y - curr->halfDimension.y * 7 * precision, curr->origin.z - curr->halfDimension.z * 7 * precision);
		Point3f SubTemp;
		Point3f halfDimension = Point3f(curr->halfDimension.x * precision, curr->halfDimension.y * precision, curr->halfDimension.z * precision);
		//cout << "精度为:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";
		for (int k = 0; k < BrickLength; ++k) {
			if (bri->brick[k]) continue;
			SubTemp = SubOrigin;
			SubTemp.x = SubTemp.x + (k & 7) *        curr->halfDimension.x / 4;
			SubTemp.y = SubTemp.y + (k & 56) / 8 *   curr->halfDimension.x / 4;
			SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.x / 4;

			Point3f OctreeVertex;
				if (IsIn(SubTemp, CutterPos)) {

					bri->brick[k] = true;
				}
		}
	}

	//如果不是surface那直接按照是否相交添加进渲染列表
	void Deeper_cut512_ForSurface(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos,byte CurrCode, bool IsSurface) {
		Bricks* bri = (Bricks*)curr->bricks;
		//Octree* temp = (Octree*)bri->brick;

		float precision = 0.125;

		Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 * precision,
			curr->origin.y - curr->halfDimension.y * 7 * precision, curr->origin.z - curr->halfDimension.z * 7 * precision);
		Point3f SubTemp;
		Point3f halfDimension = Point3f(curr->halfDimension.x * precision, curr->halfDimension.y * precision, curr->halfDimension.z * precision);
		//cout << "精度为:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";
		SubTemp = SubOrigin;
		if (!IsSurface) {
			for (int k = 0; k < BrickLength; ++k) {
				if (bri->brick[k]) continue;
				SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
				SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.x / 4;
				SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.x / 4;
				if (IsIn(SubTemp, CutterPos)) {
					bri->brick[k] = true;
					continue;
				}

				if (IsIntersectForBrick(SubTemp, halfDimension, CutterBox)) {
					bri->Rendered[k] = true;
				}
			}
		}
		else {
			byte Mask = 1;
			int Index;
			int t;//计算标志是+还是-;
			int n;//计算标志是x或y或z;
			int mask1 = 1;
			int mask2 = 1;
			for (Index = 0; Index < 6; Index++) {
				Mask = Mask << Index;
				Mask = CurrCode & Mask;
				if (!Mask) break;
			}
			//推算出是哪个面后CurrCode已经无用了，后续可用于中间变量
			for (int k = 0; k < BrickLength; ++k) {
				if (bri->brick[k]) continue;
				SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
				SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.x / 4;
				SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.x / 4;

				if (IsIn(SubTemp, CutterPos)) {
					bri->brick[k] = true;//
					bri->Rendered[k] = false;
					continue;
				}

				if (IsIntersectForBrick(SubTemp, halfDimension, CutterBox)) {
					bri->Rendered[k] =true;//加入渲染队列
				}
				else {
					t = Index % 2;
					n = (Index / 2) * 3;

					mask1 = 1;
					mask2 = 1;

					mask1 = mask1 << n;
					mask2 << (n + 1);
					mask1 = mask2 | mask1;
					mask2 = 1;
					mask2 << (n + 2);
					mask1 = mask1 | mask2;//mask转变为000 111 000  /  111 000 111 /  000 000 111

					mask2 = k & mask1;//mask2如果为  111 000 000 / 000 111 000  /  000 000 111 或者0;则在表面；

					if (t) {
						if (mask2 == mask1) bri->Rendered[k] = true;;
					}
					else {
						if (mask2 == 0) bri->Rendered[k] = true;
					}
				}
			}
		}
	}
	void AddPoint_Deeper_cut512(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos) {
		Bricks* bri = (Bricks*)curr->bricks;
		//Octree* temp = (Octree*)bri->brick;

		float precision = 0.125;

		Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 * precision,
			curr->origin.y - curr->halfDimension.y * 7 * precision, curr->origin.z - curr->halfDimension.z * 7 * precision);
		Point3f SubTemp;
		Point3f halfDimension = Point3f(curr->halfDimension.x * precision, curr->halfDimension.y * precision, curr->halfDimension.z * precision);
		//cout << "精度为:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";

		Point3f OctreeVertex ;
/* 
* 此处因区分两种情况该brick还未被加入过，此时应使用push_back，此时无论它是否被剔除都需要加入可渲染列表，也就是必须为512，
  如果该brick已经在可渲染列表中，此时需要通过计算索引来获取它在渲染列表中的位置；

*/
		if (curr->RenderedIndex == (-1))
		{
			curr->RenderedIndex = testVertex.size();
			for (int k = 0; k < BrickLength; ++k) {
				//if (bri->brick[k]) continue;
				SubTemp = SubOrigin;
				SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
				SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.x / 4;
				SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.x / 4;


				//或许可以通过判断体素中心与刀具中心的距离来判断其是否应该被剔除，看体素中心是否在刀具内部

				if (IsIn(SubTemp, CutterPos)) { 
					bri->brick[k] = true; 
					bri->sum = bri->sum - 1;
					bri->Rendered[k] = true;
					continue;
				}
				OctreeVertex = GetOctreeVertexForBrick(0, SubTemp, halfDimension);
				testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension);
				OctreeVertex = GetOctreeVertexForBrick(7, SubTemp, halfDimension);
				testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension * (-1));
				//else{bri->brick[k] = true;}
			}
		}
		else {
			int BrickIndex = curr->RenderedIndex - 4;

			for (int k = 0; k < BrickLength; ++k) {

				if (!bri->Rendered[k]) BrickIndex += 4;//达到下一个体素所在索引
				if (bri->brick[k]) continue;

				SubTemp = SubOrigin;
				SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
				SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.x / 4;
				SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.x / 4;

				//或许可以通过判断体素中心与刀具中心的距离来判断其是否应该被剔除，看体素中心是否在刀具内部
				if (IsIn(SubTemp, CutterPos)) { 
					bri->brick[k] = true; 
					bri->sum = bri->sum-1;
					testVertex[BrickIndex + 1] = Point3f(0, 0, 0);
					testVertex[BrickIndex + 3] = Point3f(0, 0, 0);
				}
				//else{bri->brick[k] = true;}
			}

			if (bri->sum==0) DeleteNode(curr);

		}
	}
	void RebuildRenderList(Octree* curr, int& Itera) {


		if (curr->accuracy > Itera && curr->bricks)
		{
			//cout << "开始";
			Bricks* Temp = (Bricks*)curr->bricks;
			float precision = 0.125;

			Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 * precision,
				curr->origin.y - curr->halfDimension.y * 7 * precision, curr->origin.z - curr->halfDimension.z * 7 * precision);
			Point3f SubTemp;

			Point3f halfDimension = Point3f(curr->halfDimension.x * precision, curr->halfDimension.y * precision, curr->halfDimension.z * precision);
			Point3f OctreeVertex;

			curr->RenderedIndex = testVertex.size();

			for (int i = 0; i < BrickLength; ++i) {

				Temp->Rendered[i] = Temp->brick[i];
				if (!Temp->brick[i]) {

					SubTemp = SubOrigin;
					SubTemp.x = SubTemp.x + (i & 7) * curr->halfDimension.x / 4;
					SubTemp.y = SubTemp.y + (i & 56) / 8 * curr->halfDimension.y / 4;
					SubTemp.z = SubTemp.z + (i & 448) / 64 * curr->halfDimension.z / 4;

					OctreeVertex = GetOctreeVertexForBrick(0, SubTemp, halfDimension);
					testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension);
					OctreeVertex = GetOctreeVertexForBrick(7, SubTemp, halfDimension);
					testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension * -1);
				}

			}
			return;
		}

		if (!curr->sub) {
			if (curr->RenderedIndex >= 0) {
				curr->RenderedIndex = testVertex.size();
				Point3f temp;
				temp = GetOctreeVertex(0, curr);
				testVertex.push_back(temp); testVertex.push_back(curr->halfDimension);
				temp = GetOctreeVertex(7, curr);
				testVertex.push_back(temp); testVertex.push_back(curr->halfDimension * (-1));
			}
			
		}
		else
		{
			curr->RenderedIndex = -1;
			//if (!curr->sub) return;//判断是否存在子节点
			for (auto child : curr->children) {
				//if (child) RebuildRenderList(child, Itera);
				if (child && (child->exist)) RebuildRenderList(child, Itera);
			}
		}
	}

	//针对相交的和处于表面的brick，对于由于没有处于相交面而未能加入可渲染列表但是可能在下一次切削中处于相交面的brick，
	//因为可渲染列表中已经没有其位置，因此为它们设置一个备用可渲染列表，问题在于如何定位该列表中的brick;
	void AddSurfacePoint_Deeper_cut512(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos, byte CurrCode) {
		Bricks* bri = (Bricks*)curr->bricks;
		//Octree* temp = (Octree*)bri->brick;

		float precision = 0.125;

		Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 * precision,
			curr->origin.y - curr->halfDimension.y * 7 * precision, curr->origin.z - curr->halfDimension.z * 7 * precision);
		Point3f SubTemp;
		Point3f halfDimension = Point3f(curr->halfDimension.x * precision, curr->halfDimension.y * precision, curr->halfDimension.z * precision);
		//cout << "精度为:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";

		Point3f OctreeVertex;
		/*
		* 此处因区分两种情况该brick还未被加入过，此时应使用push_back，此时无论它是否被剔除都需要加入可渲染列表，也就是必须为512，
		  如果该brick已经在可渲染列表中，此时需要通过计算索引来获取它在渲染列表中的位置；

		*/

		bool Is_Intersect;
		if (curr->RenderedIndex == (-1))
		{
			curr->RenderedIndex = testVertex.size();
			for (int k = 0; k < BrickLength; ++k) {
				//if (bri->brick[k]) continue;
				SubTemp = SubOrigin;
				SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
				SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.x / 4;
				SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.x / 4;


				//或许可以通过判断体素中心与刀具中心的距离来判断其是否应该被剔除，看体素中心是否在刀具内部
				
				if (IsIn(SubTemp, CutterPos)) {
					bri->brick[k] = true;
					bri->sum = bri->sum - 1;
					bri->Rendered[k] = true;
					continue;
				}
				
				OctreeVertex = GetOctreeVertexForBrick(0, SubTemp, halfDimension);
				testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension);
				OctreeVertex = GetOctreeVertexForBrick(7, SubTemp, halfDimension);
				testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension * (-1));
				

				//else{bri->brick[k] = true;}
			}
		}
		else {
			int BrickIndex = curr->RenderedIndex - 4;

			for (int k = 0; k < BrickLength; ++k) {

				if (!bri->Rendered[k]) BrickIndex += 4;//达到下一个体素所在索引
				if (bri->brick[k]) continue;

				SubTemp = SubOrigin;
				SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
				SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.x / 4;
				SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.x / 4;

				//或许可以通过判断体素中心与刀具中心的距离来判断其是否应该被剔除，看体素中心是否在刀具内部
				if (IsIn(SubTemp, CutterPos)) {
					bri->brick[k] = true;
					bri->sum = bri->sum - 1;
					testVertex[BrickIndex + 1] = Point3f(0, 0, 0);
					testVertex[BrickIndex + 3] = Point3f(0, 0, 0);
				}
				//else{bri->brick[k] = true;}
			}

			if (bri->sum == 0) DeleteNode(curr);

		}
	}


	void AddPointsForBrick4096(Octree* curr, int& Itera) {
		testVertex.clear();
		IteraAddPointsForBrick4096(curr, Itera);
	}
	void IteraAddPointsForBrick4096(Octree* curr, int& Itera) {
		if (!curr->sub) {
			if (curr->accuracy > Itera)
			{
				//cout << "开始";
				Bricks* Temp = (Bricks*)curr->bricks;
				float precision = 0.0625;

				Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 15 /16,
					curr->origin.y - curr->halfDimension.y * 15 /16, curr->origin.z - curr->halfDimension.z * 15 /16);
				Point3f SubTemp;

				Point3f halfDimension = Point3f(curr->halfDimension.x /16, curr->halfDimension.y /16, curr->halfDimension.z /16);
				Point3f OctreeVertex;
	
				for (int i = 0; i < BrickLength; ++i) {
					//cout << i << " ";
					if (!Temp->brick[i]) {
						SubTemp = SubOrigin;

						SubTemp.x = SubTemp.x + (i & 15) * curr->halfDimension.x / 8;
						SubTemp.y = SubTemp.y + (i & 240) / 16 * curr->halfDimension.y / 8;
						SubTemp.z = SubTemp.z + (i & 3840) / 256 * curr->halfDimension.z / 8;
						//cout << "SubTemp:(" << (i & 15) << "," << (i & 240) / 16 << "," << (i & 3840) / 256 << ")" << endl;
						OctreeVertex = GetOctreeVertexForBrick(0, SubTemp, halfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension);

						OctreeVertex = GetOctreeVertexForBrick(7, SubTemp, halfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension * -1);
						//cout << "添加成功";
					}
				}

				cout << "" << endl;
				
				//cout << endl;
			}
			else {
				Point3f temp;
				temp = GetOctreeVertex(0, curr);
				testVertex.push_back(temp); testVertex.push_back(curr->halfDimension);
				temp = GetOctreeVertex(7, curr);
				testVertex.push_back(temp); testVertex.push_back(curr->halfDimension * (-1));
			}
		}
		else
		{
			//if (!curr->sub) return;//判断是否存在子节点
			for (auto child : curr->children) {
				if (child) IteraAddPointsForBrick4096(child, Itera);

			}
		}
	}

	//删除已经被剔除的体素节点，并向上追溯其父亲节点

	void FindParent(Octree* Parent,int index) {
		delete Parent->children[index];
		Parent->children[index] = nullptr;
		Parent->exist = Parent->exist - 1;
		//Parent->exist = max(Parent->exist, 0);
		if (!Parent->exist) {
			DeleteNode(Parent);
		}
	}

	void DeleteNode(Octree* curr) {
		//curr->exist = false;
		if (curr->bricks != nullptr) {
			Bricks* temp = (Bricks*)curr->bricks;
			delete temp;
			//cout << "删除brick" << endl;
			curr->bricks = nullptr;
		}
		FindParent((Octree*)(curr->Parent), curr->index);
		
	}

	//寻找当前被删除节点中还未被删除的含有brick的叶子节点
	void FindBrickChild(Octree* curr,int Itera) {
		if (curr->accuracy > Itera) {
			Bricks* temp = (Bricks*)curr->bricks;
			if (temp->sum > 0) {
				BrickChildToBeDelete.push(curr);
			}
			return;
		}
		for (auto child : curr->children) {

			if (child && child->accuracy <= Itera) {
				FindBrickChild(child, Itera);
			}
		}
	}
	//删除上一个函数所含有的brick在可渲染列表中的数据
	void DeleteBrick() {
		Bricks* temp;
		int BrickIndex;
		while (!BrickChildToBeDelete.empty()) {
			temp = (Bricks*)BrickChildToBeDelete.top()->bricks;

			BrickIndex = BrickChildToBeDelete.top()->RenderedIndex - 4;

			for (int k = 0; k < BrickLength; ++k) {
				if (!temp->Rendered[k]) BrickIndex += 4;//达到下一个体素所在索引
				if (temp->brick[k]) continue;
				testVertex[BrickIndex + 1] = Point3f(0, 0, 0);
				testVertex[BrickIndex + 3] = Point3f(0, 0, 0);
			}
			delete temp;
			delete BrickChildToBeDelete.top();
			cout << "删除brick和叶子" << endl;
			BrickChildToBeDelete.pop();
		}
	}


	//可能需要的：使用刀具分割切削八叉树，获得切削后的八叉树节点
    void Cutter_Dynamic(Point3f& CutterPos, int& Itera, BoundBox& CutterBox, Device& MyDevice) {
        if (OctreeLine.empty()) return;
        
        Octree* curr = OctreeLine.top();
        OctreeLine.pop();

        //if (curr == nullptr || curr->exist == false) { return; };
        
		bool is_intersect = IsIntersect(curr, CutterBox);

        //if (Itera > Iteration || curr->accuracy > Iteration )
		//当八叉树节点达到分割极限后
		if (curr->accuracy > Itera )//&& is_intersect
		{
			//如果当前八叉树叶子节点还没有被分配bricks结构，那么为它申请一个
			if (curr->bricks == nullptr) {
				curr->bricks = new Bricks(curr);
			}
			//Init_Data4096(curr, CutterBox, CutterPos, CutterSize, MyDevice);
			//Init_Data512(curr, CutterBox, CutterPos, CutterSize);
			//w为当前八叉树节点执行更深入的分割，bricks容量为512
			Deeper_cut512(curr, CutterBox, CutterPos);
			//Deeper_cut(curr, CutterBox, CutterPos);
             //迭代次数达到最大，不可再分割，可以直接渲染,返回
        	return;
        }
        //else Itera += 1;//当前迭代次数加1
        
        if (is_intersect) {
        	//此时体素节点可能完全位于刀具内部，需要将其清除，而与刀具相交的体素节点则需要再次分割
        	//curr->rendered = false;
			BoolNum++;
        	Point3f OctreeVertex;
        	Point3f SubHalfDimension((curr->halfDimension.x / 2), (curr->halfDimension.y / 2), (curr->halfDimension.z / 2));
        
        	float Xoffset, Yoffset,Zoffset;

        	//判断体素的8个点与刀具的空间关系
        	for (int i = 0; i <= 7; ++i) {
        		OctreeVertex = GetOctreeVertex(i, curr);
        		if (IsIn(OctreeVertex, CutterPos)) {
        			if (i == 7) {
        				//体素的8个顶点全部都被判断在刀具内，说明该体素节点可剔除
						curr->exist = false;//节点剔除
						/*if (curr->bricks != nullptr) {
							Bricks* temp = (Bricks*)curr->bricks;
							delete temp;
							curr->bricks = nullptr;
						}
						*/
						DeleteNode(curr);
						VoxelNum -= 1;
        			}
        			continue;
        		}
        		else //只要有一个点在刀具外（无论是刀具全在体素中还是刀具与体素部分相交）就说明该体素节点需要被分割
        		{
        			
        			if (curr->sub==false) {
        				curr->SubDivision();//如果该体素还没有分割过，那么将其分解为8个子节点
						VoxelNum += 7;//体素节点增加8个
        			}
        			for (auto child : curr->children) {
						if (!child) continue;
        				OctreeLine.push(child);//将体素节点压入栈
        				Cutter_Dynamic(CutterPos, Itera, CutterBox, MyDevice);
        			}
					//Cutter_Dynamic(CutterPos, Itera, CutterBox);
        			break;
        		}
        
        	}
        }
        else {
        	return;
        }
        
        return;
        
     };

	void Cutter_Dynamic_Surface(Point3f& CutterPos, int& Itera, BoundBox& CutterBox, byte CurrCode, bool IsSurface) {
		if (OctreeLine.empty()) return;

		Octree* curr = OctreeLine.top();
		OctreeLine.pop();

		//if (curr == nullptr || curr->exist == false) { return; };

		bool is_intersect = IsIntersect(curr, CutterBox);

		//if (Itera > Iteration || curr->accuracy > Iteration )
		//当八叉树节点达到分割极限后
		if (curr->accuracy > Itera )//&& is_intersect
		{
			//如果当前八叉树叶子节点还没有被分配bricks结构，那么为它申请一个
			if (curr->bricks == nullptr) {
				curr->bricks = new Bricks(curr);
			}
			//Init_Data4096(curr, CutterBox, CutterPos, CutterSize, MyDevice);
			//Init_Data512(curr, CutterBox, CutterPos, CutterSize);
			//w为当前八叉树节点执行更深入的分割，bricks容量为512
			Deeper_cut512(curr, CutterBox, CutterPos);
			//Deeper_cut(curr, CutterBox, CutterPos);
			 //迭代次数达到最大，不可再分割，可以直接渲染,返回
			return;
		}
		//else Itera += 1;//当前迭代次数加1

		if (is_intersect) {
			//此时体素节点可能完全位于刀具内部，需要将其清除，而与刀具相交的体素节点则需要再次分割
			//curr->rendered = false;
			BoolNum++;
			Point3f OctreeVertex;
			Point3f SubHalfDimension((curr->halfDimension.x / 2), (curr->halfDimension.y / 2), (curr->halfDimension.z / 2));

			float Xoffset, Yoffset, Zoffset;

			//判断体素的8个点与刀具的空间关系
			for (int i = 0; i <= 7; ++i) {
				OctreeVertex = GetOctreeVertex(i, curr);
				if (IsIn(OctreeVertex, CutterPos)) {
					if (i == 7) {
						//体素的8个顶点全部都被判断在刀具内，说明该体素节点可剔除
						curr->exist = false;//节点剔除
						DeleteNode(curr);
						VoxelNum -= 1;
					}
					continue;
				}
				else //只要有一个点在刀具外（无论是刀具全在体素中还是刀具与体素部分相交）就说明该体素节点需要被分割
				{
					if (curr->sub == false) {
						curr->SubDivision();//如果该体素还没有分割过，那么将其分解为8个子节点
						VoxelNum += 7;//体素节点增加8个
					}

					byte ChildCode = 1;
					byte Mask = 1;
					for (auto child : curr->children) {
						if (!child) continue;
						OctreeLine.push(child);//将体素节点压入栈
						if (IsSurface) {
							//初始化
							ChildCode = 1;
							Mask = 1;
							//获取Z维度的正负信息
							ChildCode = ChildCode << (i & 1) + 4;

							//获取Y维度的正负信息
							Mask = Mask << (2 + (i & 2) / 2);
							ChildCode = ChildCode | Mask;
							Mask = 1;

							//获取X维度的正负信息
							Mask = Mask << ((i & 4) / 4);
							ChildCode = ChildCode | Mask;

							ChildCode = CurrCode & ChildCode;

							if (!ChildCode) {
								IsSurface = false;
							}
						}
						child->RenderedIndex = IsSurface;
						Cutter_Dynamic_Surface(CutterPos, Itera, CutterBox, ChildCode, IsSurface);
					}
					//Cutter_Dynamic(CutterPos, Itera, CutterBox);
					break;
				}

			}
		}
		else {
			curr->RenderedIndex = false;
			return;
		}

		return;

	};
	void AddPoint_Cutter_Dynamic(Point3f& CutterPos, int& Itera, BoundBox& CutterBox, Device& MyDevice) {

		if (OctreeLine.empty()) return;

		Octree* curr = OctreeLine.top();
		OctreeLine.pop();
		//if (curr == nullptr || curr->exist == false) { return; };

		bool is_intersect = IsIntersect(curr, CutterBox);

		Point3f OctreeVertex = Point3f(0,0,0);
		//if (Itera > Iteration || curr->accuracy > Iteration )

		if (curr->accuracy > Itera)//&& is_intersect
		{
			if (curr->bricks == nullptr) {
				curr->bricks = new Bricks(curr);
			}
			curr->sub = true;
			//Init_Data4096(curr, CutterBox, CutterPos, CutterSize, MyDevice);
			//Init_Data512(curr, CutterBox, CutterPos, CutterSize);
			AddPoint_Deeper_cut512(curr, CutterBox, CutterPos);
			//Deeper_cut(curr, CutterBox, CutterPos);
			 //迭代次数达到最大，不可再分割，可以直接渲染,返回
			return;
		}


		//else Itera += 1;//当前迭代次数加1

		if (is_intersect) {
			//此时体素节点可能完全位于刀具内部，需要将其清除，而与刀具相交的体素节点则需要再次分割
			//curr->rendered = false;
			BoolNum++;
			//Point3f OctreeVertex;
			Point3f SubHalfDimension((curr->halfDimension.x / 2), (curr->halfDimension.y / 2), (curr->halfDimension.z / 2));

			float Xoffset, Yoffset, Zoffset;

			//判断体素的8个点与刀具的空间关系
			for (int i = 0; i <= 7; ++i) {
				OctreeVertex = GetOctreeVertex(i, curr);
				if (IsIn(OctreeVertex, CutterPos)) {
					if (i == 7) {
						//体素的8个顶点全部都被判断在刀具内，说明该体素节点可剔除
						curr->exist = false;//节点剔除
						if (curr->RenderedIndex >=0) {
							
							testVertex[(curr->RenderedIndex) + 1] = Point3f(0, 0, 0);
							testVertex[(curr->RenderedIndex) + 3] = Point3f(0, 0, 0);
							curr->RenderedIndex = -10;
						}
						//FindBrickChild(curr, Itera);
						//DeleteBrick();
						DeleteNode(curr);
						VoxelNum -= 1;
					}
					continue;
				}
				else //只要有一个点在刀具外（无论是刀具全在体素中还是刀具与体素部分相交）就说明该体素节点需要被分割
				{
					if (curr->sub == false) {
						curr->SubDivision();//如果该体素还没有分割过，那么将其分解为8个子节点
						VoxelNum += 7;//体素节点增加8个
					}
					if (curr->RenderedIndex >=0) {
						
						testVertex[(curr->RenderedIndex) + 1] = Point3f(0, 0, 0);
						testVertex[(curr->RenderedIndex) + 3] = Point3f(0, 0, 0);
						curr->RenderedIndex = -10;
					}


					for (auto child : curr->children) {
						if (!child) continue;
						OctreeLine.push(child);//将体素节点压入栈
						AddPoint_Cutter_Dynamic(CutterPos, Itera, CutterBox, MyDevice);
					}
					//Cutter_Dynamic(CutterPos, Itera, CutterBox);
					break;
				}

			}
		}
		else 
		{
			if (curr->sub || curr->RenderedIndex == (-10)) return;//当该体素已经被分割时，不再将它加入可渲染队列
			if (curr->RenderedIndex == (-1)) {//当还未被分割的体素还未加可渲染队列时，将其加入可渲染队列,-10代表已加入但是又被删除
				curr->RenderedIndex = testVertex.size();
				OctreeVertex = GetOctreeVertex(0, curr);
				testVertex.push_back(OctreeVertex); testVertex.push_back(curr->halfDimension);
				OctreeVertex = GetOctreeVertex(7, curr);
				testVertex.push_back(OctreeVertex); testVertex.push_back(curr->halfDimension * (-1));
			}
			return;
		}

		



		return;

	};

	void AddSurfacePoint_Cutter_Dynamic(Point3f& CutterPos, int& Itera, BoundBox& CutterBox,byte CurrCode,bool IsSurface) {
		//Currcode指的是curr节点的编码，通过这个编码与curr的子节点的的编码处理，得到其子节点是否应该被加入
		//IsRendered代表该节点是否该被加入渲染列表，1-6代表它的裸露方向，IsRendered/2获取是哪个
		if (OctreeLine.empty()) return;
		
		Octree* curr = OctreeLine.top();
		OctreeLine.pop();
		//if (curr == nullptr || curr->exist == false) { return; };

		bool is_intersect = IsIntersect(curr, CutterBox);

		Point3f OctreeVertex = Point3f(0, 0, 0);

		if (curr->accuracy > Itera )//&& is_intersect
		{
			//相交或者处于表面
			if (is_intersect || IsSurface) {
				if (curr->bricks == nullptr) {
					curr->bricks = new Bricks(curr);
				}
				curr->sub = true;
				AddPoint_Deeper_cut512(curr, CutterBox, CutterPos);
			}
			 //迭代次数达到最大，不可再分割，可以直接渲染,返回
			return;
		}

		//else Itera += 1;//当前迭代次数加1

		if (is_intersect) {
			//此时体素节点可能完全位于刀具内部，需要将其清除，而与刀具相交的体素节点则需要再次分割
			//curr->rendered = false;
			BoolNum++;
			//Point3f OctreeVertex;
			Point3f SubHalfDimension((curr->halfDimension.x / 2), (curr->halfDimension.y / 2), (curr->halfDimension.z / 2));

			float Xoffset, Yoffset, Zoffset;

			//判断体素的8个点与刀具的空间关系
			for (int i = 0; i <= 7; ++i) {
				OctreeVertex = GetOctreeVertex(i, curr);
				if (IsIn(OctreeVertex, CutterPos)) {
					if (i == 7) {
						//体素的8个顶点全部都被判断在刀具内，说明该体素节点可剔除
						curr->exist = false;//节点剔除
						if (curr->RenderedIndex >=0) {
							
							testVertex[(curr->RenderedIndex) + 1] = Point3f(0, 0, 0);
							testVertex[(curr->RenderedIndex) + 3] = Point3f(0, 0, 0);
							curr->RenderedIndex = -10;
						}
						//FindBrickChild(curr, Itera);
						//DeleteBrick();
						DeleteNode(curr);
						VoxelNum -= 1;
					}
					continue;
				}
				else //只要有一个点在刀具外（无论是刀具全在体素中还是刀具与体素部分相交）就说明该体素节点需要被分割
				{
					if (curr->sub == false) {
						curr->SubDivision();//如果该体素还没有分割过，那么将其分解为8个子节点
						VoxelNum += 7;//体素节点增加8个
					}
					if (curr->RenderedIndex >=0) {
						testVertex[(curr->RenderedIndex) + 1] = Point3f(0, 0, 0);
						testVertex[(curr->RenderedIndex) + 3] = Point3f(0, 0, 0);
						curr->RenderedIndex = -10;//-10代表已加入但又被删除的体素
					}
					byte ChildCode = 1;
					byte Mask = 1; 

					for (auto child : curr->children) {
						if (!child) continue;
						OctreeLine.push(child);//将体素节点压入栈

						if (IsSurface) {
							//初始化
							ChildCode = 1;
							Mask = 1;
							//获取Z维度的正负信息
							ChildCode = ChildCode << (i & 1);

							//获取Y维度的正负信息
							Mask = Mask << (2 + (i & 2) / 2);
							ChildCode = ChildCode | Mask;
							Mask = 1;

							//获取X维度的正负信息
							Mask = Mask << (4 + (i & 4) / 4);
							ChildCode = ChildCode | Mask;

							ChildCode = CurrCode & ChildCode;

							if (!ChildCode) IsSurface = false;
						}
						AddSurfacePoint_Cutter_Dynamic(CutterPos, Itera, CutterBox,ChildCode, IsSurface);
					}
					//Cutter_Dynamic(CutterPos, Itera, CutterBox);
					break;
				}

			}
		}
		else
		{
			if (IsSurface) {
				if (curr->sub || curr->RenderedIndex ==(-10)) return;//当该体素已经被分割时，不再将它加入可渲染队列
				if (curr->RenderedIndex == (-1)) {//当还未被分割的体素还未加可渲染队列时，将其加入可渲染队列
					curr->RenderedIndex = testVertex.size();
					OctreeVertex = GetOctreeVertex(0, curr);
					testVertex.push_back(OctreeVertex); testVertex.push_back(curr->halfDimension);
					OctreeVertex = GetOctreeVertex(7, curr);
					testVertex.push_back(OctreeVertex); testVertex.push_back(curr->halfDimension * (-1));
				}
			}
			return;
		}

		return;

	};

	void CudaCuter_Dynamic(CudaOctree& CudaBuffer, Point3f& CutterPos, Point3f& CutterSize, BoundBox& CutterBox) {

		//并行切削
		PrepareCut(CudaBuffer, CutterPos, CutterSize, CutterBox);

		Point3f NodeTemp = CudaBuffer.HostCutter->SubOrigin;//工件原始坐标，也就是0号节点的坐标
		Point3f BrickOrigin;
		Point3f OctreeVertex;
		Point3f BrickHalfDimension = Point3f(CudaBuffer.HostCutter->halfDimension.x / 16,
			CudaBuffer.HostCutter->halfDimension.y / 16, CudaBuffer.HostCutter->halfDimension.z / 16);

		int BrickIndex;
		int Forward;
		long long int mask = 1;//使用或|,当所在bit位置为1时则代表已经被切削
		//int mask = 1;

		for (int i = 0; i < BrickLength; i++) {

			if (CudaBuffer.HostBuffer[i].exist) continue;//true代表已被切削了

			NodeTemp = CudaBuffer.HostCutter->SubOrigin;

			NodeTemp.x = NodeTemp.x + (i & 15) * CudaBuffer.HostCutter->halfDimension.x * 2;
			NodeTemp.y = NodeTemp.y + (i & 240) / 16 * CudaBuffer.HostCutter->halfDimension.y * 2;
			NodeTemp.z = NodeTemp.z + (i & 3840) / 256 * CudaBuffer.HostCutter->halfDimension.z * 2;

			if (CudaBuffer.HostBuffer[i].Sub) {

				BrickOrigin = NodeTemp;//当前Node的中心
				//当前node的0号brick
				BrickOrigin.x = BrickOrigin.x - CudaBuffer.HostCutter->halfDimension.x * 15 / 16;
				BrickOrigin.y = BrickOrigin.y - CudaBuffer.HostCutter->halfDimension.y * 15 / 16;
				BrickOrigin.z = BrickOrigin.z - CudaBuffer.HostCutter->halfDimension.z * 15 / 16;
				Point3f TempBrickOrigin = BrickOrigin;
				for (int j = 0; j < BrickLength; j++) {

					mask = 1;
					BrickIndex = j / 64;
					Forward = j % 64;
					mask = mask << Forward;

					//if (!(CudaBuffer.HostBrick[i].brick[BrickIndex] & mask)) 
					if (!(CudaBuffer.HostBrick[i].brick[j] ))
					{//该bit位为1则代表已被切削
					    BrickOrigin = TempBrickOrigin;

					    //当前brick的坐标
					    BrickOrigin.x = BrickOrigin.x + (j & 15) * BrickHalfDimension.x * 2;
						BrickOrigin.y = BrickOrigin.y + (j & 240) / 16 * BrickHalfDimension.y * 2;
						BrickOrigin.z = BrickOrigin.z + (j & 3840) / 256 * BrickHalfDimension.z * 2;
					    
					    OctreeVertex = GetOctreeVertexForBrick(0, BrickOrigin, BrickHalfDimension);
					    testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension);
					    
					    OctreeVertex = GetOctreeVertexForBrick(7, BrickOrigin, BrickHalfDimension);
					    testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension * (-1));
				    }
				}
				//cout << "切削brick：" << CountBrick << endl;
			}
			else {
				//cout << "SubTemp:(" << (i & 15) << "," << (i & 240) / 16 << "," << (i & 3840) / 256 << ")" << endl;
				OctreeVertex = GetOctreeVertexForBrick(0, NodeTemp, CudaBuffer.HostCutter->halfDimension);
				testVertex.push_back(OctreeVertex); testVertex.push_back(CudaBuffer.HostCutter->halfDimension);

				OctreeVertex = GetOctreeVertexForBrick(7, NodeTemp, CudaBuffer.HostCutter->halfDimension);
				testVertex.push_back(OctreeVertex); testVertex.push_back(CudaBuffer.HostCutter->halfDimension * (-1));
			}
		}
	}
	
	void CudaAddPoints(CudaOctree& CudaBuffer) {
		testVertex.clear();
		Point3f NodeTemp = CudaBuffer.HostCutter->SubOrigin;//工件原始坐标，也就是0号节点的坐标
		Point3f BrickOrigin;
		Point3f OctreeVertex;
		Point3f BrickHalfDimension = Point3f(CudaBuffer.HostCutter->halfDimension.x / 16,
			CudaBuffer.HostCutter->halfDimension.y / 16, CudaBuffer.HostCutter->halfDimension.z / 16);

		int BrickIndex;
		int Forward;
		
		//int mask = 1;

		for (int i = 0; i < BrickLength; i++) {

			if (CudaBuffer.HostBuffer[i].exist) continue;//true代表已被切削了

			NodeTemp = CudaBuffer.HostCutter->SubOrigin;

			NodeTemp.x = NodeTemp.x + (i & 15) * CudaBuffer.HostCutter->halfDimension.x * 2;
			NodeTemp.y = NodeTemp.y + (i & 240) / 16 * CudaBuffer.HostCutter->halfDimension.y * 2;
			NodeTemp.z = NodeTemp.z + (i & 3840) / 256 * CudaBuffer.HostCutter->halfDimension.z * 2;

			if (CudaBuffer.HostBuffer[i].Sub) {

				BrickOrigin = NodeTemp;//当前Node的中心
				//当前node的0号brick
				BrickOrigin.x = BrickOrigin.x - CudaBuffer.HostCutter->halfDimension.x * 15 / 16;
				BrickOrigin.y = BrickOrigin.y - CudaBuffer.HostCutter->halfDimension.y * 15 / 16;
				BrickOrigin.z = BrickOrigin.z - CudaBuffer.HostCutter->halfDimension.z * 15 / 16;
				Point3f TempBrickOrigin = BrickOrigin;
				for (int j = 0; j < BrickLength; j++) {
					BrickIndex = j / 8;
					Forward = j % 8;
					BYTE mask = 1;//使用或|,当所在bit位置为1时则代表已经被切削

					mask = mask << Forward;

					if (!(CudaBuffer.HostBrick[i].brick[BrickIndex] & mask)){//该bit位为1则代表已被切削
						BrickOrigin = TempBrickOrigin;

						//当前brick的坐标
						BrickOrigin.x = BrickOrigin.x + (j & 15) * BrickHalfDimension.x * 2;
						BrickOrigin.y = BrickOrigin.y + (j & 240) / 16 * BrickHalfDimension.y * 2;
						BrickOrigin.z = BrickOrigin.z + (j & 3840) / 256 * BrickHalfDimension.z * 2;

						OctreeVertex = GetOctreeVertexForBrick(0, BrickOrigin, BrickHalfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension);
						OctreeVertex = GetOctreeVertexForBrick(7, BrickOrigin, BrickHalfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension * (-1));
					}
				}
				//cout << "切削brick：" << CountBrick << endl;
			}
			else {
				//cout << "SubTemp:(" << (i & 15) << "," << (i & 240) / 16 << "," << (i & 3840) / 256 << ")" << endl;
				OctreeVertex = GetOctreeVertexForBrick(0, NodeTemp, CudaBuffer.HostCutter->halfDimension);
				testVertex.push_back(OctreeVertex); testVertex.push_back(CudaBuffer.HostCutter->halfDimension);

				OctreeVertex = GetOctreeVertexForBrick(7, NodeTemp, CudaBuffer.HostCutter->halfDimension);
				testVertex.push_back(OctreeVertex); testVertex.push_back(CudaBuffer.HostCutter->halfDimension * (-1));
			}
		}
	}

	void CudaAddPoints2(CudaOctree& CudaBuffer) {
		testVertex.clear();
		Point3f NodeTemp = CudaBuffer.HostCutter->SubOrigin;//工件原始坐标，也就是0号节点的坐标
		Point3f BrickOrigin;
		Point3f OctreeVertex;
		Point3f BrickHalfDimension = Point3f(CudaBuffer.HostCutter->halfDimension.x / 32,
			CudaBuffer.HostCutter->halfDimension.y / 32, CudaBuffer.HostCutter->halfDimension.z / 32);

		int BrickIndex;
		int Forward;

		//int mask = 1;

		for (int i = 0; i < BrickLength; i++) {

			if (CudaBuffer.HostBuffer[i].exist) continue;//true代表已被切削了

			NodeTemp = CudaBuffer.HostCutter->SubOrigin;

			NodeTemp.x = NodeTemp.x + (i & 15) * CudaBuffer.HostCutter->halfDimension.x * 2;
			NodeTemp.y = NodeTemp.y + (i & 240) / 16 * CudaBuffer.HostCutter->halfDimension.y * 2;
			NodeTemp.z = NodeTemp.z + (i & 3840) / 256 * CudaBuffer.HostCutter->halfDimension.z * 2;

			if (CudaBuffer.HostBuffer[i].Sub) {

				BrickOrigin = NodeTemp;//当前Node的中心
				//当前node的0号brick
				BrickOrigin.x = BrickOrigin.x - CudaBuffer.HostCutter->halfDimension.x * 31 / 32;
				BrickOrigin.y = BrickOrigin.y - CudaBuffer.HostCutter->halfDimension.y * 31 / 32;
				BrickOrigin.z = BrickOrigin.z - CudaBuffer.HostCutter->halfDimension.z * 31 / 32;
				Point3f TempBrickOrigin = BrickOrigin;
				for (int j = 0; j < BrickLength * 8; j++) {
					BrickIndex = j / 8;
					Forward = j % 8;
					BYTE mask = 1;//使用或|,当所在bit位置为1时则代表已经被切削

					mask = mask << Forward;

					if (!(CudaBuffer.HostBrick[i].brick[BrickIndex] & mask)) {//该bit位为1则代表已被切削
						BrickOrigin = TempBrickOrigin;

						//当前brick的坐标
						BrickOrigin.x = BrickOrigin.x + (j & 31) * BrickHalfDimension.x * 2;
						BrickOrigin.y = BrickOrigin.y + (j & 992) / 32 * BrickHalfDimension.y * 2;
						BrickOrigin.z = BrickOrigin.z + (j & 31744) / 1024 * BrickHalfDimension.z * 2;

						OctreeVertex = GetOctreeVertexForBrick(0, BrickOrigin, BrickHalfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension);
						OctreeVertex = GetOctreeVertexForBrick(7, BrickOrigin, BrickHalfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension * (-1));
					}
				}
				//cout << "切削brick：" << CountBrick << endl;
			}
			else {
				//cout << "SubTemp:(" << (i & 15) << "," << (i & 240) / 16 << "," << (i & 3840) / 256 << ")" << endl;
				OctreeVertex = GetOctreeVertexForBrick(0, NodeTemp, CudaBuffer.HostCutter->halfDimension);
				testVertex.push_back(OctreeVertex); testVertex.push_back(CudaBuffer.HostCutter->halfDimension);

				OctreeVertex = GetOctreeVertexForBrick(7, NodeTemp, CudaBuffer.HostCutter->halfDimension);
				testVertex.push_back(OctreeVertex); testVertex.push_back(CudaBuffer.HostCutter->halfDimension * (-1));
			}
		}
	}

	void CudaAddPointsForThreeLevels(CudaOctree& CudaBuffer) {
		testVertex.clear();
		Point3f NodeTemp = CudaBuffer.HostCutter->SubOrigin;//工件原始坐标，也就是0号节点的坐标
		Point3f BrickOrigin;
		Point3f OctreeVertex;
		Point3f BrickHalfDimension = Point3f(CudaBuffer.HostCutter->halfDimension.x / 16,
			CudaBuffer.HostCutter->halfDimension.y / 16, CudaBuffer.HostCutter->halfDimension.z / 16);

		int BrickIndex;
		int Forward;

		//int mask = 1;
		for (int OctreeNodeNumber = 0; OctreeNodeNumber < 8; OctreeNodeNumber++){
			for (int i = 0; i < BrickLength; i++) {

				if (CudaBuffer.HostBuffer[OctreeNodeNumber * 4096 + i].exist) continue;//true代表已被切削了

				NodeTemp = CudaBuffer.HostOctree[OctreeNodeNumber].Origin;

				NodeTemp.x = NodeTemp.x + (i & 15) * CudaBuffer.HostCutter->halfDimension.x * 2;//HostCutter中保存的是Node节点的尺寸信息
				NodeTemp.y = NodeTemp.y + (i & 240) / 16 * CudaBuffer.HostCutter->halfDimension.y * 2;
				NodeTemp.z = NodeTemp.z + (i & 3840) / 256 * CudaBuffer.HostCutter->halfDimension.z * 2;

				if (CudaBuffer.HostBuffer[i].Sub) {

					BrickOrigin = NodeTemp;//当前Node的中心
					//当前node的0号brick
					BrickOrigin.x = BrickOrigin.x - CudaBuffer.HostCutter->halfDimension.x * 15 / 16;
					BrickOrigin.y = BrickOrigin.y - CudaBuffer.HostCutter->halfDimension.y * 15 / 16;
					BrickOrigin.z = BrickOrigin.z - CudaBuffer.HostCutter->halfDimension.z * 15 / 16;
					Point3f TempBrickOrigin = BrickOrigin;
					for (int j = 0; j < BrickLength; j++) {
						BrickIndex = j / 8;
						Forward = j % 8;
						BYTE mask = 1;//使用或|,当所在bit位置为1时则代表已经被切削

						mask = mask << Forward;

						if (!(CudaBuffer.HostBrick[OctreeNodeNumber * 4096 + i].brick[BrickIndex] & mask)) {//该bit位为1则代表已被切削
							BrickOrigin = TempBrickOrigin;

							//当前brick的坐标
							BrickOrigin.x = BrickOrigin.x + (j & 15) * BrickHalfDimension.x * 2;
							BrickOrigin.y = BrickOrigin.y + (j & 240) / 16 * BrickHalfDimension.y * 2;
							BrickOrigin.z = BrickOrigin.z + (j & 3840) / 256 * BrickHalfDimension.z * 2;

							OctreeVertex = GetOctreeVertexForBrick(0, BrickOrigin, BrickHalfDimension);
							testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension);
							OctreeVertex = GetOctreeVertexForBrick(7, BrickOrigin, BrickHalfDimension);
							testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension * (-1));
						}
					}
					//cout << "切削brick：" << CountBrick << endl;
				}
				else {
					//cout << "SubTemp:(" << (i & 15) << "," << (i & 240) / 16 << "," << (i & 3840) / 256 << ")" << endl;
					OctreeVertex = GetOctreeVertexForBrick(0, NodeTemp, CudaBuffer.HostCutter->halfDimension);
					testVertex.push_back(OctreeVertex); testVertex.push_back(CudaBuffer.HostCutter->halfDimension);

					OctreeVertex = GetOctreeVertexForBrick(7, NodeTemp, CudaBuffer.HostCutter->halfDimension);
					testVertex.push_back(OctreeVertex); testVertex.push_back(CudaBuffer.HostCutter->halfDimension * (-1));
				}
			}
     	}




	}

	void CopyMamery(CudaOctree& CudaBuffer) {
		cudaDeviceSynchronize();
		//将Node数组的信息复制返回给Host端
		cudaMemcpy(CudaBuffer.HostBuffer, CudaBuffer.DeviceBuffer, BrickLength * sizeof(Node), cudaMemcpyDeviceToHost);
		//将brick数组的信息复制返回给host端
		cudaMemcpy(CudaBuffer.HostBrick, CudaBuffer.DeviceBrick, BrickLength * sizeof(BitBricks), cudaMemcpyDeviceToHost);
	}

};

