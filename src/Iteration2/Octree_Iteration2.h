#pragma once
#include"../src/Octree/Octree.h"
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


class OctreeCut_Iteration2 {
private:

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

	vector<RenderData> testVertex;

	stack<Octree*>  OctreeLine;//存储未被判断的体素节点
	stack<Octree*> RenderedCube;
	stack<Octree*> BrickChildToBeDelete;

	byte ChildPosCode[8];
	float Precesion[14];

	OctreeCut_Iteration2(Octree& copy, Point3f Center, Point3f CS) :C(Center), CutterSize(CS) {
		Head = new Octree(copy);

		byte ChildCode;
		byte Mask;

		for (int i = 0; i < 8; i++) {
			//初始化
			ChildCode = 1;
			Mask = 1;
			//获取子节点在父节点的Z维度的部位信息
			ChildCode = ChildCode << ((i & 1) + 4);

			//获取子节点在父节点的Y维度的部位信息
			Mask = Mask << ((i & 2) / 2 + 2);
			ChildCode = ChildCode | Mask;

			//获取子节点在父节点的X维度的部位信息
			Mask = 1;
			Mask = Mask << ((i & 4) / 4);
			ChildCode = ChildCode | Mask;

			ChildPosCode[i] = ChildCode;
		}

		for (int i = 0; i < 12; i++) {
			Precesion[i] = pow(2, i);
			//cout << Precesion[i] << endl;
		}
		
	}


	//输入整数和节点指针，获取节点体素的某个顶点的位置
	Point3f GetOctreeVertex(int number, Octree* curr) {
		Point3f OctreeVertex;

		OctreeVertex.x = curr->origin.x - curr->halfDimension.x;
		OctreeVertex.x = OctreeVertex.x + (number & 4) / 2 * curr->halfDimension.x;

		OctreeVertex.y = curr->origin.y - curr->halfDimension.y;
		OctreeVertex.y = OctreeVertex.y + (number & 2) * curr->halfDimension.y;

		OctreeVertex.z = curr->origin.z - curr->halfDimension.z;
		OctreeVertex.z = OctreeVertex.z + (number & 1) * 2 * curr->halfDimension.z;

		return OctreeVertex;
	}

