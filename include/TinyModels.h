#ifndef TINYMODELS_H
#define TINYMODELS_H
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <fbxsdk.h>

#define MODELFORMAT_ERROR -1
#define MODELFORMAT_FBX 0
#define MODELFORMAT_OBJ 1

/*template<typename Type>
struct TVec2
{
	template<typename 
	TVec2(Type 
	union
	{
		Type x, r;
	};	

	union 
	{
		Type y, g;
	};
};

template<typename Type>
struct TVec3 : TVec2
{
	union
	{
		Type z, b;
	};
};

template<typename Type>
struct TVec4 : TVec3
{
	union
	{
		Type w, a;
	};
};*/

template<typename Type>
struct TVertex
{	
	enum TOffsets
	{
		TPositionOffset = 0,
		TColorOffset = TPositionOffset + sizeof(Type) * 4,
		TNormalOffset = TColorOffset + sizeof(Type) * 4,
		TTangentOffset = TNormalOffset + sizeof(Type) * 4,
		TBiNormalOffset = TTangentOffset + sizeof(Type) * 4,
		TIndicesOffset = TBiNormalOffset + sizeof(Type) * 4,
		TWeightsOffset = TIndicesOffset + sizeof(Type) * 4,
		TUVOffset = TWeightsOffset + sizeof(Type) * 4
	};

	Type Position[4]; 
	Type Color[4]; 
	Type Normal[4]; 
	Type Tangent[4];
	Type BiNormal[4]; 
	Type Indices[4];
	Type Weights[4];

	Type UV[2];
    Type UV2[2];

	int FBXControlPointIndex;
};

template<typename Type>
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

	TMaterial() : Ambient(0), Diffuse(0), Specular(0), Emissive(0)
	{
		memset(Name, 0, 255);
		memset(TextureFileNames, 0, TextureTypes_Count * 255);
		memset(TextureIDs, 0, TextureTypes_Count * (sizeof(unsigned int)));
	}

	char Name[255];
	Type Ambient[4];
	Type Diffuse[4];
	Type Specular[4];
	Type Emissive[4];

	char TextureFileNames[TextureTypes_Count][255];
	unsigned int TextureIDs[TextureTypes_Count];
};

#define TNODE 0
#define TMESH 1
#define TLIGHT 2
#define TCAMERA 3

template<typename Type>
class TNode
{
public:

	TNode() : NodeType(TNODE), Parent(nullptr), UserData(nullptr), Name(0)
	{
		for(unsigned int TransformIter = 0; TransformIter < 4; TransformIter++)
		{
			LocalTransform[TransformIter * 5] = 1;
			GlobalTransform[TransformIter * 5] = 1;
		}
	}

	virtual ~TNode()
	{
#if defined(_MSC_VER)
		for each(auto Iter in Children)
#else
		for (auto Iter : Children)
#endif
		{
			delete *Iter;
		}
	}

	unsigned int NodeType;
	char Name[255];
	Type LocalTransform[16];
	Type GlobalTransform[16];
	
	TNode* Parent;
	std::vector<TNode*> Children;
	void* UserData;
};

template<typename Type>
class TMeshNode : public TNode<Type>
{
public:

	TMeshNode() : Material(nullptr)
	{
	   	//NodeType = TMESH;
	}

	virtual ~TMeshNode(){};
	TMaterial<Type>* Material;
	std::vector<TVertex<Type>> Vertices;
	std::vector<unsigned int> Indices;
};

template<typename Type>
class TLightNode : public TNode<Type>
{
public:

	TLightNode(){};//) : NodeType(TLIGHT){};
	virtual ~TLightNode(){};

	enum TLightType
	{
		TPoint = 0,
		TDirectional,
		TSpot,
	};

	TLightType LightType;
	bool On;
	Type Color;
	
	Type InnerAngle;
	Type OuterAngle;

	Type Attenuation[4];
};

