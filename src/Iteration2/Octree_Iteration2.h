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

	vector<RenderData> testVertex;

	stack<Octree*>  OctreeLine;//�洢δ���жϵ����ؽڵ�
	stack<Octree*> RenderedCube;
	stack<Octree*> BrickChildToBeDelete;

	byte ChildPosCode[8];
	float Precesion[14];

	OctreeCut_Iteration2(Octree& copy, Point3f Center, Point3f CS) :C(Center), CutterSize(CS) {
		Head = new Octree(copy);

		byte ChildCode;
		byte Mask;

		for (int i = 0; i < 8; i++) {
			//��ʼ��
			ChildCode = 1;
			Mask = 1;
			//��ȡ�ӽڵ��ڸ��ڵ��Zά�ȵĲ�λ��Ϣ
			ChildCode = ChildCode << ((i & 1) + 4);

			//��ȡ�ӽڵ��ڸ��ڵ��Yά�ȵĲ�λ��Ϣ
			Mask = Mask << ((i & 2) / 2 + 2);
			ChildCode = ChildCode | Mask;

			//��ȡ�ӽڵ��ڸ��ڵ��Xά�ȵĲ�λ��Ϣ
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


	//���������ͽڵ�ָ�룬��ȡ�ڵ����ص�ĳ�������λ��
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

	void ReadPathFile(const char* cfilename) {
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
		float Z = (OctreeVer.z - CutPosition.z);
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

	//������Ҫ�ģ���������Ⱦ���ؼ������Ⱦ���У���Ե�bricks����Ϊ512
	void AddPointsForBrick512(Octree* curr, int& Itera) {
		testVertex.clear();
		IteraAddPointsForBrick512(curr, Itera);
	}

	void IteraAddPointsForBrick512(Octree* curr, int& Itera) {
		if (!curr->sub) {
			RenderData RD;
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
						//RenderData RD = RenderData(OctreeVertex, curr->accuracy + 1 + 3);
						testVertex.push_back(RD);
						OctreeVertex = GetOctreeVertexForBrick(7, SubTemp, halfDimension);
						//RD.Postion = OctreeVertex;RD.Itera *= (-1);
						testVertex.push_back(RD); //testVertex.push_back(halfDimension * -1);
						//cout << "��ӳɹ�";
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
			SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
			SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.x / 4;
			SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.x / 4;

			Point3f OctreeVertex;
			if (IsIn(SubTemp, CutterPos)) {
				bri->brick[k] = true;
			}
		}
	}

	//�������surface��ֱ�Ӱ����Ƿ��ཻ��ӽ���Ⱦ�б�
	void Deeper_cut512_ForSurface(Octree* curr, BoundBox& CutterBox, Point3f& CutterPos, byte CurrCode, bool IsSurface) {

		Bricks* bri = (Bricks*)curr->bricks;
		//Octree* temp = (Octree*)bri->brick;

		float precision = 0.125;

		Point3f SubOrigin = Point3f(curr->origin.x - curr->halfDimension.x * 7 * precision,
			curr->origin.y - curr->halfDimension.y * 7 * precision, curr->origin.z - curr->halfDimension.z * 7 * precision);
		Point3f SubTemp;
		Point3f halfDimension = Point3f(curr->halfDimension.x * precision, curr->halfDimension.y * precision, curr->halfDimension.z * precision);
		//cout << "����Ϊ:(" << halfDimension.x << "," << halfDimension.y << "," << halfDimension.z << ")  ";


		if (!IsSurface) {//������Ǳ������أ���ôֱ�Ӹ������Ƿ��뵶�߰�Χ���ཻ�������Ƿ������Ⱦ�б�
			//cout << "�Ǳ���" << endl;
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
				//�ж��Ƿ��뵶���ཻ���Ӷ��ж��Ƿ�Ӧ�ñ���Ⱦ
				for (int i = 0; i <= 7; ++i) {
					OctreeVertex = GetOctreeVertexForBrick(i, SubTemp, halfDimension);
					if (IsIn(OctreeVertex, CutterPos)) {
						bri->Rendered[k] = true;
						break;
					}
				}
				//if (!bri->Rendered[k]) cout << "������Ⱦ" << endl;
			}

			if (!bri->sum) DeleteNode(curr);
		}
		else {
			//cout << "����" << endl;
			byte Mask = 1;
			int Index;
			int t;//�����־��+����-;
			int n;//�����־��x��y��z;
			int mask1 = 1;
			int mask2 = 1;
			for (Index = 0; Index < 6; Index++) {
				Mask = 1;
				Mask = Mask << Index;
				Mask = CurrCode & Mask;
				if (Mask) break;
			}
			//��������ĸ����CurrCode�Ѿ������ˣ������������м����
			for (int k = 0; k < BrickLength; ++k) {

				if (bri->brick[k]) continue;

				SubTemp = SubOrigin;
				SubTemp.x = SubTemp.x + (k & 7) * curr->halfDimension.x / 4;
				SubTemp.y = SubTemp.y + (k & 56) / 8 * curr->halfDimension.y / 4;
				SubTemp.z = SubTemp.z + (k & 448) / 64 * curr->halfDimension.z / 4;

				//�ж��Ƿ��ڵ����ڲ���Ҳ����˵�Ƿ�����
				if (IsIn(SubTemp, CutterPos)) {
					bri->brick[k] = true;//�����brick����
					bri->Rendered[k] = false;//����������Ⱦ�б�
					bri->sum--;
					continue;
				}


				Point3f OctreeVertex;
				//�жϸ������Ƿ��뵶���ཻ�����߰�Χ�в�׼ȷ�����ֱ��ʹ�õ���
				for (int i = 0; i <= 7; ++i) {
					OctreeVertex = GetOctreeVertexForBrick(i, SubTemp, halfDimension);
					if (IsIn(OctreeVertex, CutterPos)) {
						bri->Rendered[k] = true;
						break;
					}
				}

				if (bri->Rendered[k] = true) { continue; }//����Ѿ���������Ⱦ�б���ֱ��������������Ҫ�ٴ��ж����Ƿ�Ϊ��������
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


				//�������ͨ���ж����������뵶�����ĵľ������ж����Ƿ�Ӧ�ñ��޳��������������Ƿ��ڵ����ڲ�

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
					testVertex[BrickIndex + 1].Postion = Point3f(0, 0, 0);
					testVertex[BrickIndex + 3].Postion = Point3f(0, 0, 0);
				}
				//else{bri->brick[k] = true;}
			}

			if (bri->sum == 0) DeleteNode(curr);

		}
	}

	//��ӱ�������
	void AddSurfacePointsForBrick512(Octree* curr, int& Itera) {
		testVertex.clear();
		IteraAddSurfacePointsForBrick512(curr, Itera);
	}

	//��ӱ�������
	void IteraAddSurfacePointsForBrick512(Octree* curr, int& Itera) {
		if (!curr->sub) {
			RenderData RD;
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
					
						//cout << "��ӳɹ�";
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
			//if (!curr->sub) return;//�ж��Ƿ�����ӽڵ�
			for (auto child : curr->children) {
				//if (child->exist) IteraAddPointsForBrick512(child, Itera);
				if (child && (child->exist)) IteraAddPointsForBrick512(child, Itera);
			}
		}
	}

	//ɾ���Ѿ����޳������ؽڵ㣬������׷���丸�׽ڵ�

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
			//cout << "ɾ��brick" << endl;
			curr->bricks = nullptr;
		}
		FindParent((Octree*)(curr->Parent), curr->index);

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
				testVertex[BrickIndex + 1].Postion = Point3f(0, 0, 0);
				testVertex[BrickIndex + 3].Postion = Point3f(0, 0, 0);
			}
			delete temp;
			delete BrickChildToBeDelete.top();
			cout << "ɾ��brick��Ҷ��" << endl;
			BrickChildToBeDelete.pop();
		}
	}

	//��Ա������صĶ�̬��������
	void Cutter_Dynamic_Surface(Point3f& CutterPos, int& Itera, BoundBox& CutterBox, byte CurrCode, bool IsSurface) {
		if (OctreeLine.empty()) return;

		Octree* curr = OctreeLine.top();
		OctreeLine.pop();

		//if (curr == nullptr || curr->exist == false) { return; };

		bool is_intersect = IsIntersect(curr, CutterBox);

		//if (Itera > Iteration || curr->accuracy > Iteration )
		//���˲����ڵ�ﵽ�ָ�޺�
		if (curr->accuracy > Itera && is_intersect)//&& is_intersect
		{
			//�����ǰ�˲���Ҷ�ӽڵ㻹û�б�����bricks�ṹ����ôΪ������һ��
			if (curr->bricks == nullptr) {
				curr->bricks = new Bricks(curr);
			}
			//Init_Data4096(curr, CutterBox, CutterPos, CutterSize, MyDevice);
			//Init_Data512(curr, CutterBox, CutterPos, CutterSize);
			//wΪ��ǰ�˲����ڵ�ִ�и�����ķָbricks����Ϊ512
			//Deeper_cut512(curr, CutterBox, CutterPos);
			Deeper_cut512_ForSurface(curr, CutterBox, CutterPos, CurrCode, IsSurface);
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

					byte ChildCode = 0;
					//byte Mask = 1;
					for (auto child : curr->children) {
						if (!child) continue;
						OctreeLine.push(child);//�����ؽڵ�ѹ��ջ
						ChildCode = 0;
						if (IsSurface) {
							/*
							//��ʼ��
							ChildCode = 1;
							Mask = 1;
							//��ȡZά�ȵ�������Ϣ
							ChildCode = ChildCode << ((child->index & 1) + 4);

							//��ȡYά�ȵ�������Ϣ
							Mask = Mask << ((child->index & 2) / 2 + 2);
							ChildCode = ChildCode | Mask;

							//��ȡXά�ȵ�������Ϣ
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