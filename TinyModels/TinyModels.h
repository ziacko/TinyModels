#ifndef TINYMODELS_H
#define TINYMODELS_H
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <fbxsdk.h>

#define MODELFORMAT_ERROR -1
#define MODELFORMAT_FBX 0
#define MODELFORMAT_OBJ 1

struct TVec2
{
	union
	{
		double x, r;
	};	

	union 
	{
		double y, g;
	};
};

struct TVec3 : TVec2
{
	union
	{
		double z, b;
	};
};

struct TVec4 : TVec3
{
	union
	{
		double w, a;
	};
};

struct TVertex
{
	
	enum TOffsets
	{
		TPositionOffset = 0,
		TColorOffset = TPositionOffset + sizeof(TVec4),
		TNormalOffset = TColorOffset + sizeof(TVec4),
		TTangentOffset = TNormalOffset + sizeof(TVec4),
		TBiNormalOffset = TTangentOffset + sizeof(TVec4),
		TIndicesOffset = TBiNormalOffset + sizeof(TVec4),
		TWeightsOffset = TIndicesOffset + sizeof(TVec4),
		TUVOffset = TIndicesOffset + sizeof(TVec4)
	};

	TVec4 Position, 
		Color, 
		Normal, 
		Tangent,
		BiNormal, 
		Indices, 
		weights;

	TVec2 UV, UV2;

	int FBXControlPointIndex;

};

struct TMaterial
{
	enum TextureTypes
	{
		TDiffuseTexture = 0,
		TAmbientTexture,
		TGlowTexture,
		TSpecularTexture,
		TGlossTexture,
		TNormalTexture,
		TAlphaTexture,
		TDisplacementTexture,
		TextureTypes_Count
	};

	char Name[255];

};

struct TNode
{

	std::vector<TNode*> Children;
};

struct TMesh
{

};

struct TScene
{

};

class TinyModels
{


};
#endif