template<typename Type>
class TCameraNode : public TNode<Type>
{
public:

	TCameraNode(){};// : NodeType(TCAMERA){};
	virtual ~TCameraNode(){};

	Type AspectRatio;
	Type FOV;
	Type Near;
	Type Far;
	Type ViewMatrix[16];
};

template<typename Type>
class TKeyFrame
{
	TKeyFrame() : Key(0)
	{
		Rotation[3] = 1;
		Translation[3] = 1;
		Scale[3] = 1;
	}
	unsigned int Key;
	Type Rotation[4];
	Type Translation[4];
	Type Scale[4];
};

template<typename Type>
class TTrack
{
	TTrack() : BoneIndex(0), KeyFrameCount(0), KeyFrames(nullptr){};
	~TTrack(){};

	unsigned int BoneIndex;
	unsigned int KeyFrameCount;
	TKeyFrame<Type>* KeyFrames;

};

template<typename Type>
struct TAnimation
{
	unsigned int TotalFrames() const
	{
		return EndFrame - StartFrame;
	}

	float TotalTime(float FPS = 24.0f) const
	{
		return (EndFrame - StartFrame) / FPS;
	}

	char Name[255];
	unsigned int TrackCount;
	TTrack<Type>* Tracks;
	unsigned int StartFrame;
	unsigned int EndFrame;
};

template<typename Type>
struct TSkeleton
{
public:
	TSkeleton() : 
		BoneCount(0), Nodes(nullptr), Bones(nullptr), 
		BindPoses(nullptr), UserData(nullptr){};

	~TSkeleton()
	{
		delete Nodes;
		delete Bones;
		delete BindPoses;
	}

	void Evaluate(const TAnimation<Type>* Animation,
			float Time, bool Loop = true, float FPS = 24.0f);

	unsigned int BoneCount;
	TNode<Type>** Nodes;
	Type** Bones;
	Type** BindPoses;
	void* UserData;
};

template<typename Type>
struct TScene
{
	TMeshNode* GetMeshByName(const char* Name)
	{
		auto MeshIter = Meshes.find(Name);
		if(MeshIter != Meshes.end())
		{
			return MeshIter->second();
		}
		return nullptr;
	}

	TLightNode* GetLightByName(const char* Name)
	{
		auto LightIter = Lights.find(Name);
		if(LightIter != Lights.end())
		{
			return LightIter->second();
		}
		return nullptr;
	}

	TCameraNode* GetCameraByName(const char* Name)
	{
		auto CameraIter = Cameras.find(Name);
		if(CameraIter != Cameras.end())
		{
			return CameraIter->second;
		}
		return nullptr;
	}

	TMaterial* GetMaterialByName(const char* Name)
	{
		auto MaterialIter = Materials.find(Name);
		if(MaterialIter != Materials.end())
		{
			return MaterialIter->second;
		}
		return nullptr;
	}

	TAnimation* GetAnimationByName(const char* Name)
	{
		auto AnimationIter = Animation.find(Name);
		if(AnimationIter != Animations.end())
		{
			return AnimationIter->second;
		}
		return nullptr;
	}

	TMeshNode* GetMeshByIndex(unsigned int Index)
	{
		TMeshNode* NewMesh = nullptr;
		auto MeshIter = Meshes.begin();
		unsigned int Size = Meshes.size();

		for(unsigned int IndexIter = 0;
				IndexIter <= Index && Index < Size;
				IndexIter++, MeshIter++)
		{
			NewMesh = MeshIter->second;
		}
		return NewMesh;
	}

	TLightNode* GetLightByIndex(unsigned int Index)
	{
	TLightNode* NewLight = nullptr;
	auto LightIter = Lights.begin();
	unsigned int Size = Lights.size();

	for(unsigned int IndexIter = 0;
			IndexIter <= Index && Index < Size;
			IndexIter++, LightIter++)
		{
			NewLight = LightIter->second;
		}
		return NewLight;
	}

