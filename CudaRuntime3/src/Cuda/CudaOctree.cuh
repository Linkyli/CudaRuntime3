#include <iostream>
#include <Windows.h>
#include<stdio.h>
#include<vector>
#include<../src/Octree/Octree.h>
#include"../src/Octree/MyStruct.h">

using namespace std;
CudaOctree AllocMemoryForCudaOctree(Point3f& origin, Point3f& halfDimension);
CudaOctree AllocMemoryForCudaOctreeForThreeLevel(Point3f& origin, Point3f& halfDimension);
void PrepareCut(CudaOctree& CudaBuffer, Point3f& CutterPos, Point3f& CutterSize, BoundBox& CutterBox);
void PrepareCutForThreeLevel(CudaOctree& CudaBuffer, Point3f& CutterPos, Point3f& CutterSize, BoundBox& CutterBox);