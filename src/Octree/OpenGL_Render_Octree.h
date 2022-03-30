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
	bool* D_buffer;//Device������
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

	std::unordered_map<Point3f, int, HashFunctionOfPoint3f>  positionToVertex;//λ�ö�Ӧ��������
	std::unordered_map<Point3i, int, HashFunctionOfPoint3i>  vertexToFace;//��������������Ӧһ��������Ƭ
	int TempVertex[8] = {};//�洢һ���ڵ��8�����������,���ڷָ�����
	Point3f TestTempVertex[8] = {};//�洢һ���ڵ��8�������λ��,���ڷָ�����
	ReadPath Path;

public:

	Octree* Head;//�����ĳ�ʼֵ
	Point3f C;
	Point3f CutterSize;//H��R��r;Ŀǰ��ΪR==r
	float Accuracy;
	//int Iteration;
	int CubeID = 0;
	int VoxelNum = 1;
	int BoolNum = 0;
	vector<Point3f> vertices;//���λ�ü���
	vector<Point3i> faces;//����������������������һ����

	vector<Point3f> testVertex;

	stack<Octree*>  OctreeLine;//�洢δ���жϵ����ؽڵ�
	stack<Octree*> RenderedCube;
	stack<Octree*> BrickChildToBeDelete;

	OctreeCut_2(Octree& copy, Point3f Center, Point3f CS) :C(Center), CutterSize(CS) {
		Head = new Octree(copy);
		
	}

	//��ȡ�����������е�����
	int getVertexIndex(const Point3f& position)
	{
		auto it = positionToVertex.find(position);
        
        if (it != positionToVertex.end())
        {
        	return it->second;
        }
        

		int ret = static_cast<int>((vertices.size()));
		//��ȡ��ǰvertices�Ĵ�С����Ϊ�¶�����±�,ÿ����float����һ������λ��

		Point3f NewPosition = position;//����һ��VertexRecord���󲢳�ʼ��λ��Ϊposition
		//����λ�õ��������������������ö�������
		vertices.push_back(NewPosition);

		positionToVertex[position] = ret;

		return ret;
	}
	
	//��vec3�е���������
	Point3i SortVec3(Point3i vec3) {//���������������������������
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

	//��ȡ��Ƭ����Ƭ�����е���������Ƭ������ʱ��faces�����Ƭ
	int getFaceIndex(Point3i SortedvertexPair)
	{
		//auto sortedVertexPair = vertexPair.x < vertexPair.y ? vertexPair : vertexPair.yx();
		auto it = vertexToFace.find(SortedvertexPair);
		if (it != vertexToFace.end())
		{
			return it->second;
		}

		int ret = static_cast<int>((faces.size()));

	   //vertexPair��Ҫʱxyz�Ӵ�С

		faces.push_back(SortedvertexPair);
		vertexToFace[SortedvertexPair] = ret;

		return ret;
	}

	//��faces�����������Ƭ����getFaceIndex()�����������ظ������Կ���ɾ���ú���
	void addTriangle(Point3i SortedvertexPair) {

		auto it = vertexToFace.find(SortedvertexPair);
		if (it != vertexToFace.end())
		{
			return;
		};
		//vertexPair��Ҫʱxyz��С����
		faces.push_back(SortedvertexPair);
		vertexToFace[SortedvertexPair] = (static_cast<int>((faces.size()))) - 1;
		return;

	}

	//���������ͽڵ�ָ�룬��ȡ�ڵ����ص�ĳ�������λ��
	Point3f GetOctreeVertex(int number, Octree* curr) {
		Point3f OctreeVertex;
		/*if (((number & 1) == 1)) { OctreeVertex.z = curr->origin.z + curr->halfDimension.z; }//Z���������Ϸ���
		else { OctreeVertex.z = curr->origin.z - curr->halfDimension.z; }

		if (((number & 2) == 2)) { OctreeVertex.y = curr->origin.y + curr->halfDimension.y; }//Y��������ǰ����
		else { OctreeVertex.y = curr->origin.y - curr->halfDimension.y; }

		if (((number & 4) == 4)) { OctreeVertex.x = curr->origin.x + curr->halfDimension.x; }//Z���������ҷ���
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
	//�����Ƭ��������faces�����ڰ���������
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

		faces.push_back(Point3i(TempVertex[0], TempVertex[1], TempVertex[3]));//��������Ƭ�������棬���鲻Ҫ�����ڵ��޳�����Ȳ��ԣ���
		faces.push_back(Point3i(TempVertex[3], TempVertex[2], TempVertex[0]));

		faces.push_back(Point3i(TempVertex[0], TempVertex[4], TempVertex[6]));//֮��Ҫ������ӵķ�����������
		faces.push_back(Point3i(TempVertex[6], TempVertex[2], TempVertex[0]));
		
		faces.push_back(Point3i(TempVertex[1], TempVertex[5], TempVertex[7]));//
		faces.push_back(Point3i(TempVertex[7], TempVertex[3], TempVertex[1]));
		
		faces.push_back(Point3i(TempVertex[0], TempVertex[4], TempVertex[5]));//
		faces.push_back(Point3i(TempVertex[5], TempVertex[1], TempVertex[0]));
		
		faces.push_back(Point3i(TempVertex[2], TempVertex[6], TempVertex[7]));//
		faces.push_back(Point3i(TempVertex[7], TempVertex[3], TempVertex[2]));
		
		faces.push_back(Point3i(TempVertex[4], TempVertex[5], TempVertex[7]));//
		faces.push_back(Point3i(TempVertex[7], TempVertex[6], TempVertex[4]));
		

		/*faces.push_back(SortVec3(Point3i(TempVertex[0], TempVertex[1], TempVertex[3])));//��������Ƭ�������棬���鲻Ҫ�����ڵ��޳�����Ȳ��ԣ���
		faces.push_back(SortVec3(Point3i(TempVertex[0], TempVertex[2], TempVertex[3])));
		faces.push_back(SortVec3(Point3i(TempVertex[0], TempVertex[2], TempVertex[4])));//֮��Ҫ������ӵķ�����������
		faces.push_back(SortVec3(Point3i(TempVertex[2], TempVertex[4], TempVertex[6])));
		faces.push_back(SortVec3(Point3i(TempVertex[0], TempVertex[4], TempVertex[5])));//
		faces.push_back(SortVec3(Point3i(TempVertex[2], TempVertex[1], TempVertex[5])));
		faces.push_back(SortVec3(Point3i(TempVertex[1], TempVertex[5], TempVertex[7])));//
		faces.push_back(SortVec3(Point3i(TempVertex[1], TempVertex[3], TempVertex[7])));
		faces.push_back(SortVec3(Point3i(TempVertex[2], TempVertex[3], TempVertex[6])));//
		faces.push_back(SortVec3(Point3i(TempVertex[3], TempVertex[6], TempVertex[7])));
		faces.push_back(SortVec3(Point3i(TempVertex[5], TempVertex[6], TempVertex[7])));//
		faces.push_back(SortVec3(Point3i(TempVertex[4], TempVertex[5], TempVertex[6])));*/


		//faces.push_back(TempVertex[0]); faces.push_back(TempVertex[3]); faces.push_back(TempVertex[1]);//��
		//faces.push_back(TempVertex[0]); faces.push_back(TempVertex[2]); faces.push_back(TempVertex[3]);

		//faces.push_back(TempVertex[0]); faces.push_back(TempVertex[4]); faces.push_back(TempVertex[2]);//��
		//faces.push_back(TempVertex[2]); faces.push_back(TempVertex[4]); faces.push_back(TempVertex[6]);

		//faces.push_back(TempVertex[0]); faces.push_back(TempVertex[5]); faces.push_back(TempVertex[4]);//��
		//faces.push_back(TempVertex[0]); faces.push_back(TempVertex[1]); faces.push_back(TempVertex[5]);

		//faces.push_back(TempVertex[1]); faces.push_back(TempVertex[7]); faces.push_back(TempVertex[5]);//��
		//faces.push_back(TempVertex[1]); faces.push_back(TempVertex[3]); faces.push_back(TempVertex[7]);

		//faces.push_back(TempVertex[2]); faces.push_back(TempVertex[6]); faces.push_back(TempVertex[3]);//ǰ
		//faces.push_back(TempVertex[3]); faces.push_back(TempVertex[6]); faces.push_back(TempVertex[7]);

		//faces.push_back(TempVertex[5]); faces.push_back(TempVertex[7]); faces.push_back(TempVertex[6]);//��
		//faces.push_back(TempVertex[4]); faces.push_back(TempVertex[5]); faces.push_back(TempVertex[6]);

		//curr->rendered = true;
		return;

	}

	//�����Ƭ�������������testvertices���飬���ڰ�������ƣ�Ŀǰʹ�õĻ��Ʒ���
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

	//��ȡ���ߵ�AABB��Χ�У������ж��Ƿ��ĳ�����ؽڵ��ཻ
	BoundBox GetCutterBoundBox(Point3f& Center)//��ʱ�����ǵ��ߵ���ת
	{
		//��ʱ�����ǵ��ߵ���ת��
		BoundBox box;
		box.Origin = Center;
		box.Origin.z = Center.z + (CutterSize.x - CutterSize.y) / 2;

		box.length = 2 * CutterSize.y;
		box.width = 2 * CutterSize.y;
		box.height = CutterSize.y + CutterSize.x;

		return box;
	}

	//�ж����ؽڵ��Ƿ��뵶���ཻ�����ؽڵ���ȫλ�ڰ�Χ����Ҳ��Ϊ���ཻ
	bool IsIntersect(Octree* Voxel, BoundBox& Cutter) {
		if (std::abs(Voxel->origin.x - Cutter.Origin.x) < std::abs(Voxel->halfDimension.x + (Cutter.length / 2))
			&& std::abs(Voxel->origin.y - Cutter.Origin.y) < std::abs(Voxel->halfDimension.y + (Cutter.width / 2))
			&& std::abs(Voxel->origin.z - Cutter.Origin.z) < std::abs(Voxel->halfDimension.z + (Cutter.height / 2))
			)
		{
			//cout << "�ཻ" << endl;
			return true;
		}
		else
		{
			//cout << "���ཻ" << endl;
			return false;
		}

	}

	bool IsIntersectForBrick(Point3f& origin, Point3f halfDimension, BoundBox& Cutter) {
		if (std::abs(origin.x - Cutter.Origin.x) < std::abs(halfDimension.x + (Cutter.length / 2))
			&& std::abs(origin.y - Cutter.Origin.y) < std::abs(halfDimension.y + (Cutter.width / 2))
			&& std::abs(origin.z - Cutter.Origin.z) < std::abs(halfDimension.z + (Cutter.height / 2))
			)
		{
			//cout << "�ཻ" << endl;
			return true;
		}
		else
		{
			//cout << "���ཻ" << endl;
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
	//��ȡ������ĳ��ʱ��΢Ԫ��λ�ã����涨���˵��ߵĹ켣
	Point3f GetCutterPos(float T, float t) {

		float part = t / T;

		return Point3f(C.x + part * Head->halfDimension.x * 2 * 3 / 2, C.y + part * Head->halfDimension.y * 2 * 3 / 2, C.z);//��λΪmm;

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
		//0~10s����(15,50)->(85,50)
		if (t <= 200) {
			return Point3f(t * 7, 0, 0) + C;
		}
		//10~20s,��(85,50)->(30,10)
		else if (t <= 400) {
			t -= 200;
			return Point3f(t * -5.5, t * -4, 0) + Point3f(85, 50, C.z);
		}
		//20s~30s,��(30,10)->(50,75)
		else if (t <= 600) {
			t -= 20;
			return Point3f(t * 2, t * 6.5, 0) + Point3f(30, 10, C.z);
		}
		//30s~40s,��(50,75)->(70,10)
		else if (t <= 800) {
			t -= 30;
			return Point3f(t * 2, t * -6.5, 0) + Point3f(50, 75, C.z);
		}
		//40s~50s,��(70,10)->(15,50)
		else {
			t -= 40;
			return Point3f(t * -5.5, t * 4, 0) + Point3f(70, 10, C.z);
		}
		*/
	}
	//�ж����ص�ĳ�������Ƿ��ڵ����ڲ�
	bool IsIn(Point3f& OctreeVer, Point3f CutPosition) {

		float X = abs(OctreeVer.x - CutPosition.x);
		float Y = abs(OctreeVer.y - CutPosition.y);
		float Z =    (OctreeVer.z - CutPosition.z);
		//if (X > CutterSize.y) return false;
		//if (Y > CutterSize.y) return false;
		bool IntheCircle = ((X * X + Y * Y) <= CutterSize.y * CutterSize.y);//����ͷϳ����ֱ������
		
		if (IntheCircle && (OctreeVer.z > CutPosition.z)) return true;

		bool InTheSphere = (X * X + Y * Y + Z * Z) <= (CutterSize.y * CutterSize.y);//����ͷϳ��������
		//if (Z > CutterSize.x) return false;
		//if (OctreeVer.z <= CutPosition.z) return true;
		if (InTheSphere) return true;

		return false;

	}

    //���ھ�̬��ʾ����
	void Cutter(Point3f& CutterPos, int& Itera, BoundBox CutterBox) {


		if (OctreeLine.empty()) return;

		Octree* curr = OctreeLine.top();
		OctreeLine.pop();
		if (curr == nullptr || curr->exist == false) { return; };

		//if (Itera > Iteration || curr->accuracy > Iteration ) 
		if (curr->accuracy > Itera)
		{
			addtestCube(curr);
			//addCube(curr);//�������������������������ָ����,����������ӽ���Ƭ���ϣ�Ȼ�󷵻�
			return;
		}
		//else Itera += 1;//��ǰ����������1

		if (IsIntersect(curr, CutterBox)) {
			//��ʱ���ؽڵ������ȫλ�ڵ����ڲ�����Ҫ������������뵶���ཻ�����ؽڵ�����Ҫ�ٴηָ�

			Point3f OctreeVertex;
			Point3f SubHalfDimension((curr->halfDimension.x / 2), (curr->halfDimension.y / 2), (curr->halfDimension.z / 2));

			float Xoffset;
			float Yoffset;
			float Zoffset;
			//�ж����ص�8�����뵶�ߵĿռ��ϵ
			for (int i = 0; i <= 7; ++i) {

				OctreeVertex = GetOctreeVertex(i, curr);

				if (IsIn(OctreeVertex, CutterPos)) {
					if (i == 7) {
						//���ص�8������ȫ�������ж��ڵ����ڣ�˵�������ؽڵ���޳�
						curr->exist = false;//�ڵ��޳�
					}
					continue;
				}
				else //ֻҪ��һ�����ڵ����⣨�����ǵ���ȫ�������л��ǵ��������ز����ཻ����˵�������ؽڵ���Ҫ���ָ�
				{
					if (curr->children[0] == nullptr) {
						curr->SubDivision();//��������ػ�û�зָ������ô����ֽ�Ϊ8���ӽڵ�
					}
					for (auto child : curr->children) {
						OctreeLine.push(child);//�����ؽڵ�ѹ��ջ
						Cutter(CutterPos, Itera, CutterBox);
					}

					break;
				}

			}
		}
		else {
			addtestCube(curr);
			//addCube(curr);//���뵶���ཻ,����������ӽ���Ƭ���ϣ�Ȼ�󷵻�
			return;
		}

		return;

	};
	void ClearFaces() {
		faces.clear();
	}

	//����ѭ��
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


	//���������Ƭ����testvertices�������������Ʊ������Ĺ���
    void IteraAddtestCube(Octree* curr) {

	    if (!curr->sub)
	    {
		Point3f temp;
		for (int i = 0; i <= 7; i++) {
			temp = GetOctreeVertex(i, curr);
			TestTempVertex[i] = temp;
		}   
		/*
		//��Ƭ��Ϣ                               //������Ϣ
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[5]);
		testVertex.push_back(TestTempVertex[1]); testVertex.push_back(TestTempVertex[7]); 
		testVertex.push_back(TestTempVertex[3]); testVertex.push_back(TestTempVertex[2]);
		testVertex.push_back(TestTempVertex[3]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[2]); testVertex.push_back(TestTempVertex[6]);
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[1]);
		//��Ƭ��Ϣ                               //������Ϣ
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[5]);
		testVertex.push_back(TestTempVertex[4]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[6]); testVertex.push_back(TestTempVertex[2]);
		testVertex.push_back(TestTempVertex[6]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[2]); testVertex.push_back(TestTempVertex[3]);
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[4]);
		//��Ƭ��Ϣ                               //������Ϣ
		testVertex.push_back(TestTempVertex[1]); testVertex.push_back(TestTempVertex[0]);
		testVertex.push_back(TestTempVertex[5]); testVertex.push_back(TestTempVertex[4]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[3]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[2]);
		testVertex.push_back(TestTempVertex[3]); testVertex.push_back(TestTempVertex[0]);
		testVertex.push_back(TestTempVertex[1]); testVertex.push_back(TestTempVertex[5]);
		//��Ƭ��Ϣ                               //������Ϣ
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[6]);
		testVertex.push_back(TestTempVertex[4]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[5]); testVertex.push_back(TestTempVertex[1]);
		testVertex.push_back(TestTempVertex[5]); testVertex.push_back(TestTempVertex[7]);
		testVertex.push_back(TestTempVertex[1]); testVertex.push_back(TestTempVertex[3]);
		testVertex.push_back(TestTempVertex[0]); testVertex.push_back(TestTempVertex[4]);
		//��Ƭ��Ϣ                               //������Ϣ
		testVertex.push_back(TestTempVertex[2]); testVertex.push_back(TestTempVertex[0]);
		testVertex.push_back(TestTempVertex[6]); testVertex.push_back(TestTempVertex[4]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[3]);
		testVertex.push_back(TestTempVertex[7]); testVertex.push_back(TestTempVertex[1]);
		testVertex.push_back(TestTempVertex[3]); testVertex.push_back(TestTempVertex[0]);
		testVertex.push_back(TestTempVertex[2]); testVertex.push_back(TestTempVertex[6]);
		//��Ƭ��Ϣ                               //������Ϣ
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
		    //if (!curr->sub) return;//�ж��Ƿ�����ӽڵ�
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
			faces.push_back(Point3i(TempVertex[0], TempVertex[1], TempVertex[3]));//��������Ƭ�������棬���鲻Ҫ�����ڵ��޳�����Ȳ��ԣ���
			faces.push_back(Point3i(TempVertex[3], TempVertex[2], TempVertex[0]));
			faces.push_back(Point3i(TempVertex[0], TempVertex[4], TempVertex[6]));//֮��Ҫ������ӵķ�����������
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
			//if (!curr->sub) return;//�ж��Ƿ�����ӽڵ�
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
			//if (!curr->sub) return;//�ж��Ƿ�����ӽڵ�
			for (auto child : curr->children) {
				if (child->exist) IteraAddPoints(child);

			}
		}
	}

	//������Ҫ�ĵģ����ӵ��brick�ṹ�İ˲���������˲���ͷ���ڵ�������������������˲���Ȼ��������Ⱦ���У���Ե�bricks����Ϊ64
	void AddPointsForBrick(Octree* curr, int& Itera) {
		testVertex.clear();//��յ�ǰ����Ⱦ����
		IteraAddPointsForBrick(curr, Itera);
	}

	//������Ҫ�ģ��ݹ麯����ͨ������˲����ڵ����һ���ӽڵ㣬�ﵽ�����˲�����Ŀ�ģ����ڲ�ӵ��brick�ķ�Ҷ�ӽڵ㣬������������ֱ�Ӽ������Ⱦ����
	//����ӵ��bricks�ṹ��Ҷ�ӽڵ㣬��ȡ��ָ���bricks���󣬲�����bricks����ȡ��Ҫ��Ⱦ�����ؽ���������Ⱦ���У���Ե�bricks����Ϊ64��
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
			//if (!curr->sub) return;//�ж��Ƿ�����ӽڵ�
			for (auto child : curr->children) {
				if (child && (child->exist)) IteraAddPointsForBrick(child, Itera);//

			}
		}
	}


	//������Ҫ�ģ����Ӧ��bricks�ṹʱʹ�õ����bricks�ṹ�ķָ������Ե�bricks������С��ֻ��64��Ŀǰʹ�õ�bricks����Ϊ512
	void Deeper_cut(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos) {

		Bricks* bri = (Bricks*)curr->bricks;
		//Octree* temp = (Octree*)bri->brick;
		Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 3 / 4,
			curr->origin.y - curr->halfDimension.y * 3 / 4, curr->origin.z - curr->halfDimension.z * 3 / 4);
		Point3f SubTemp;
		Point3f halfDimension = Point3f(curr->halfDimension.x / 4, curr->halfDimension.y / 4, curr->halfDimension.z / 4);
		//cout << "����Ϊ:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";
		for (int k = 0; k < BrickLength; ++k) {
			if (bri->brick[k]) continue;
			SubTemp = SubOrigin;
			SubTemp.x = SubTemp.x + (k & 3) * curr->halfDimension.x / 2;
			SubTemp.y = SubTemp.y + (k & 12) / 4 * curr->halfDimension.y / 2;
			SubTemp.z = SubTemp.z + (k & 48) / 16 * curr->halfDimension.z / 2;

			Point3f OctreeVertex;
			//�������ͨ���ж����������뵶�����ĵľ������ж����Ƿ�Ӧ�ñ��޳��������������Ƿ��ڵ����ڲ�
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
	
	//������Ҫ�ģ���������Ⱦ���ؼ������Ⱦ���У���Ե�bricks����Ϊ512
	void AddPointsForBrick512(Octree* curr, int& Itera) {
		testVertex.clear();
		IteraAddPointsForBrick512(curr, Itera);
	}

	void IteraAddPointsForBrick512(Octree* curr, int& Itera) {
		if (!curr->sub) {
			if (curr->accuracy > Itera && curr->bricks)
			{
				//cout << "��ʼ";
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
						//cout << "��ӳɹ�";
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
			//if (!curr->sub) return;//�ж��Ƿ�����ӽڵ�
			for (auto child : curr->children) {
				//if (child->exist) IteraAddPointsForBrick512(child, Itera);
				if (child && (child->exist)) IteraAddPointsForBrick512(child, Itera);

			}
		}
	}


	//������Ҫ�ģ��Դﵽ�ָ�޵������ٴηָʹ��bricks�ṹ�������Ѿ��ﵽ�ָ�޵İ˲���Ҷ�ӽڵ㣬��������ָ���bricks�ṹ��
    //�жϸ�bricks�д����ÿ���Ƿ��ڵ����ڲ����ڵ����ڲ��򽫸�λ�õ�boolֵ��Ϊtrue��������Ҫ����Ⱦ��
    //bricks�ṹ�е�rendered����Ŀǰ���ڶ�̬���¿���Ⱦ���У���Ӧ�û�����Ҫ�õ�
	void Deeper_cut512(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos) {
		Bricks* bri = (Bricks*)curr->bricks;
		//Octree* temp = (Octree*)bri->brick;

		float precision = 0.125;

		Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 * precision,
			curr->origin.y - curr->halfDimension.y * 7 * precision, curr->origin.z - curr->halfDimension.z * 7 * precision);
		Point3f SubTemp;
		Point3f halfDimension = Point3f(curr->halfDimension.x * precision, curr->halfDimension.y * precision, curr->halfDimension.z * precision);
		//cout << "����Ϊ:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";
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

	//�������surface��ֱ�Ӱ����Ƿ��ཻ��ӽ���Ⱦ�б�
	void Deeper_cut512_ForSurface(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos,byte CurrCode, bool IsSurface) {
		Bricks* bri = (Bricks*)curr->bricks;
		//Octree* temp = (Octree*)bri->brick;

		float precision = 0.125;

		Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 * precision,
			curr->origin.y - curr->halfDimension.y * 7 * precision, curr->origin.z - curr->halfDimension.z * 7 * precision);
		Point3f SubTemp;
		Point3f halfDimension = Point3f(curr->halfDimension.x * precision, curr->halfDimension.y * precision, curr->halfDimension.z * precision);
		//cout << "����Ϊ:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";
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
			int t;//�����־��+����-;
			int n;//�����־��x��y��z;
			int mask1 = 1;
			int mask2 = 1;
			for (Index = 0; Index < 6; Index++) {
				Mask = Mask << Index;
				Mask = CurrCode & Mask;
				if (!Mask) break;
			}
			//��������ĸ����CurrCode�Ѿ������ˣ������������м����
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
					bri->Rendered[k] =true;//������Ⱦ����
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
					mask1 = mask1 | mask2;//maskת��Ϊ000 111 000  /  111 000 111 /  000 000 111

					mask2 = k & mask1;//mask2���Ϊ  111 000 000 / 000 111 000  /  000 000 111 ����0;���ڱ��棻

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
		//cout << "����Ϊ:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";

		Point3f OctreeVertex ;
/* 
* �˴����������������brick��δ�����������ʱӦʹ��push_back����ʱ�������Ƿ��޳�����Ҫ�������Ⱦ�б�Ҳ���Ǳ���Ϊ512��
  �����brick�Ѿ��ڿ���Ⱦ�б��У���ʱ��Ҫͨ��������������ȡ������Ⱦ�б��е�λ�ã�

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


				//�������ͨ���ж����������뵶�����ĵľ������ж����Ƿ�Ӧ�ñ��޳��������������Ƿ��ڵ����ڲ�

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

				if (!bri->Rendered[k]) BrickIndex += 4;//�ﵽ��һ��������������
				if (bri->brick[k]) continue;

				SubTemp = SubOrigin;
				SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
				SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.x / 4;
				SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.x / 4;

				//�������ͨ���ж����������뵶�����ĵľ������ж����Ƿ�Ӧ�ñ��޳��������������Ƿ��ڵ����ڲ�
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
			//cout << "��ʼ";
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
			//if (!curr->sub) return;//�ж��Ƿ�����ӽڵ�
			for (auto child : curr->children) {
				//if (child) RebuildRenderList(child, Itera);
				if (child && (child->exist)) RebuildRenderList(child, Itera);
			}
		}
	}

	//����ཻ�ĺʹ��ڱ����brick����������û�д����ཻ���δ�ܼ������Ⱦ�б��ǿ�������һ�������д����ཻ���brick��
	//��Ϊ����Ⱦ�б����Ѿ�û����λ�ã����Ϊ��������һ�����ÿ���Ⱦ�б�����������ζ�λ���б��е�brick;
	void AddSurfacePoint_Deeper_cut512(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos, byte CurrCode) {
		Bricks* bri = (Bricks*)curr->bricks;
		//Octree* temp = (Octree*)bri->brick;

		float precision = 0.125;

		Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 * precision,
			curr->origin.y - curr->halfDimension.y * 7 * precision, curr->origin.z - curr->halfDimension.z * 7 * precision);
		Point3f SubTemp;
		Point3f halfDimension = Point3f(curr->halfDimension.x * precision, curr->halfDimension.y * precision, curr->halfDimension.z * precision);
		//cout << "����Ϊ:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";

		Point3f OctreeVertex;
		/*
		* �˴����������������brick��δ�����������ʱӦʹ��push_back����ʱ�������Ƿ��޳�����Ҫ�������Ⱦ�б�Ҳ���Ǳ���Ϊ512��
		  �����brick�Ѿ��ڿ���Ⱦ�б��У���ʱ��Ҫͨ��������������ȡ������Ⱦ�б��е�λ�ã�

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


				//�������ͨ���ж����������뵶�����ĵľ������ж����Ƿ�Ӧ�ñ��޳��������������Ƿ��ڵ����ڲ�
				
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

				if (!bri->Rendered[k]) BrickIndex += 4;//�ﵽ��һ��������������
				if (bri->brick[k]) continue;

				SubTemp = SubOrigin;
				SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
				SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.x / 4;
				SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.x / 4;

				//�������ͨ���ж����������뵶�����ĵľ������ж����Ƿ�Ӧ�ñ��޳��������������Ƿ��ڵ����ڲ�
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
				//cout << "��ʼ";
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
						//cout << "��ӳɹ�";
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
			//if (!curr->sub) return;//�ж��Ƿ�����ӽڵ�
			for (auto child : curr->children) {
				if (child) IteraAddPointsForBrick4096(child, Itera);

			}
		}
	}

	//ɾ���Ѿ����޳������ؽڵ㣬������׷���丸�׽ڵ�

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
			//cout << "ɾ��brick" << endl;
			curr->bricks = nullptr;
		}
		FindParent((Octree*)(curr->Parent), curr->index);
		
	}

	//Ѱ�ҵ�ǰ��ɾ���ڵ��л�δ��ɾ���ĺ���brick��Ҷ�ӽڵ�
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
	//ɾ����һ�����������е�brick�ڿ���Ⱦ�б��е�����
	void DeleteBrick() {
		Bricks* temp;
		int BrickIndex;
		while (!BrickChildToBeDelete.empty()) {
			temp = (Bricks*)BrickChildToBeDelete.top()->bricks;

			BrickIndex = BrickChildToBeDelete.top()->RenderedIndex - 4;

			for (int k = 0; k < BrickLength; ++k) {
				if (!temp->Rendered[k]) BrickIndex += 4;//�ﵽ��һ��������������
				if (temp->brick[k]) continue;
				testVertex[BrickIndex + 1] = Point3f(0, 0, 0);
				testVertex[BrickIndex + 3] = Point3f(0, 0, 0);
			}
			delete temp;
			delete BrickChildToBeDelete.top();
			cout << "ɾ��brick��Ҷ��" << endl;
			BrickChildToBeDelete.pop();
		}
	}


	//������Ҫ�ģ�ʹ�õ��߷ָ������˲��������������İ˲����ڵ�
    void Cutter_Dynamic(Point3f& CutterPos, int& Itera, BoundBox& CutterBox, Device& MyDevice) {
        if (OctreeLine.empty()) return;
        
        Octree* curr = OctreeLine.top();
        OctreeLine.pop();

        //if (curr == nullptr || curr->exist == false) { return; };
        
		bool is_intersect = IsIntersect(curr, CutterBox);

        //if (Itera > Iteration || curr->accuracy > Iteration )
		//���˲����ڵ�ﵽ�ָ�޺�
		if (curr->accuracy > Itera )//&& is_intersect
		{
			//�����ǰ�˲���Ҷ�ӽڵ㻹û�б�����bricks�ṹ����ôΪ������һ��
			if (curr->bricks == nullptr) {
				curr->bricks = new Bricks(curr);
			}
			//Init_Data4096(curr, CutterBox, CutterPos, CutterSize, MyDevice);
			//Init_Data512(curr, CutterBox, CutterPos, CutterSize);
			//wΪ��ǰ�˲����ڵ�ִ�и�����ķָbricks����Ϊ512
			Deeper_cut512(curr, CutterBox, CutterPos);
			//Deeper_cut(curr, CutterBox, CutterPos);
             //���������ﵽ��󣬲����ٷָ����ֱ����Ⱦ,����
        	return;
        }
        //else Itera += 1;//��ǰ����������1
        
        if (is_intersect) {
        	//��ʱ���ؽڵ������ȫλ�ڵ����ڲ�����Ҫ������������뵶���ཻ�����ؽڵ�����Ҫ�ٴηָ�
        	//curr->rendered = false;
			BoolNum++;
        	Point3f OctreeVertex;
        	Point3f SubHalfDimension((curr->halfDimension.x / 2), (curr->halfDimension.y / 2), (curr->halfDimension.z / 2));
        
        	float Xoffset, Yoffset,Zoffset;

        	//�ж����ص�8�����뵶�ߵĿռ��ϵ
        	for (int i = 0; i <= 7; ++i) {
        		OctreeVertex = GetOctreeVertex(i, curr);
        		if (IsIn(OctreeVertex, CutterPos)) {
        			if (i == 7) {
        				//���ص�8������ȫ�������ж��ڵ����ڣ�˵�������ؽڵ���޳�
						curr->exist = false;//�ڵ��޳�
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
        		else //ֻҪ��һ�����ڵ����⣨�����ǵ���ȫ�������л��ǵ��������ز����ཻ����˵�������ؽڵ���Ҫ���ָ�
        		{
        			
        			if (curr->sub==false) {
        				curr->SubDivision();//��������ػ�û�зָ������ô����ֽ�Ϊ8���ӽڵ�
						VoxelNum += 7;//���ؽڵ�����8��
        			}
        			for (auto child : curr->children) {
						if (!child) continue;
        				OctreeLine.push(child);//�����ؽڵ�ѹ��ջ
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
		//���˲����ڵ�ﵽ�ָ�޺�
		if (curr->accuracy > Itera )//&& is_intersect
		{
			//�����ǰ�˲���Ҷ�ӽڵ㻹û�б�����bricks�ṹ����ôΪ������һ��
			if (curr->bricks == nullptr) {
				curr->bricks = new Bricks(curr);
			}
			//Init_Data4096(curr, CutterBox, CutterPos, CutterSize, MyDevice);
			//Init_Data512(curr, CutterBox, CutterPos, CutterSize);
			//wΪ��ǰ�˲����ڵ�ִ�и�����ķָbricks����Ϊ512
			Deeper_cut512(curr, CutterBox, CutterPos);
			//Deeper_cut(curr, CutterBox, CutterPos);
			 //���������ﵽ��󣬲����ٷָ����ֱ����Ⱦ,����
			return;
		}
		//else Itera += 1;//��ǰ����������1

		if (is_intersect) {
			//��ʱ���ؽڵ������ȫλ�ڵ����ڲ�����Ҫ������������뵶���ཻ�����ؽڵ�����Ҫ�ٴηָ�
			//curr->rendered = false;
			BoolNum++;
			Point3f OctreeVertex;
			Point3f SubHalfDimension((curr->halfDimension.x / 2), (curr->halfDimension.y / 2), (curr->halfDimension.z / 2));

			float Xoffset, Yoffset, Zoffset;

			//�ж����ص�8�����뵶�ߵĿռ��ϵ
			for (int i = 0; i <= 7; ++i) {
				OctreeVertex = GetOctreeVertex(i, curr);
				if (IsIn(OctreeVertex, CutterPos)) {
					if (i == 7) {
						//���ص�8������ȫ�������ж��ڵ����ڣ�˵�������ؽڵ���޳�
						curr->exist = false;//�ڵ��޳�
						DeleteNode(curr);
						VoxelNum -= 1;
					}
					continue;
				}
				else //ֻҪ��һ�����ڵ����⣨�����ǵ���ȫ�������л��ǵ��������ز����ཻ����˵�������ؽڵ���Ҫ���ָ�
				{
					if (curr->sub == false) {
						curr->SubDivision();//��������ػ�û�зָ������ô����ֽ�Ϊ8���ӽڵ�
						VoxelNum += 7;//���ؽڵ�����8��
					}

					byte ChildCode = 1;
					byte Mask = 1;
					for (auto child : curr->children) {
						if (!child) continue;
						OctreeLine.push(child);//�����ؽڵ�ѹ��ջ
						if (IsSurface) {
							//��ʼ��
							ChildCode = 1;
							Mask = 1;
							//��ȡZά�ȵ�������Ϣ
							ChildCode = ChildCode << (i & 1) + 4;

							//��ȡYά�ȵ�������Ϣ
							Mask = Mask << (2 + (i & 2) / 2);
							ChildCode = ChildCode | Mask;
							Mask = 1;

							//��ȡXά�ȵ�������Ϣ
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
			 //���������ﵽ��󣬲����ٷָ����ֱ����Ⱦ,����
			return;
		}


		//else Itera += 1;//��ǰ����������1

		if (is_intersect) {
			//��ʱ���ؽڵ������ȫλ�ڵ����ڲ�����Ҫ������������뵶���ཻ�����ؽڵ�����Ҫ�ٴηָ�
			//curr->rendered = false;
			BoolNum++;
			//Point3f OctreeVertex;
			Point3f SubHalfDimension((curr->halfDimension.x / 2), (curr->halfDimension.y / 2), (curr->halfDimension.z / 2));

			float Xoffset, Yoffset, Zoffset;

			//�ж����ص�8�����뵶�ߵĿռ��ϵ
			for (int i = 0; i <= 7; ++i) {
				OctreeVertex = GetOctreeVertex(i, curr);
				if (IsIn(OctreeVertex, CutterPos)) {
					if (i == 7) {
						//���ص�8������ȫ�������ж��ڵ����ڣ�˵�������ؽڵ���޳�
						curr->exist = false;//�ڵ��޳�
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
				else //ֻҪ��һ�����ڵ����⣨�����ǵ���ȫ�������л��ǵ��������ز����ཻ����˵�������ؽڵ���Ҫ���ָ�
				{
					if (curr->sub == false) {
						curr->SubDivision();//��������ػ�û�зָ������ô����ֽ�Ϊ8���ӽڵ�
						VoxelNum += 7;//���ؽڵ�����8��
					}
					if (curr->RenderedIndex >=0) {
						
						testVertex[(curr->RenderedIndex) + 1] = Point3f(0, 0, 0);
						testVertex[(curr->RenderedIndex) + 3] = Point3f(0, 0, 0);
						curr->RenderedIndex = -10;
					}


					for (auto child : curr->children) {
						if (!child) continue;
						OctreeLine.push(child);//�����ؽڵ�ѹ��ջ
						AddPoint_Cutter_Dynamic(CutterPos, Itera, CutterBox, MyDevice);
					}
					//Cutter_Dynamic(CutterPos, Itera, CutterBox);
					break;
				}

			}
		}
		else 
		{
			if (curr->sub || curr->RenderedIndex == (-10)) return;//���������Ѿ����ָ�ʱ�����ٽ����������Ⱦ����
			if (curr->RenderedIndex == (-1)) {//����δ���ָ�����ػ�δ�ӿ���Ⱦ����ʱ������������Ⱦ����,-10�����Ѽ��뵫���ֱ�ɾ��
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
		//Currcodeָ����curr�ڵ�ı��룬ͨ�����������curr���ӽڵ�ĵı��봦���õ����ӽڵ��Ƿ�Ӧ�ñ�����
		//IsRendered����ýڵ��Ƿ�ñ�������Ⱦ�б�1-6����������¶����IsRendered/2��ȡ���ĸ�
		if (OctreeLine.empty()) return;
		
		Octree* curr = OctreeLine.top();
		OctreeLine.pop();
		//if (curr == nullptr || curr->exist == false) { return; };

		bool is_intersect = IsIntersect(curr, CutterBox);

		Point3f OctreeVertex = Point3f(0, 0, 0);

		if (curr->accuracy > Itera )//&& is_intersect
		{
			//�ཻ���ߴ��ڱ���
			if (is_intersect || IsSurface) {
				if (curr->bricks == nullptr) {
					curr->bricks = new Bricks(curr);
				}
				curr->sub = true;
				AddPoint_Deeper_cut512(curr, CutterBox, CutterPos);
			}
			 //���������ﵽ��󣬲����ٷָ����ֱ����Ⱦ,����
			return;
		}

		//else Itera += 1;//��ǰ����������1

		if (is_intersect) {
			//��ʱ���ؽڵ������ȫλ�ڵ����ڲ�����Ҫ������������뵶���ཻ�����ؽڵ�����Ҫ�ٴηָ�
			//curr->rendered = false;
			BoolNum++;
			//Point3f OctreeVertex;
			Point3f SubHalfDimension((curr->halfDimension.x / 2), (curr->halfDimension.y / 2), (curr->halfDimension.z / 2));

			float Xoffset, Yoffset, Zoffset;

			//�ж����ص�8�����뵶�ߵĿռ��ϵ
			for (int i = 0; i <= 7; ++i) {
				OctreeVertex = GetOctreeVertex(i, curr);
				if (IsIn(OctreeVertex, CutterPos)) {
					if (i == 7) {
						//���ص�8������ȫ�������ж��ڵ����ڣ�˵�������ؽڵ���޳�
						curr->exist = false;//�ڵ��޳�
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
				else //ֻҪ��һ�����ڵ����⣨�����ǵ���ȫ�������л��ǵ��������ز����ཻ����˵�������ؽڵ���Ҫ���ָ�
				{
					if (curr->sub == false) {
						curr->SubDivision();//��������ػ�û�зָ������ô����ֽ�Ϊ8���ӽڵ�
						VoxelNum += 7;//���ؽڵ�����8��
					}
					if (curr->RenderedIndex >=0) {
						testVertex[(curr->RenderedIndex) + 1] = Point3f(0, 0, 0);
						testVertex[(curr->RenderedIndex) + 3] = Point3f(0, 0, 0);
						curr->RenderedIndex = -10;//-10�����Ѽ��뵫�ֱ�ɾ��������
					}
					byte ChildCode = 1;
					byte Mask = 1; 

					for (auto child : curr->children) {
						if (!child) continue;
						OctreeLine.push(child);//�����ؽڵ�ѹ��ջ

						if (IsSurface) {
							//��ʼ��
							ChildCode = 1;
							Mask = 1;
							//��ȡZά�ȵ�������Ϣ
							ChildCode = ChildCode << (i & 1);

							//��ȡYά�ȵ�������Ϣ
							Mask = Mask << (2 + (i & 2) / 2);
							ChildCode = ChildCode | Mask;
							Mask = 1;

							//��ȡXά�ȵ�������Ϣ
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
				if (curr->sub || curr->RenderedIndex ==(-10)) return;//���������Ѿ����ָ�ʱ�����ٽ����������Ⱦ����
				if (curr->RenderedIndex == (-1)) {//����δ���ָ�����ػ�δ�ӿ���Ⱦ����ʱ������������Ⱦ����
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

		//��������
		PrepareCut(CudaBuffer, CutterPos, CutterSize, CutterBox);

		Point3f NodeTemp = CudaBuffer.HostCutter->SubOrigin;//����ԭʼ���꣬Ҳ����0�Žڵ������
		Point3f BrickOrigin;
		Point3f OctreeVertex;
		Point3f BrickHalfDimension = Point3f(CudaBuffer.HostCutter->halfDimension.x / 16,
			CudaBuffer.HostCutter->halfDimension.y / 16, CudaBuffer.HostCutter->halfDimension.z / 16);

		int BrickIndex;
		int Forward;
		long long int mask = 1;//ʹ�û�|,������bitλ��Ϊ1ʱ������Ѿ�������
		//int mask = 1;

		for (int i = 0; i < BrickLength; i++) {

			if (CudaBuffer.HostBuffer[i].exist) continue;//true�����ѱ�������

			NodeTemp = CudaBuffer.HostCutter->SubOrigin;

			NodeTemp.x = NodeTemp.x + (i & 15) * CudaBuffer.HostCutter->halfDimension.x * 2;
			NodeTemp.y = NodeTemp.y + (i & 240) / 16 * CudaBuffer.HostCutter->halfDimension.y * 2;
			NodeTemp.z = NodeTemp.z + (i & 3840) / 256 * CudaBuffer.HostCutter->halfDimension.z * 2;

			if (CudaBuffer.HostBuffer[i].Sub) {

				BrickOrigin = NodeTemp;//��ǰNode������
				//��ǰnode��0��brick
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
					{//��bitλΪ1������ѱ�����
					    BrickOrigin = TempBrickOrigin;

					    //��ǰbrick������
					    BrickOrigin.x = BrickOrigin.x + (j & 15) * BrickHalfDimension.x * 2;
						BrickOrigin.y = BrickOrigin.y + (j & 240) / 16 * BrickHalfDimension.y * 2;
						BrickOrigin.z = BrickOrigin.z + (j & 3840) / 256 * BrickHalfDimension.z * 2;
					    
					    OctreeVertex = GetOctreeVertexForBrick(0, BrickOrigin, BrickHalfDimension);
					    testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension);
					    
					    OctreeVertex = GetOctreeVertexForBrick(7, BrickOrigin, BrickHalfDimension);
					    testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension * (-1));
				    }
				}
				//cout << "����brick��" << CountBrick << endl;
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
		Point3f NodeTemp = CudaBuffer.HostCutter->SubOrigin;//����ԭʼ���꣬Ҳ����0�Žڵ������
		Point3f BrickOrigin;
		Point3f OctreeVertex;
		Point3f BrickHalfDimension = Point3f(CudaBuffer.HostCutter->halfDimension.x / 16,
			CudaBuffer.HostCutter->halfDimension.y / 16, CudaBuffer.HostCutter->halfDimension.z / 16);

		int BrickIndex;
		int Forward;
		
		//int mask = 1;

		for (int i = 0; i < BrickLength; i++) {

			if (CudaBuffer.HostBuffer[i].exist) continue;//true�����ѱ�������

			NodeTemp = CudaBuffer.HostCutter->SubOrigin;

			NodeTemp.x = NodeTemp.x + (i & 15) * CudaBuffer.HostCutter->halfDimension.x * 2;
			NodeTemp.y = NodeTemp.y + (i & 240) / 16 * CudaBuffer.HostCutter->halfDimension.y * 2;
			NodeTemp.z = NodeTemp.z + (i & 3840) / 256 * CudaBuffer.HostCutter->halfDimension.z * 2;

			if (CudaBuffer.HostBuffer[i].Sub) {

				BrickOrigin = NodeTemp;//��ǰNode������
				//��ǰnode��0��brick
				BrickOrigin.x = BrickOrigin.x - CudaBuffer.HostCutter->halfDimension.x * 15 / 16;
				BrickOrigin.y = BrickOrigin.y - CudaBuffer.HostCutter->halfDimension.y * 15 / 16;
				BrickOrigin.z = BrickOrigin.z - CudaBuffer.HostCutter->halfDimension.z * 15 / 16;
				Point3f TempBrickOrigin = BrickOrigin;
				for (int j = 0; j < BrickLength; j++) {
					BrickIndex = j / 8;
					Forward = j % 8;
					BYTE mask = 1;//ʹ�û�|,������bitλ��Ϊ1ʱ������Ѿ�������

					mask = mask << Forward;

					if (!(CudaBuffer.HostBrick[i].brick[BrickIndex] & mask)){//��bitλΪ1������ѱ�����
						BrickOrigin = TempBrickOrigin;

						//��ǰbrick������
						BrickOrigin.x = BrickOrigin.x + (j & 15) * BrickHalfDimension.x * 2;
						BrickOrigin.y = BrickOrigin.y + (j & 240) / 16 * BrickHalfDimension.y * 2;
						BrickOrigin.z = BrickOrigin.z + (j & 3840) / 256 * BrickHalfDimension.z * 2;

						OctreeVertex = GetOctreeVertexForBrick(0, BrickOrigin, BrickHalfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension);
						OctreeVertex = GetOctreeVertexForBrick(7, BrickOrigin, BrickHalfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension * (-1));
					}
				}
				//cout << "����brick��" << CountBrick << endl;
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
		Point3f NodeTemp = CudaBuffer.HostCutter->SubOrigin;//����ԭʼ���꣬Ҳ����0�Žڵ������
		Point3f BrickOrigin;
		Point3f OctreeVertex;
		Point3f BrickHalfDimension = Point3f(CudaBuffer.HostCutter->halfDimension.x / 32,
			CudaBuffer.HostCutter->halfDimension.y / 32, CudaBuffer.HostCutter->halfDimension.z / 32);

		int BrickIndex;
		int Forward;

		//int mask = 1;

		for (int i = 0; i < BrickLength; i++) {

			if (CudaBuffer.HostBuffer[i].exist) continue;//true�����ѱ�������

			NodeTemp = CudaBuffer.HostCutter->SubOrigin;

			NodeTemp.x = NodeTemp.x + (i & 15) * CudaBuffer.HostCutter->halfDimension.x * 2;
			NodeTemp.y = NodeTemp.y + (i & 240) / 16 * CudaBuffer.HostCutter->halfDimension.y * 2;
			NodeTemp.z = NodeTemp.z + (i & 3840) / 256 * CudaBuffer.HostCutter->halfDimension.z * 2;

			if (CudaBuffer.HostBuffer[i].Sub) {

				BrickOrigin = NodeTemp;//��ǰNode������
				//��ǰnode��0��brick
				BrickOrigin.x = BrickOrigin.x - CudaBuffer.HostCutter->halfDimension.x * 31 / 32;
				BrickOrigin.y = BrickOrigin.y - CudaBuffer.HostCutter->halfDimension.y * 31 / 32;
				BrickOrigin.z = BrickOrigin.z - CudaBuffer.HostCutter->halfDimension.z * 31 / 32;
				Point3f TempBrickOrigin = BrickOrigin;
				for (int j = 0; j < BrickLength * 8; j++) {
					BrickIndex = j / 8;
					Forward = j % 8;
					BYTE mask = 1;//ʹ�û�|,������bitλ��Ϊ1ʱ������Ѿ�������

					mask = mask << Forward;

					if (!(CudaBuffer.HostBrick[i].brick[BrickIndex] & mask)) {//��bitλΪ1������ѱ�����
						BrickOrigin = TempBrickOrigin;

						//��ǰbrick������
						BrickOrigin.x = BrickOrigin.x + (j & 31) * BrickHalfDimension.x * 2;
						BrickOrigin.y = BrickOrigin.y + (j & 992) / 32 * BrickHalfDimension.y * 2;
						BrickOrigin.z = BrickOrigin.z + (j & 31744) / 1024 * BrickHalfDimension.z * 2;

						OctreeVertex = GetOctreeVertexForBrick(0, BrickOrigin, BrickHalfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension);
						OctreeVertex = GetOctreeVertexForBrick(7, BrickOrigin, BrickHalfDimension);
						testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension * (-1));
					}
				}
				//cout << "����brick��" << CountBrick << endl;
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
		Point3f NodeTemp = CudaBuffer.HostCutter->SubOrigin;//����ԭʼ���꣬Ҳ����0�Žڵ������
		Point3f BrickOrigin;
		Point3f OctreeVertex;
		Point3f BrickHalfDimension = Point3f(CudaBuffer.HostCutter->halfDimension.x / 16,
			CudaBuffer.HostCutter->halfDimension.y / 16, CudaBuffer.HostCutter->halfDimension.z / 16);

		int BrickIndex;
		int Forward;

		//int mask = 1;
		for (int OctreeNodeNumber = 0; OctreeNodeNumber < 8; OctreeNodeNumber++){
			for (int i = 0; i < BrickLength; i++) {

				if (CudaBuffer.HostBuffer[OctreeNodeNumber * 4096 + i].exist) continue;//true�����ѱ�������

				NodeTemp = CudaBuffer.HostOctree[OctreeNodeNumber].Origin;

				NodeTemp.x = NodeTemp.x + (i & 15) * CudaBuffer.HostCutter->halfDimension.x * 2;//HostCutter�б������Node�ڵ�ĳߴ���Ϣ
				NodeTemp.y = NodeTemp.y + (i & 240) / 16 * CudaBuffer.HostCutter->halfDimension.y * 2;
				NodeTemp.z = NodeTemp.z + (i & 3840) / 256 * CudaBuffer.HostCutter->halfDimension.z * 2;

				if (CudaBuffer.HostBuffer[i].Sub) {

					BrickOrigin = NodeTemp;//��ǰNode������
					//��ǰnode��0��brick
					BrickOrigin.x = BrickOrigin.x - CudaBuffer.HostCutter->halfDimension.x * 15 / 16;
					BrickOrigin.y = BrickOrigin.y - CudaBuffer.HostCutter->halfDimension.y * 15 / 16;
					BrickOrigin.z = BrickOrigin.z - CudaBuffer.HostCutter->halfDimension.z * 15 / 16;
					Point3f TempBrickOrigin = BrickOrigin;
					for (int j = 0; j < BrickLength; j++) {
						BrickIndex = j / 8;
						Forward = j % 8;
						BYTE mask = 1;//ʹ�û�|,������bitλ��Ϊ1ʱ������Ѿ�������

						mask = mask << Forward;

						if (!(CudaBuffer.HostBrick[OctreeNodeNumber * 4096 + i].brick[BrickIndex] & mask)) {//��bitλΪ1������ѱ�����
							BrickOrigin = TempBrickOrigin;

							//��ǰbrick������
							BrickOrigin.x = BrickOrigin.x + (j & 15) * BrickHalfDimension.x * 2;
							BrickOrigin.y = BrickOrigin.y + (j & 240) / 16 * BrickHalfDimension.y * 2;
							BrickOrigin.z = BrickOrigin.z + (j & 3840) / 256 * BrickHalfDimension.z * 2;

							OctreeVertex = GetOctreeVertexForBrick(0, BrickOrigin, BrickHalfDimension);
							testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension);
							OctreeVertex = GetOctreeVertexForBrick(7, BrickOrigin, BrickHalfDimension);
							testVertex.push_back(OctreeVertex); testVertex.push_back(BrickHalfDimension * (-1));
						}
					}
					//cout << "����brick��" << CountBrick << endl;
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
		//��Node�������Ϣ���Ʒ��ظ�Host��
		cudaMemcpy(CudaBuffer.HostBuffer, CudaBuffer.DeviceBuffer, BrickLength * sizeof(Node), cudaMemcpyDeviceToHost);
		//��brick�������Ϣ���Ʒ��ظ�host��
		cudaMemcpy(CudaBuffer.HostBrick, CudaBuffer.DeviceBrick, BrickLength * sizeof(BitBricks), cudaMemcpyDeviceToHost);
	}

};