	TCameraNode* GetCameraByIndex(unsigned int Index)
	{
		TCameraNode* NewCamera = nullptr;
		auto CameraIter = Cameras.begin();
		unsigned int Size = Cameras.size();

		for(unsigned int IndexIter = 0;
				IndexIter <= Index && Index < Size;
				IndexIter++, CameraIter++)
		{
			NewCamera = CameraIter->second;
		}
		return NewCamera;
	}

	TMaterial* GetMaterialByIndex(unsigned int Index)
	{
		TMaterial* NewMaterial = nullptr;
		auto MaterialIter = Materials.begin();
		unsigned int Size = Materials.size();

		for(unsigned int IndexIter = 0;
				IndexIter <= Index && Index < Size;
				IndexIter++, MaterialIter++)
		{
			NewMaterial = MaterialIter->second;
		}
		return NewMaterial;
	}

	TAnimation* GetAnimationByIndex(unsigned int Index)
	{
		TAnimation* NewAnimation = nullptr;
		auto AnimationIter = Animations.begin();
		unsigned int Size = Animations.size();

		for(unsigned int IndexIter = 0;
				IndexIter <= Index && Index < Size;
				IndexIter++, AnimationIter++)
		{
			NewAnimation = AnimationIter->second;
		}
		return NewAnimation;
	}

	void Unload()
	{
		delete Root;
		Root = nullptr;

#ifdef _MSC_VER
		for each(auto Iter in Materials)
#else
		for(auto Iter : Materials)
#endif
		{
			delete Iter;
		}

		for(unsigned int SkeletonIter = 0; SkeletonIter < Skeletons.size(); SkeletonIter++)
		{
			delete Skeletons[SkeletonIter];
		}

#ifdef _MSC_VER
		for each(auto Iter in Animations)
#else
		for(auto Iter : Animations)
#endif
		{
			for(unsigned int AnimationIter = 0; AnimationIter < Iter->TrackCount; AnimationIter++)
			{
				delete[] Iter.second->Tracks[AnimationIter].KeyFrames;
				delete[] Iter.second->Tracks;
				delete Iter.second;
			}
		}

		Meshes.clear();
		Lights.clear();
		Cameras.clear();
		Materials.clear();
		Animations.clear();

		Skeletons.clear();
	}

	void CollectBones(void* Objects)
	{
		TNode* NewNode = (TNode*)Objects;

		if(NewNode->GetNodeAttribute() != nullptr &&
				NewNode->GetNodeAttribute()->GetAttributeType() == 
				TNodeAttribute::Skeleton)
		{
			unsigned int Index = 
		}

	TMeshNode* 
	TNode<Type>* Root;

	const char Path[255];
	Type AbientLight[4];
	
	std::map<const char*, TMeshNode<Type>*> Meshes;
	std::map<const char*, TLightNode<Type>*> Lights;
	std::map<const char*, TCameraNode<Type>*> Cameras;
	std::map<const char*, TMaterial<Type>*> Materials;
	std::map<const char*, TAnimation<Type>*> Animations;

	std::vector<TSkeleton<Type>*> Skeletons;
	};

template<typename Type>
class ModelManager
{
public:
	ModelManager()
	{
		Assistor = nullptr; 
	}

private:

	struct ImportAssistor
	{
		ImportAssistor() : Evaluator(nullptr){}
		~ImportAssistor() 
		{
			Evaluator = nullptr;
		}

		FBXScene* CurrentScene;
		FbxAnimEvaluator* Evaluator;
		std::vector<TNode*> Bones;

		std::map<const char*, unsigned int> BoneIndexMap;
	};

	ImportAssistor* Assistor;
	
	std::map<const char*, TScene<Type>*> Scenes;

	void DisplayContent(TScene* Scene);
	void DisplayHierarchy(TScene* Scene);
	void DisplayPose(TScene* Scene);
	void DisplayAnimation(TScene* Scene);

};
#endif