	Point3f GetOctreeVertexForBrick(int number, Point3f& origin, Point3f& halfDimension) {
		Point3f OctreeVertex;

		OctreeVertex.x = origin.x - halfDimension.x;
		OctreeVertex.x = OctreeVertex.x + (number & 4) / 2 * halfDimension.x;

		OctreeVertex.y = origin.y - halfDimension.y;
		OctreeVertex.y = OctreeVertex.y + (number & 2) * halfDimension.y;

		OctreeVertex.z = origin.z - halfDimension.z;
		OctreeVertex.z = OctreeVertex.z + (number & 1) * 2 * halfDimension.z;

		return OctreeVertex;
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

	void ReadPathFile(const char* cfilename) {
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
			temp = Point3f(-0.6f * (float)t / 200.0f, (0.8f - 1.6f * (float)t / 200.0f), z);
		}
		else if (t <= 400) {
			temp = Point3f((-0.6f + 1.4f * (float)(t - 200) / 200.0f), (-0.8f + 1.0f * (float)(t - 200) / 200.0f), z);
		}
		else  if (t <= 600) {
			temp = Point3f((0.8f - 1.6f * (float)(t - 400) / 200.0f), 0.2f, z);
		}
		else if (t <= 800) {
			temp = Point3f((-0.8f + 1.4f * (float)(t - 600) / 200.0f), (0.2f - 1.0f * (float)(t - 600) / 200.0f), z);
		}
		else if (t <= 1000) {
			temp = Point3f((0.6f - 0.6f * (float)(t - 800) / 200.0f), (-0.8f + 1.6f * (float)(t - 800) / 200.0f), z);
		}
		else {
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
		float Z = (OctreeVer.z - CutPosition.z);
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

		Cutter_Dynamic_Surface(CutterPos, Itera, CutterBox, InitCode, IsSurface);

		AddSurfacePointsForBrick512(Head, Itera);
		//AddSurfacePoint_Cutter_Dynamic(CutterPos, Itera, CutterBox,InitCode,IsSurface);
		//AddPoint_Cutter_Dynamic(CutterPos, Itera, CutterBox, MyDevice);
		//IteraAddCube(Head);
		//IteraAddtestCube(Head);
		//AddPoints(Head);
		//AddPointsForBrick512(Head, Itera);
		//AddPointsForBrick4096(Head, Itera);
		//AddPointsForBrick(Head, Itera);
	}

	//可能需要的：将将可渲染体素加入可渲染队列，针对的bricks容量为512
	void AddPointsForBrick512(Octree* curr, int& Itera) {
		testVertex.clear();
		IteraAddPointsForBrick512(curr, Itera);
	}

	void IteraAddPointsForBrick512(Octree* curr, int& Itera) {
		if (!curr->sub) {
			RenderData RD;
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
						//RenderData RD = RenderData(OctreeVertex, curr->accuracy + 1 + 3);
						testVertex.push_back(RD);
						OctreeVertex = GetOctreeVertexForBrick(7, SubTemp, halfDimension);
						//RD.Postion = OctreeVertex;RD.Itera *= (-1);
						testVertex.push_back(RD); //testVertex.push_back(halfDimension * -1);
						//cout << "添加成功";
					}
				}

			}
			else {
				Point3f temp;
				temp = GetOctreeVertex(0, curr);
				//RenderData RD = RenderData(temp, curr->accuracy + 1);
				testVertex.push_back(RD);
				temp = GetOctreeVertex(7, curr);
				//RD.Postion = temp; RD.Itera *= (-1);
				testVertex.push_back(RD); 
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
			SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
			SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.x / 4;
			SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.x / 4;

			Point3f OctreeVertex;
			if (IsIn(SubTemp, CutterPos)) {
				bri->brick[k] = true;
			}
		}
	}

	//如果不是surface那直接按照是否相交添加进渲染列表
	void Deeper_cut512_ForSurface(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos, byte CurrCode, bool IsSurface) {

		Bricks* bri = (Bricks*)curr->bricks;
		//Octree* temp = (Octree*)bri->brick;

		float precision = 0.125;

		Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 * precision,
			curr->origin.y - curr->halfDimension.y * 7 * precision, curr->origin.z - curr->halfDimension.z * 7 * precision);
		Point3f SubTemp;
		Point3f halfDimension = Point3f(curr->halfDimension.x * precision, curr->halfDimension.y * precision, curr->halfDimension.z * precision);
		//cout << "精度为:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";


		if (!IsSurface) {//如果不是表面体素，那么直接根据它是否与刀具包围盒相交来决定是否加入渲染列表
			//cout << "非表面" << endl;
			for (int k = 0; k < BrickLength; ++k) {
				if (bri->brick[k]) continue;
				SubTemp = SubOrigin;
				SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
				SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.y / 4;
				SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.z / 4;
				if (IsIn(SubTemp, CutterPos)) {
					bri->brick[k] = true;
					bri->Rendered[k] = false;
					bri->sum--;
					continue;
				}

				Point3f OctreeVertex;
				//判断是否与刀具相交，从而判断是否应该被渲染
				for (int i = 0; i <= 7; ++i) {
					OctreeVertex = GetOctreeVertexForBrick(i, SubTemp, halfDimension);
					if (IsIn(OctreeVertex, CutterPos)) {
						bri->Rendered[k] = true;
						break;
					}
				}
				//if (!bri->Rendered[k]) cout << "不被渲染" << endl;
			}

			if (!bri->sum) DeleteNode(curr);
		}
		else {
			//cout << "表面" << endl;
			byte Mask = 1;
			int Index;
			int t;//计算标志是+还是-;
			int n;//计算标志是x或y或z;
			int mask1 = 1;
			int mask2 = 1;
			for (Index = 0; Index < 6; Index++) {
				Mask = 1;
				Mask = Mask << Index;
				Mask = CurrCode & Mask;
				if (Mask) break;
			}
			//推算出是哪个面后CurrCode已经无用了，后续可用于中间变量
			for (int k = 0; k < BrickLength; ++k) {

				if (bri->brick[k]) continue;

				SubTemp = SubOrigin;
				SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
				SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.y / 4;
				SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.z / 4;

				//判断是否在刀具内部，也就是说是否被切削
				if (IsIn(SubTemp, CutterPos)) {
					bri->brick[k] = true;//清除该brick体素
					bri->Rendered[k] = false;//不被加入渲染列表
					bri->sum--;
					continue;
				}


				Point3f OctreeVertex;
				//判断该体素是否与刀具相交，刀具包围盒不准确，因此直接使用刀具
				for (int i = 0; i <= 7; ++i) {
					OctreeVertex = GetOctreeVertexForBrick(i, SubTemp, halfDimension);
					if (IsIn(OctreeVertex, CutterPos)) {
						bri->Rendered[k] = true;
						break;
					}
				}

				if (bri->Rendered[k] = true) { continue; }//如果已经被加入渲染列表那直接跳过，否则需要再次判断它是否为表面体素
				else {
					t = Index % 2;
					n = (Index / 2) * 3;

					mask1 = 7;
					mask2 = 7;
					mask1 << n;
					mask2 = k & mask1;

					if (t) { if (mask2 == mask1) bri->Rendered[k] = true; }
					else { if (mask2 == 0) bri->Rendered[k] = true; }
				}
			}
			if (!bri->sum) DeleteNode(curr);
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
		RenderData RD;
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
				//RenderData RD = RenderData(OctreeVertex, curr->accuracy + 1 + 3);
				testVertex.push_back(RD);
				OctreeVertex = GetOctreeVertexForBrick(7, SubTemp, halfDimension);
				//RD.Postion = OctreeVertex; RD.Itera *= (-1);
				testVertex.push_back(RD); //testVertex.push_back(halfDimension * -1);

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
					testVertex[BrickIndex + 1].Postion = Point3f(0, 0, 0);
					testVertex[BrickIndex + 3].Postion = Point3f(0, 0, 0);
				}
				//else{bri->brick[k] = true;}
			}

			if (bri->sum == 0) DeleteNode(curr);

		}
	}

	//添加表面体素
	void AddSurfacePointsForBrick512(Octree* curr, int& Itera) {
		testVertex.clear();
		IteraAddSurfacePointsForBrick512(curr, Itera);
	}

	//添加表面体素
	void IteraAddSurfacePointsForBrick512(Octree* curr, int& Itera) {
		if (!curr->sub) {
			RenderData RD;
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

					if (Temp->Rendered[i])//if (!Temp->brick[i]) ,&& Temp->Rendered[i],!Temp->brick[i] &&
					{
						SubTemp = SubOrigin;

						SubTemp.x = SubTemp.x + (i & 7) * curr->halfDimension.x / 4;
						SubTemp.y = SubTemp.y + (i & 56) / 8 * curr->halfDimension.y / 4;
						SubTemp.z = SubTemp.z + (i & 448) / 64 * curr->halfDimension.z / 4;

						/*
						OctreeVertex = GetOctreeVertexForBrick(0, SubTemp, halfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension);
						//RenderItera.push_back((curr->accuracy + 1  + 3));
						OctreeVertex = GetOctreeVertexForBrick(7, SubTemp, halfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(halfDimension * -1);
						//RenderItera.push_back((curr->accuracy + 1 + 3));
						*/
				    
						OctreeVertex = GetOctreeVertexForBrick(0, SubTemp, halfDimension);
						RD = RenderData(OctreeVertex, halfDimension);
						testVertex.push_back(RD);
						OctreeVertex = GetOctreeVertexForBrick(7, SubTemp, halfDimension);
						RD.Postion = OctreeVertex; RD.Itera = RD.Itera * (-1);
						testVertex.push_back(RD); //testVertex.push_back(halfDimension * -1); 
					
						//cout << "添加成功";
					}
				}

			}
			else {
				if (curr->Rendered) {
					Point3f temp;
					temp = GetOctreeVertex(0, curr);
					RD = RenderData(temp, curr->halfDimension);
					testVertex.push_back(RD);

					temp = GetOctreeVertex(7, curr);
					RD.Postion = temp; RD.Itera = RD.Itera * (-1);
					testVertex.push_back(RD);
					/*
					temp = GetOctreeVertex(0, curr);
					testVertex.push_back(temp); testVertex.push_back(curr->halfDimension);
					RenderItera.push_back((curr->accuracy + 1));
					temp = GetOctreeVertex(7, curr);
					testVertex.push_back(temp); testVertex.push_back(curr->halfDimension * (-1));
					RenderItera.push_back((curr->accuracy + 1));
					*/
				}
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

	//删除已经被剔除的体素节点，并向上追溯其父亲节点

	void FindParent(Octree* Parent, int index) {
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
				testVertex[BrickIndex + 1].Postion = Point3f(0, 0, 0);
				testVertex[BrickIndex + 3].Postion = Point3f(0, 0, 0);
			}
			delete temp;
			delete BrickChildToBeDelete.top();
			cout << "删除brick和叶子" << endl;
			BrickChildToBeDelete.pop();
		}
	}

	//针对表面体素的动态切削方法
	void Cutter_Dynamic_Surface(Point3f& CutterPos, int& Itera, BoundBox& CutterBox, byte CurrCode, bool IsSurface) {
		if (OctreeLine.empty()) return;

		Octree* curr = OctreeLine.top();
		OctreeLine.pop();

		//if (curr == nullptr || curr->exist == false) { return; };

		bool is_intersect = IsIntersect(curr, CutterBox);

		//if (Itera > Iteration || curr->accuracy > Iteration )
		//当八叉树节点达到分割极限后
		if (curr->accuracy > Itera && is_intersect)//&& is_intersect
		{
			//如果当前八叉树叶子节点还没有被分配bricks结构，那么为它申请一个
			if (curr->bricks == nullptr) {
				curr->bricks = new Bricks(curr);
			}
			//Init_Data4096(curr, CutterBox, CutterPos, CutterSize, MyDevice);
			//Init_Data512(curr, CutterBox, CutterPos, CutterSize);
			//w为当前八叉树节点执行更深入的分割，bricks容量为512
			//Deeper_cut512(curr, CutterBox, CutterPos);
			Deeper_cut512_ForSurface(curr, CutterBox, CutterPos, CurrCode, IsSurface);
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

					byte ChildCode = 0;
					//byte Mask = 1;
					for (auto child : curr->children) {
						if (!child) continue;
						OctreeLine.push(child);//将体素节点压入栈
						ChildCode = 0;
						if (IsSurface) {
							/*
							//初始化
							ChildCode = 1;
							Mask = 1;
							//获取Z维度的正负信息
							ChildCode = ChildCode << ((child->index & 1) + 4);

							//获取Y维度的正负信息
							Mask = Mask << ((child->index & 2) / 2 + 2);
							ChildCode = ChildCode | Mask;

							//获取X维度的正负信息
							Mask = 1;
							Mask = Mask << ((child->index & 4) / 4);
							ChildCode = ChildCode | Mask;

							ChildCode = CurrCode & ChildCode;

							if (ChildCode == 0) { child->Rendered = false; }
							else { child->Rendered = true; }

							*/
							ChildCode = CurrCode & ChildPosCode[child->index];
							if (ChildCode == 0) { child->Rendered = false; }
							else { child->Rendered = true; }

						}
						Cutter_Dynamic_Surface(CutterPos, Itera, CutterBox, ChildCode, child->Rendered);
					}
					//Cutter_Dynamic(CutterPos, Itera, CutterBox);
					break;
				}

			}
		}
		else {
			if (IsSurface && !curr->sub) {
				curr->Rendered = true;
			}
			else {
				curr->Rendered = false;
			}
			return;
		}

		return;

	};

};