#ifndef TINYMODELS_H
#define TINYMODELS_H
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <fbxsdk.h>
#include <algorithm>

#define PI 3.14159265359f
#define TAU 6.28318530717958657692f
#define HALFPI 1.57079632679489661923f;
#define THREEHALFPI 4.71238898038468985769f;

#define	EPSILON 0.00000000001f;
#define	DEG2RAD 0.01745329251994329577f;
#define	RAD2DEG 57.2957795130823208768f;

inline int  Max(int x, int y){ return (x > y) ? x : y; }
inline int  Min(int x, int y){ return (x < y) ? x : y; }
inline float  Maxf(float x, float y){ return (x > y) ? x : y; }
inline float  Minf(float x, float y){ return (x < y) ? x : y; }


template <typename Type> class ModelManager;


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

	TMaterial()
	{
		Ambient = new Type[4];
		Diffuse = new Type[4];
		Specular = new Type[4];
		Emissive =  new Type[4];

		memset(Name, 0, 255);
		memset(TextureFileNames, 0, TextureTypes_Count * 255);
		memset(TextureIDs, 0, TextureTypes_Count * (sizeof(unsigned int)));
	}

	char Name[255];
	Type* Ambient;
	Type* Diffuse;
	Type* Specular;
	Type* Emissive;

	char TextureFileNames[TextureTypes_Count][255];
	unsigned int TextureIDs[TextureTypes_Count];
};

template<typename Type>
class TNode
{
public:

	enum TNodeType
	{
		TNODE = 0,
		TMESH,
		TLIGHT,
		TCAMERA
	};

	TNode() : NodeType(TNODE), Parent(nullptr), UserData(nullptr)
	{
		Name = 0;
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
			delete Iter;
		}
	}

	unsigned int NodeType;
	char* Name;
	Type LocalTransform[16];
	Type GlobalTransform[16];
	
	TNode<Type>* Parent;
	std::vector<TNode<Type>*> Children;
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
	Type Color[4];
	
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
public:
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
public:
	TTrack() : BoneIndex(0), KeyFrameCount(0), KeyFrames(nullptr){};
	~TTrack(){};

	unsigned int BoneIndex;
	unsigned int KeyFrameCount;
	TKeyFrame<Type>* KeyFrames;

};

template<typename Type>
struct TAnimation
{
public:
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
		float Time, bool Looping = true, float FPS = 24.0f)
	{
		if (Animation != nullptr)
		{
			int AnimationFrames = Animation->EndFrame - Animation->StartFrame;
			float AnimationDuration = AnimationFrames / FPS;

			float FrameTime = 0;
			if (Looping)
			{
				FrameTime = Maxf(fmod(Time, AnimationDuration), 0);
			}
			else
			{
				FrameTime = Minf(Maxf(Time, 0), AnimationDuration);
			}

			unsigned int Frame = Animation->StartFrame + (int)(FrameTime * FPS);

			const TTrack<Type>* Track = nullptr;
			const TKeyFrame<Type>* Start = nullptr;
			const TKeyFrame<Type>* End = nullptr;

			for (unsigned int i = 0; i < Animation->TrackCount;
				i++, Track = &(Animation->Tracks[i]), Start = nullptr, End = nullptr)
			{
				for (unsigned int j = 0; j < Track->KeyFrameCount - 1; j++)
				{
					if (Track->KeyFrames[j].Key <= Frame &&
						Track->KeyFrames[j + 1].Key >= Frame)
					{
						Start = &(Track->KeyFrames[j]);
						End = &(Track->KeyFrames[j + 1]);
						break;
					}
				}

				if (Start != nullptr && End != nullptr)
				{
					float StartTime = (Start->Key - Animation->StartFrame) / FPS;
					float EndTime = (End->Key - Animation->StartFrame) / FPS;

					float FScale = Maxf(0, Minf(1, (FrameTime - StartTime) / (EndTime - StartTime)));

					Type Transform[4];
					Type Scale[4];

					for (unsigned int j = 0; j < 4; j++)
					{
						Transform[j] = Start->Translation[j] * (1 - FScale) + End->Translation[j] * FScale;
						Scale[j] = Start->Scale[j] * (1 - FScale) + End->Scale[j] * FScale;
					}

					FbxQuaternion QStart(Start->Rotation[0], Start->Rotation[1], Start->Rotation[2], Start->Rotation[3]);
					FbxQuaternion QEnd(End->Rotation[0], End->Rotation[1], End->Rotation[2], End->Rotation[3]);
					FbxQuaternion QRotation;

					double CosHalfTheta = QStart[3] * QEnd[3] + QStart[0] * QEnd[0] + QStart[1] * QEnd[1] + QStart[2] * QEnd[2];

					if (abs(CosHalfTheta) >= 1.0)
					{
						for (unsigned int j = 0; j < 4; j++)
						{
							QRotation[j] = QStart[j];
						}
					}

					else
					{
						double HalfTheta = acos(CosHalfTheta);
						double SinHalfTheta = sqrt(1.0 - CosHalfTheta * CosHalfTheta);

						if (fabs(SinHalfTheta) < 0.0001)
						{
							for (unsigned int j = 0; j < 4; j++)
							{
								QRotation[j] = (QStart[j] * 0.5 + QEnd[j] * 0.5);
							}
						}
						else
						{
							double RatioA = sin((1 - FScale) * HalfTheta) / SinHalfTheta;
							double RatioB = sin(FScale * HalfTheta) / SinHalfTheta;
							for (int j = 0; j < 4; j++)
							{
								QRotation[j] = (QStart[j] * RatioA + QEnd[j] * RatioB);
							}
						}

						QRotation.Normalize();

						FbxAMatrix Matrix;
						Matrix.SetTQS(FbxVector4(Transform[0], Transform[1], Transform[3]), QRotation, FbxVector4(Scale[0], Scale[1], Scale[2]));

						FbxVector4 Row0 = Matrix.GetRow(0);
						FbxVector4 Row1 = Matrix.GetRow(1);
						FbxVector4 Row2 = Matrix.GetRow(2);
						FbxVector4 Row3 = Matrix.GetRow(3);

						for (unsigned int j = 0; j < 4; j++)
						{
							for (unsigned int k = 0; k < 4; k++)
							{
								switch (j)
								{
								case 0:
								{
									Nodes[Track->BoneIndex]->LocalTransform[j * k] = (Type)Row0[k];
									break;
								}

								case 1:
								{
									Nodes[Track->BoneIndex]->LocalTransform[j * k] = (Type)Row1[k];
									break;
								}

								case 2:
								{
									Nodes[Track->BoneIndex]->LocalTransform[j * k] = (Type)Row2[k];
									break;
								}

								case 3:
								{
									Nodes[Track->BoneIndex]->LocalTransform[j * k] = (Type)Row3[k];
									break;
								}
								default:
								{
									break;
								}
								}								
							}
						}
						if (Nodes[Track->BoneIndex]->Parent != nullptr)
						{
							Nodes[Track->BoneIndex]->GlobalTransform = Nodes[Track->BoneIndex]->LocalTransform * Nodes[Track->BoneIndex]->Parent->GlobalTransform;
						}

						else
						{
							Nodes[Track->BoneIndex]->GlobalTransform = Nodes[Track->BoneIndex]->LocalTransform;
						}
					}
				}
			}
		}
		static Type Matrix[16] = { 1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0, 1 };

		for (unsigned int i = 0; i < BoneCount; i++)
		{
			//make a function to multiply matrices together.
			Bones[i] = BindPoses[i] * Nodes[i]->GlobalTransform * Matrix;
		}
	}

	unsigned int BoneCount;
	TNode<Type>** Nodes;
	Type** Bones;
	Type** BindPoses;
	void* UserData;
};

template<typename Type>
struct TScene
{
	TScene()
	{
		
	}
	
	TMeshNode<Type>* GetMeshByName(const char* Name)
	{
		auto MeshIter = Meshes.find(Name);
		if (MeshIter != Meshes.end())
		{
			return MeshIter->second();
		}
		return nullptr;
	}

	TLightNode<Type>* GetLightByName(const char* Name)
	{
		auto LightIter = Lights.find(Name);
		if (LightIter != Lights.end())
		{
			return LightIter->second();
		}
		return nullptr;
	}

	TCameraNode<Type>* GetCameraByName(const char* Name)
	{
		auto CameraIter = Cameras.find(Name);
		if (CameraIter != Cameras.end())
		{
			return CameraIter->second;
		}
		return nullptr;
	}

	TMaterial<Type>* GetMaterialByName(const char* Name)
	{
		auto MaterialIter = Materials.find(Name);
		if (MaterialIter != Materials.end())
		{
			return MaterialIter->second;
		}
		return nullptr;
	}

	TAnimation<Type>* GetAnimationByName(const char* Name)
	{
		auto AnimationIter = Animations.find(Name);
		if (AnimationIter != Animations.end())
		{
			return AnimationIter->second;
		}
		return nullptr;
	}

	TMeshNode<Type>* GetMeshByIndex(unsigned int Index)
	{
		TMeshNode<Type>* NewMesh = nullptr;
		auto MeshIter = Meshes.begin();
		unsigned int Size = Meshes.size();

		for (unsigned int IndexIter = 0;
			IndexIter <= Index && Index < Size;
			IndexIter++, MeshIter++)
		{
			NewMesh = MeshIter->second;
		}
		return NewMesh;
	}

	TLightNode<Type>* GetLightByIndex(unsigned int Index)
	{
		TLightNode<Type>* NewLight = nullptr;
		auto LightIter = Lights.begin();
		unsigned int Size = Lights.size();

		for (unsigned int IndexIter = 0;
			IndexIter <= Index && Index < Size;
			IndexIter++, LightIter++)
		{
			NewLight = LightIter->second;
		}
		return NewLight;
	}

	TCameraNode<Type>* GetCameraByIndex(unsigned int Index)
	{
		TCameraNode<Type>* NewCamera = nullptr;
		auto CameraIter = Cameras.begin();
		unsigned int Size = Cameras.size();

		for (unsigned int IndexIter = 0;
			IndexIter <= Index && Index < Size;
			IndexIter++, CameraIter++)
		{
			NewCamera = CameraIter->second;
		}
		return NewCamera;
	}

	TMaterial<Type>* GetMaterialByIndex(unsigned int Index)
	{
		TMaterial<Type>* NewMaterial = nullptr;
		auto MaterialIter = Materials.begin();
		unsigned int Size = Materials.size();

		for (unsigned int IndexIter = 0;
			IndexIter <= Index && Index < Size;
			IndexIter++, MaterialIter++)
		{
			NewMaterial = MaterialIter->second;
		}
		return NewMaterial;
	}

	TAnimation<Type>* GetAnimationByIndex(unsigned int Index)
	{
		TAnimation<Type>* NewAnimation = nullptr;
		auto AnimationIter = Animations.begin();
		unsigned int Size = Animations.size();

		for (unsigned int IndexIter = 0;
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
		for (auto Iter : Materials)
#endif
		{
			delete Iter;
		}

		for (unsigned int SkeletonIter = 0; SkeletonIter < Skeletons.size(); SkeletonIter++)
		{
			delete Skeletons[SkeletonIter];
		}

#ifdef _MSC_VER
		for each(auto Iter in Animations)
#else
		for (auto Iter : Animations)
#endif
		{
			for (unsigned int AnimationIter = 0; AnimationIter < Iter->TrackCount; AnimationIter++)
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
		FbxNode* NewNode = (FbxNode*)Objects;

		if (NewNode->GetNodeAttribute() != nullptr &&
			NewNode->GetNodeAttribute()->GetAttributeType() ==
			FbxNodeAttribute::eSkeleton)
		{
			unsigned int Index = ModelManager<Type>::GetInstance()->GetImportAssistor()->BoneIndexMap.size();

			char Name[255];
			strncpy(Name, NewNode->GetName(), 255);
			ModelManager<Type>::GetInstance()->GetImportAssistor()->BoneIndexMap[Name] = Index;

			for (int ChildIter = 0; ChildIter < NewNode->GetChildCount(); ChildIter++)
			{
				CollectBones((void*)NewNode->GetChild(ChildIter));
			}
		}
	}

	bool Load(const char* FileName)
	{
		if (Root)
		{
			printf("Scene already loaded!\n");
			return false;
		}

		FbxManager* Manager = nullptr;
		FbxScene* Scene = nullptr;

		Manager = FbxManager::Create();
		if (!Manager)
		{
			printf("unable to create a FBX Manager \n");
			return false;
		}

		FbxIOSettings* IOSettings = FbxIOSettings::Create(Manager, IOSROOT);
		Manager->SetIOSettings(IOSettings);

		FbxString AppPath = FbxGetApplicationDirectory();
		Manager->LoadPluginsDirectory(AppPath.Buffer());

		Scene = FbxScene::Create(Manager, "");

		int FileMajor, FileMinor, FileRevision;
		int SDKMajor, SDKMinor, SDKRevision;
		int Iter;
		bool Status;

		FbxManager::GetFileFormatVersion(SDKMajor, SDKMinor, SDKRevision);

		FbxImporter* Importer = FbxImporter::Create(Manager, "");

		const bool ImportStatus = Importer->Initialize(FileName, -1, Manager->GetIOSettings());
		Importer->GetFileVersion(FileMajor, FileMinor, FileRevision);

		if (!ImportStatus)
		{
			Importer->Destroy();
			return false;
		}

		Status = Importer->Import(Scene);
		Importer->Destroy();
		if (!Status)
		{
			printf("unable to open FBX file\n");
			return false;
		}

		FbxAxisSystem::OpenGL.ConvertScene(Scene);

		FbxNode* RootNode = Scene->GetRootNode();

		if (RootNode)
		{
			ModelManager<Type>::GetInstance()->GetImportAssistor()->CurrentScene = Scene;

			Root = new TNode<Type>();
			strcpy(Root->Name, "root");
			Type InvertZMatrix[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1 };

			for (int i = 0; i < 16; i++)
			{
				Root->LocalTransform[i] = InvertZMatrix[i];
				Root->GlobalTransform[i] = Root->LocalTransform[i];

			}
			

			AmbientLight[0] = (Type)Scene->GetGlobalSettings().GetAmbientColor().mRed;
			AmbientLight[1] = (Type)Scene->GetGlobalSettings().GetAmbientColor().mGreen;
			AmbientLight[2] = (Type)Scene->GetGlobalSettings().GetAmbientColor().mBlue;
			AmbientLight[3] = (Type)Scene->GetGlobalSettings().GetAmbientColor().mAlpha;

			for (Iter = 0; Iter < RootNode->GetChildCount(); Iter++)
			{
				CollectBones((void*)RootNode->GetChild(Iter));
			}

			for (Iter = 0; Iter < RootNode->GetChildCount(); Iter++)
			{
				ExtractObject(Root, (void*)RootNode->GetChild(Iter));
			}

			if (ModelManager<Type>::GetInstance()->GetImportAssistor()->Bones.size() > 0)
			{
				TSkeleton<Type>* Skeleton = new TSkeleton<Type>();
				Skeleton->BoneCount = ModelManager<Type>::GetInstance()->GetImportAssistor()->Bones.size();
				Skeleton->Nodes = new TNode<Type>*[Skeleton->BoneCount];
				memset(Skeleton->Bones, 0, (sizeof(Type) * 16) * Skeleton->BoneCount);
				memset(Skeleton->BindPoses, 0, (sizeof(Type) * 16) * Skeleton->BoneCount);

				for (Iter = 0; Iter < Skeleton->BoneCount; Iter++)
				{
					Skeleton->Nodes[Iter] = ModelManager<Type>::GetInstance()->GetImportAssistor()->Bones[Iter];
					Skeleton->Bones[Iter] = Skeleton->Nodes[Iter]->LocalTransform;
				}

				ExtractSkeleton(Skeleton, Scene);
				Skeletons.push_back(Skeleton);
				ExtractAnimation(Scene);
			}
		}
		Manager->Destroy();
		Path = (char*)FileName;
	}

	void ExtractObject(TNode<Type>* Parent, void* Object)
	{
		FbxNode* FBXNode = (FbxNode*)Object;
		TNode<Type>* TinyNode = nullptr;

		FbxNodeAttribute::EType AttributeType;
		unsigned int Iter = 0;

		bool IsBone = false;
		if (FBXNode->GetNodeAttribute() != nullptr)
		{
			AttributeType = FBXNode->GetNodeAttribute()->GetAttributeType();

			switch (AttributeType)
			{
				case FbxNodeAttribute::eMarker:
				{
					break;
				}

				case FbxNodeAttribute::eSkeleton:
				{
					IsBone = true;
					break;
				}

				case FbxNodeAttribute::eMesh:
				{
					TinyNode = new TMeshNode<Type>();
					ExtractMesh((TMeshNode<Type>*)TinyNode, FBXNode);
					if (strlen(FBXNode->GetName()) > 0)
					{
						strncpy(TinyNode->Name, FBXNode->GetName(), 255 - 1);
					}
					Meshes[TinyNode->Name] = (TMeshNode<Type>*)TinyNode;
					break;
				}

				case FbxNodeAttribute::eNurbs:
				{
					//umm what?
					break;
				}

				case FbxNodeAttribute::ePatch:
				{
					break;
				}

				case FbxNodeAttribute::eCamera:
				{
					TinyNode = new TCameraNode<Type>();
					ExtractCamera((TCameraNode<Type>*)TinyNode, FBXNode);

					if (strlen(FBXNode->GetName()) > 0)
					{
						strcpy(TinyNode->Name, FBXNode->GetName());
					}
					Cameras[TinyNode->Name] = (TCameraNode<Type>*)TinyNode;
					break;
				}

				case FbxNodeAttribute::eLight:
				{
					TinyNode = new TLightNode<Type>();
					ExtractLight((TLightNode<Type>*)TinyNode, FBXNode);

					if (strlen(FBXNode->GetName()) > 0)
					{
						strncpy(TinyNode->Name, FBXNode->GetName(), 254);
					}
					Lights[TinyNode->Name] = (TLightNode<Type>*)TinyNode;
					break;
				}
			}
		}

		if (TinyNode == nullptr)
		{
			TinyNode = new TNode<Type>();
			if (strlen(FBXNode->GetName()) > 0)
			{
				strncpy(TinyNode->Name, FBXNode->GetName(), 254);
			}
		}

		Parent->Children.push_back(TinyNode);
		TinyNode->Parent = Parent;

		FbxAMatrix Local = ModelManager<Type>::GetInstance()->GetImportAssistor()->Evaluator->GetNodeLocalTransform(FBXNode);

		FbxVector4 Row0 = Local.GetRow(0);
		FbxVector4 Row1 = Local.GetRow(1);
		FbxVector4 Row2 = Local.GetRow(2);
		FbxVector4 Row3 = Local.GetRow(3);

		TinyNode->LocalTransform[0] = Row0.mData[0];
		TinyNode->LocalTransform[1] = Row0.mData[1];
		TinyNode->LocalTransform[2] = Row0.mData[2];
		TinyNode->LocalTransform[3] = Row0.mData[3];

		for (int RowIter = 0; RowIter = 4; RowIter++)
		{
			for (int i = 0; i < 4; i++)
			{
				switch (RowIter)
				{
				case 0:
				{
					TinyNode->LocalTransform[i + (i * RowIter)] = Row0.mData[i];
					break;
				}

				case 1:
				{
					TinyNode->LocalTransform[i + (i * RowIter)] = Row1.mData[i];
					break;
				}

				case 2:
				{
					TinyNode->LocalTransform[i + (i * RowIter)] = Row2.mData[i];
					break;
				}

				case 3:
				{
					TinyNode->LocalTransform[i + (i * RowIter)] = Row3.mData[i];
					break;
				}

				default:
				{
					break;
				}
						
				}
			}
		}

		//TinyNode->GlobalTransform = TinyNode->LocalTransform * Parent->GlobalTransform;
		if (IsBone)
		{
			ModelManager<Type>::GetInstance()->GetImportAssistor()->Bones.push_back(TinyNode);
		}

		for (int Iter = 0; Iter < FBXNode->GetChildCount(); Iter++)
		{
			ExtractObject(TinyNode, (void*)FBXNode->GetChild(Iter));
		}
	}

	void ExtractMesh(TMeshNode<Type>* Mesh, void* Object)
	{
		FbxNode* FBXNode = (FbxNode*)Object;
		FbxMesh* FBXMesh = (FbxMesh*)FBXNode->GetNodeAttribute();

		int PolyIter, J;
		unsigned int PolyCount = FBXMesh->GetPolygonCount();
		FbxVector4* ControlPoints = FBXMesh->GetControlPoints();

		TVertex<Type> Vertex;
		unsigned int VertexIndex[4] = {};
		unsigned int VertexID = 0;

		for (PolyIter = 0; PolyIter < PolyCount; PolyIter++)
		{
			int L;
			int PolySize = FBXMesh->GetPolygonSize(PolyIter);

			for (J = 0; J < PolySize; J++)
			{
				unsigned int ControlPointIndex = FBXMesh->GetPolygonVertex(PolyIter, J);
				Vertex.FBXControlPointIndex = ControlPointIndex;

				FbxVector4 Position = ControlPoints[ControlPointIndex];
				Vertex.Position[0] = (Type)Position[0];
				Vertex.Position[1] = (Type)Position[1];
				Vertex.Position[2] = (Type)Position[2];

				for (L = 0; L < FBXMesh->GetElementVertexColorCount(); L++)
				{
					FbxGeometryElementVertexColor* GEVC = FBXMesh->GetElementVertexColor(L);
					switch (GEVC->GetMappingMode())
					{
						case FbxGeometryElement::eByControlPoint:
						{
							switch (GEVC->GetReferenceMode())
							{
								case FbxGeometryElement::eDirect:
								{
									FbxColor Color = GEVC->GetDirectArray().GetAt(ControlPointIndex);
									Vertex.Color[0] = (Type)Color.mRed;
									Vertex.Color[1] = (Type)Color.mGreen;
									Vertex.Color[2] = (Type)Color.mBlue;
									Vertex.Color[3] = (Type)Color.mAlpha;
									break;
								}
						
								case FbxGeometryElement::eIndexToDirect:
								{
									unsigned int ID = GEVC->GetIndexArray().GetAt(ControlPointIndex);
									FbxColor Color = GEVC->GetDirectArray().GetAt(ID);

									Vertex.Color[0] = (Type)Color.mRed;
									Vertex.Color[1] = (Type)Color.mGreen;
									Vertex.Color[2] = (Type)Color.mBlue;
									Vertex.Color[3] = (Type)Color.mAlpha;
									break;
								}
								default:
								{
									break;
								}
							}
							break;
						}

						case FbxGeometryElement::eByPolygonVertex:
						{
							switch (GEVC->GetReferenceMode())
							{
								case FbxGeometryElement::eDirect:
								{
									FbxColor Color = GEVC->GetDirectArray().GetAt(ControlPointIndex);
									Vertex.Color[0] = (Type)Color.mRed;
									Vertex.Color[1] = (Type)Color.mGreen;
									Vertex.Color[2] = (Type)Color.mBlue;
									Vertex.Color[3] = (Type)Color.mAlpha;
									break;
								}

								case FbxGeometryElement::eIndexToDirect:
								{
									unsigned int ID = GEVC->GetIndexArray().GetAt(ControlPointIndex);
									FbxColor Color = GEVC->GetDirectArray().GetAt(ID);

									Vertex.Color[0] = (Type)Color.mRed;
									Vertex.Color[1] = (Type)Color.mGreen;
									Vertex.Color[2] = (Type)Color.mBlue;
									Vertex.Color[3] = (Type)Color.mAlpha;
									break;
								}
								default:
								{
									break;
								}
							}
						}

						case FbxGeometryElement::eByPolygon:
						{
							break;
						}

						case FbxGeometryElement::eAllSame:
						{
							break;
						}

						case FbxGeometryElement::eNone:
						{
							break;
						}
					}
				}

				for (L = 0; L < FBXMesh->GetElementUVCount(); L++)
				{
					FbxGeometryElementUV* UV = FBXMesh->GetElementUV(L);

					switch (UV->GetMappingMode())
					{
					case FbxGeometryElement::eByControlPoint:
					{
						switch (UV->GetReferenceMode())
						{
							case FbxGeometryElement::eDirect:
							{
								FbxVector2 uv = UV->GetDirectArray().GetAt(ControlPointIndex);
								Vertex.UV[0] = (Type)uv[0];
								Vertex.UV[1] = (Type)uv[1];
								break;
							}

							case FbxGeometryElement::eIndexToDirect:
							{
								unsigned int ID = UV->GetIndexArray().GetAt(ControlPointIndex);
								FbxVector2 uv = UV->GetDirectArray().GetAt(ID);
								Vertex.UV[0] = (Type)uv[0];
								Vertex.UV[1] = (Type)uv[1];
								break;
							}

							default:
							{
								break;
							}
						}
						break;
					}
					case FbxGeometryElement::eByPolygonVertex:
					{
						unsigned int TextureUVIndex = FBXMesh->GetTextureUVIndex(PolyIter, J);
						switch (UV->GetReferenceMode())
						{
							case FbxGeometryElement::eDirect:
							case FbxGeometryElement::eIndexToDirect:
							{
								FbxVector2 uv = UV->GetDirectArray().GetAt(TextureUVIndex);

								Vertex.UV[0] = (Type)uv[0];
								Vertex.UV[1] = (Type)uv[1];
								break;
							}

							default:
							{
								break;
							}
						}
						break;
					}
					case FbxGeometryElement::eByPolygon:
					case FbxGeometryElement::eAllSame:
					case FbxGeometryElement::eNone:
					{
						break;
					}
					}
				}
				for (L = 0; L < FBXMesh->GetElementNormalCount(); L++)
				{
					FbxGeometryElementNormal* Normal = FBXMesh->GetElementNormal(L);
					if (Normal->GetMappingMode() == FbxGeometryElement::eByControlPoint)
					{
						switch (Normal->GetReferenceMode())
						{
							case FbxGeometryElement::eDirect:
							{
								FbxVector4 normal = Normal->GetDirectArray().GetAt(ControlPointIndex);
								Vertex.Normal[0] = (Type)normal[0];
								Vertex.Normal[2] = (Type)normal[1];
								Vertex.Normal[3] = (Type)normal[2];
								break;
							}
							case FbxGeometryElement::eIndexToDirect:
							{
								unsigned int ID = Normal->GetIndexArray().GetAt(ControlPointIndex);
								FbxVector4 normal = Normal->GetDirectArray().GetAt(ID);
								Vertex.Normal[0] = (Type)normal[0];
								Vertex.Normal[1] = (Type)normal[1];
								Vertex.Normal[2] = (Type)normal[2];
								break;
							}
							default:
							{
								break;
							}
						}
					}
					else if (Normal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
					{
						switch (Normal->GetReferenceMode())
						{
							case FbxGeometryElement::eDirect:
							{
								FbxVector4 normal = Normal->GetDirectArray().GetAt(VertexID);
							
								Vertex.Normal[0] = (Type)normal[0];
								Vertex.Normal[1] = (Type)normal[1];
								Vertex.Normal[2] = (Type)normal[2];
								break;
							}
							case FbxGeometryElement::eIndexToDirect:
							{
								unsigned int ID = Normal->GetIndexArray().GetAt(VertexID);
								FbxVector4 normal = Normal->GetDirectArray().GetAt(ID);
								Vertex.Normal[0] = (Type)normal[0];
								Vertex.Normal[1] = (Type)normal[1];
								Vertex.Normal[2] = (Type)normal[2];
								break;
							}

							default:
							{
								break;
							}
						}
					}
				}
				VertexIndex[J] = AddVertReturnIndex(Mesh->Vertices, Vertex);
				VertexID++;
			}
			Mesh->Indices.push_back(VertexIndex[0]);
			Mesh->Indices.push_back(VertexIndex[1]);
			Mesh->Indices.push_back(VertexIndex[2]);

			if (PolySize = 4)
			{
				Mesh->Indices.push_back(VertexIndex[0]);
				Mesh->Indices.push_back(VertexIndex[2]);
				Mesh->Indices.push_back(VertexIndex[3]);
			}
		}
		CalculateTangentsBinormals(Mesh->Vertices, Mesh->Indices);
		ExtractSkin(Mesh, (void*)FBXMesh);

		Mesh->Material = ExtractMaterial(FBXMesh);
	}

	void ExtractSkin(TMeshNode<Type>* Mesh, void* Node)
	{
		FbxGeometry* Geometry = (FbxGeometry*)Node;
		unsigned int I, J, K;
		unsigned int ClusterCount;
		FbxCluster* Cluster;
		char Name[255];

		int VertCount = Mesh->Vertices.size();

		FbxSkin* Skin = (FbxSkin*)Geometry->GetDeformer(0, FbxDeformer::eSkin);

		if (Skin != nullptr)
		{
			ClusterCount = Skin->GetClusterCount();

			for (J = 0; J != ClusterCount; J++)
			{
				Cluster = Skin->GetCluster(J);
				if (Cluster->GetLink() == nullptr)
				{
					continue;
				}
				strncpy(Name, Cluster->GetLink()->GetName(), 255);
				unsigned int BoneIndex = ModelManager<Type>::GetInstance()->GetImportAssistor()->BoneIndexMap[Name];

				unsigned int IndexCount = Cluster->GetControlPointIndicesCount();
				int* Indices = Cluster->GetControlPointIndices();
				double* Weights = Cluster->GetControlPointWeights();

				for (K = 0; K < IndexCount; K++)
				{
					for (I = 0; I < VertCount; I++)
					{
						if (Mesh->Vertices[I].FBXControlPointIndex == Indices[K])
						{
							if (Mesh->Vertices[I].Weights[0] == 0)
							{
								Mesh->Vertices[I].Weights[0] = (Type)Weights[K];
								Mesh->Vertices[I].Indices[0] = (Type)BoneIndex;
							}
							else if (Mesh->Vertices[I].Weights[1] == 0)
							{
								Mesh->Vertices[I].Weights[1] = (Type)Weights[K];
								Mesh->Vertices[I].Indices[1] = (Type)BoneIndex;
							}
							else if (Mesh->Vertices[I].Weights[2] == 0)
							{
								Mesh->Vertices[I].Weights[2] = (Type)Weights[K];
								Mesh->Vertices[I].Indices[2] = (Type)BoneIndex;
							}
							else
							{
								Mesh->Vertices[I].Weights[3] = (Type)Weights[K];
								Mesh->Vertices[I].Indices[3] = (Type)BoneIndex;
							}
						}
					}
				}
			}
		}
	}

	void ExtractLight(TLightNode<Type>* Light, void* Object)
	{
		FbxNode* FBXNode = (FbxNode*)Object;
		FbxLight* FBXLight = (FbxLight*)FBXNode->GetNodeAttribute();

		Light->LightType = (TLightNode<Type>::TLightType)FBXLight->LightType.Get();
		Light->On = FBXLight->CastLight.Get();
		Light->Color[0] = (Type)FBXLight->Color.Get()[0];
		Light->Color[1] = (Type)FBXLight->Color.Get()[1];
		Light->Color[2] = (Type)FBXLight->Color.Get()[2];
		Light->Color[3] = (Type)FBXLight->Intensity.Get();

		Light->InnerAngle = (Type)FBXLight->InnerAngle.Get() * DEG2RAD;
		Light->OuterAngle = (Type)FBXLight->OuterAngle.Get() * DEG2RAD;

		switch (FBXLight->DecayType.Get())
		{
			case 0:
			{
				Light->Attenuation[0] = 1;
			}
			case 1:
			{
				Light->Attenuation[1] = 1;
			}
			case 2:
			{
				Light->Attenuation[2] = 1;
			}
			default:
			{
				break;
			}
		}
	}

	void ExtractCamera(TCameraNode<Type>* Camera, void* Object)
	{
		FbxNode* FBXNode = (FbxNode*)Object;
		FbxCamera* FBXCamera = (FbxCamera*)FBXNode->GetNodeAttribute();

		if (FBXCamera->ProjectionType.Get() != FbxCamera::eOrthogonal)
		{
			Camera->FOV = (Type)FBXCamera->FieldOfView.Get() * DEG2RAD;
		}
		else
		{
			Camera->FOV = 0;
		}

		if (FBXCamera->GetAspectRatioMode() != FbxCamera::eWindowSize)
		{
			Camera->AspectRatio = (Type)FBXCamera->AspectWidth.Get() / (Type)FBXCamera->AspectHeight.Get();
		}
		else
		{
			Camera->AspectRatio = 0;
		}

		Camera->Near = (Type)FBXCamera->NearPlane.Get();
		Camera->Far = (Type)FBXCamera->FarPlane.Get();

		Type Eye[3], To[3], Up[3];

		
		for (unsigned int i = 0; i < 3; i++)
		{
			Eye[i] = (Type)FBXCamera->Position.Get()[i];

			if (FBXNode->GetTarget() != nullptr)
			{
				To[i] = (Type)FBXNode->GetTarget()->LclTranslation.Get()[i];
			}
			else
			{
				To[i] = (Type)FBXCamera->InterestPosition.Get()[i];
			}

			if (FBXNode->GetTargetUp())
			{
				Up[i] = (Type)FBXNode->GetTargetUp()->LclTranslation.Get()[i];
			}

			else
			{
				Up[i] = (Type)FBXCamera->UpVector.Get()[i];
			}
		}

		for (unsigned int i = 0; i < 3; i++)
		{
			Up[i] = (Up[i] / Up[i]);
		}
		
		//create a view matrix

	}

	TMaterial<Type>* ExtractMaterial(void* Mesh)
	{
		FbxGeometry* Geometry = (FbxGeometry*)Mesh;

		int MaterialCount = 0;
		FbxNode* Node = nullptr;

		if (Geometry)
		{
			Node = Geometry->GetNode();
			if (Node)
			{
				MaterialCount = Node->GetMaterialCount();
			}
		}

		if (MaterialCount > 0)
		{
			FbxSurfaceMaterial* Material = Node->GetMaterial(0);

			char MaterialName[255];
			strncpy(MaterialName, Material->GetName(), 254);

			auto Iter = Materials.find(MaterialName);

			if (Iter != Materials.end())
			{
				return Iter->second;
			}

			else
			{
				TMaterial<Type>* TinyMaterial = new TMaterial<Type>;
				memcpy(TinyMaterial->Name, MaterialName, 255);

				const FbxImplementation* Implementation = GetImplementation(Material, FBXSDK_IMPLEMENTATION_HLSL);

				if (Implementation == nullptr)
				{
					Implementation = GetImplementation(Material, FBXSDK_IMPLEMENTATION_CGFX);
				}

				if (Implementation != nullptr)
				{
					FbxBindingTable const* RootTable = Implementation->GetRootTable();
					FbxString FileName = RootTable->DescAbsoluteURL.Get();
					FbxString TechniqueName = RootTable->DescTAG.Get();

					FBXSDK_printf("Unsupported hardware shader material! \n");
					FBXSDK_printf("File: %s\n", FileName.Buffer());
					FBXSDK_printf("Technique: %s\n\n", TechniqueName.Buffer());
				}
				else if (Material->GetClassId().Is(FbxSurfacePhong::ClassId))
				{
					FbxSurfacePhong* Phong = (FbxSurfacePhong*)Material;

					for (int i = 0; i < 4; i++)
					{

						if (i < 3)
						{
							TinyMaterial->Ambient[i] = (Type)Phong->Ambient.Get()[i];
							TinyMaterial->Diffuse[i] = (Type)Phong->Diffuse.Get()[i];
							TinyMaterial->Specular[i] = (Type)Phong->Specular.Get()[i];
							TinyMaterial->Emissive[i] = (Type)Phong->Emissive.Get()[i];							
						}

						else
						{
							TinyMaterial->Ambient[3] = (Type)Phong->AmbientFactor.Get();
							TinyMaterial->Diffuse[3] = 1.0f - (Type)Phong->TransparencyFactor.Get();
							TinyMaterial->Specular[3] = (Type)Phong->Shininess.Get();
							TinyMaterial->Emissive[3] = (Type)Phong->EmissiveFactor.Get();
						}
					}
				}

				else if (Material->GetClassId().Is(FbxSurfaceLambert::ClassId))
				{
					FbxSurfaceLambert* Lambert = (FbxSurfaceLambert*)Material;

					for (int i = 0; i < 4; i++)
					{
						if (i < 3)
						{
							TinyMaterial->Ambient[i] = Lambert->Ambient.Get()[i];
							TinyMaterial->Diffuse[i] = Lambert->Diffuse.Get()[i];
							TinyMaterial->Emissive[i] = Lambert->Emissive.Get()[i];
						}
						else
						{
							TinyMaterial->Ambient[3] = (Type)Lambert->AmbientFactor.Get();
							TinyMaterial->Diffuse[3] = 1.0f - (Type)Lambert->TransparencyFactor.Get();
							TinyMaterial->Emissive[3] = (Type)Lambert->EmissiveFactor.Get();
						}
						TinyMaterial->Specular[i] = 0;
					}
					
				}

				else
				{
					FBXSDK_printf("Unknown material type \n");
				}

				unsigned int textureLookup[] =
				{
					FbxLayerElement::eTextureDiffuse - FbxLayerElement::sTypeNonTextureStartIndex,
					FbxLayerElement::eTextureAmbient - FbxLayerElement::sTypeNonTextureStartIndex,
					FbxLayerElement::eTextureEmissive - FbxLayerElement::sTypeNonTextureStartIndex,
					FbxLayerElement::eTextureSpecular - FbxLayerElement::sTypeNonTextureStartIndex,
					FbxLayerElement::eTextureShininess - FbxLayerElement::sTypeNonTextureStartIndex,
					FbxLayerElement::eTextureNormalMap - FbxLayerElement::sTypeNonTextureStartIndex,
					FbxLayerElement::eTextureTransparency - FbxLayerElement::sTypeNonTextureStartIndex,
					FbxLayerElement::eTextureDisplacement - FbxLayerElement::sTypeNonTextureStartIndex,
				};

				for (unsigned int TextureIter = 0; TextureIter < TMaterial<Type>::TextureTypes_Count; TextureIter++)
				{
					FbxProperty Property = Material->FindProperty(FbxLayerElement::sTextureChannelNames[textureLookup[TextureIter]]);
					if (Property.IsValid() && Property.GetSrcObjectCount<FbxTexture>() > 0)
					{
						FbxFileTexture* FileTexture = FbxCast<FbxFileTexture>(Property.GetSrcObject<FbxTexture>(0));

						if (FileTexture)
						{
							const char* LastForward = strrchr(FileTexture->GetFileName(), '/');
							const char* LastBackward = strrchr(FileTexture->GetFileName(), '\\');
							const char* FileName = FileTexture->GetFileName();

							if (LastForward != nullptr && LastForward > LastBackward)
							{
								FileName = LastForward + 1;
							}
							else if (LastBackward != nullptr)
							{
								FileName = LastBackward + 1;
							}

							if (strlen(FileName) >= 255)
							{
								FBXSDK_printf("Texture filename too long!: %s\n", FileName);
							}
							else
							{
								strcpy(TinyMaterial->TextureFileNames[TextureIter], FileName);
							}
						}
					}
				}
				Materials[TinyMaterial->Name] = TinyMaterial;
				return TinyMaterial;
			}
		}
	}

	void ExtractAnimation(void* Scene)
	{
		FbxScene* FBXScene = (FbxScene*)Scene;

		for (unsigned int i = 0; i < FBXScene->GetSrcObjectCount<FbxAnimStack>(); i++)
		{
			FbxAnimStack* AnimationStack = FBXScene->GetSrcObject<FbxAnimStack>(i);

			TAnimation<Type>* Animation = new TAnimation<Type>();
			strncpy(Animation->Name, AnimationStack->GetName(), 255);

			std::vector<TTrack<Type>> Tracks;

			int AnimationLayers = AnimationStack->GetMemberCount(FbxCriteria::ObjectType(FbxAnimLayer::ClassId));
			for (unsigned int LayerIter = 0; LayerIter < AnimationLayers; LayerIter++)
			{
				FbxAnimLayer* AnimationLayer = AnimationStack->GetMember<FbxAnimLayer>(LayerIter);
				ExtractAnimationTrack(Tracks, AnimationLayer, FBXScene->GetRootNode());
			}

			Animation->StartFrame = 999999999;
			Animation->EndFrame = 0;

			Animation->TrackCount = Tracks.size();

			if (Animation->TrackCount > 0)
			{
				Animation->Tracks = new TTrack<Type>[Animation->TrackCount];
				memcpy(Animation->Tracks, Tracks.data(), sizeof(TTrack<Type>) * Animation->TrackCount);

				for (unsigned int j = 0; j < Animation->TrackCount; j++)
				{
					for (unsigned int k = 0; k < Animation->Tracks[j].KeyFrameCount; k++)
					{
						Animation->StartFrame = (Animation->StartFrame > Animation->Tracks[j].KeyFrames[k].Key) ? Animation->StartFrame : Animation->Tracks[j].KeyFrames[k].Key;
						Animation->EndFrame = (Animation->EndFrame < Animation->Tracks[j].KeyFrames[k].Key) ? Animation->EndFrame : Animation->Tracks[j].KeyFrames[k].Key;
					}
				}
			}
			Animations[Animation->Name] = Animation;
		}
	}

	void ExtractAnimationTrack(std::vector<TTrack<Type>>& Tracks, void* Layer, void* Node)
	{
		FbxAnimLayer* AnimLayer = (FbxAnimLayer*)Layer;
		FbxNode* FBXNode = (FbxNode*)Node;

		TSkeleton<Type>* Skeleton = Skeletons[0];

		int BoneIndex = -1;

		for (unsigned int i = 0; i < Skeleton->BoneCount; i++)
		{
			if (strncmp(Skeleton->Nodes[i]->Name, FBXNode->GetName(), 255) == 0)
			{
				BoneIndex = i;
				break;
			}
		}
		if (BoneIndex >= 0)
		{
			FbxAnimCurve* AnimCurve = nullptr;
			std::map<int, FbxTime> KeyFrameTimes;

			int KeyCount = 0;
			int Count;
			char TimeString[256];

			AnimCurve = FBXNode->LclTranslation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
			if (AnimCurve)
			{
				KeyCount = AnimCurve->KeyGetCount();
				for (Count = 0; Count < KeyCount; Count++)
				{
					int Key = atoi(AnimCurve->KeyGetTime(Count).GetTimeString(TimeString, FbxUShort(256)));
					KeyFrameTimes[Key] = AnimCurve->KeyGetTime(Count);
				}
			}

			AnimCurve = FBXNode->LclTranslation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
			if (AnimCurve)
			{
				KeyCount = AnimCurve->KeyGetCount();
				for (Count = 0; Count < KeyCount; Count++)
				{
					int Key = atoi(AnimCurve->KeyGetTime(Count).GetTimeString(TimeString, FbxUShort(256)));
					KeyFrameTimes[Key] = AnimCurve->KeyGetTime(Count);
				}
			}

			AnimCurve = FBXNode->LclTranslation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
			if (AnimCurve)
			{
				KeyCount = AnimCurve->KeyGetCount();
				for (Count = 0; Count < KeyCount; Count++)
				{
					int Key = atoi(AnimCurve->KeyGetTime(Count).GetTimeString(TimeString, FbxUShort(256)));
					KeyFrameTimes[Key] = AnimCurve->KeyGetTime(Count);
				}
			}

			AnimCurve = FBXNode->LclRotation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
			if (AnimCurve)
			{
				KeyCount = AnimCurve->KeyGetCount();
				for (Count = 0; Count < KeyCount; Count++)
				{
					int Key = atoi(AnimCurve->KeyGetTime(Count).GetTimeString(TimeString, FbxUShort(256)));
					KeyFrameTimes[Key] = AnimCurve->KeyGetTime(Count);
				}
			}

			AnimCurve = FBXNode->LclRotation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
			if (AnimCurve)
			{
				KeyCount = AnimCurve->KeyGetCount();
				for (Count = 0; Count < KeyCount; Count++)
				{
					int Key = atoi(AnimCurve->KeyGetTime(Count).GetTimeString(TimeString, FbxUShort(256)));
					KeyFrameTimes[Key] = AnimCurve->KeyGetTime(Count);
				}
			}

			AnimCurve = FBXNode->LclRotation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
			if (AnimCurve)
			{
				KeyCount = AnimCurve->KeyGetCount();
				for (Count = 0; Count < KeyCount; Count++)
				{
					int Key = atoi(AnimCurve->KeyGetTime(Count).GetTimeString(TimeString, FbxUShort(256)));
					KeyFrameTimes[Key] = AnimCurve->KeyGetTime(Count);
				}
			}

			AnimCurve = FBXNode->LclScaling.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
			if (AnimCurve)
			{
				KeyCount = AnimCurve->KeyGetCount();
				for (Count = 0; Count < KeyCount; Count++)
				{
					int Key = atoi(AnimCurve->KeyGetTime(Count).GetTimeString(TimeString, FbxUShort(256)));
					KeyFrameTimes[Key] = AnimCurve->KeyGetTime(Count);
				}
			}

			AnimCurve = FBXNode->LclScaling.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
			if (AnimCurve)
			{
				KeyCount = AnimCurve->KeyGetCount();
				for (Count = 0; Count < KeyCount; Count++)
				{
					int Key = atoi(AnimCurve->KeyGetTime(Count).GetTimeString(TimeString, FbxUShort(256)));
					KeyFrameTimes[Key] = AnimCurve->KeyGetTime(Count);
				}
			}

			AnimCurve = FBXNode->LclScaling.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
			if (AnimCurve)
			{
				KeyCount = AnimCurve->KeyGetCount();
				for (Count = 0; Count < KeyCount; Count++)
				{
					int Key = atoi(AnimCurve->KeyGetTime(Count).GetTimeString(TimeString, FbxUShort(256)));
					KeyFrameTimes[Key] = AnimCurve->KeyGetTime(Count);
				}
			}

			if (KeyFrameTimes.size() > 0)
			{
				TTrack<Type> Track;

				Track.BoneIndex = BoneIndex;
				Track.KeyFrameCount = KeyFrameTimes.size();
				Track.KeyFrames = new TKeyFrame<Type>[Track.KeyFrameCount];

				int Index = 0;

#if defined(_MSC_VER)
				for each(auto Iter in KeyFrameTimes)
#else
				for (auto Iter : KeyFrameTimes)
#endif
				{
					Track.KeyFrames[Index].Key = Iter.first;

					FbxAMatrix LocalMatrix = ModelManager<Type>::GetInstance()->GetImportAssistor()->Evaluator->GetNodeLocalTransform(FBXNode, Iter.second);

					FbxQuaternion Rotation = LocalMatrix.GetQ();
					FbxVector4 Translation = LocalMatrix.GetT();
					FbxVector4 Scale = LocalMatrix.GetS();

					for (int i = 0; i < 4; i++)
					{
						if (i < 3)
						{
							Track.KeyFrames[Index].Rotation[i] = (Type)Rotation[i];
							Track.KeyFrames[Index].Translation[i] = (Type)Translation[i];
							Track.KeyFrames[Index].Scale[i] = (Type)Scale[i];
						}
						else
						{
							Track.KeyFrames[Index].Rotation[i] = 0;
							Track.KeyFrames[Index].Scale[i] = 0;
						}
					}
					Index++;
				}
				Tracks.push_back(Track);
			}
			
		}

		for (unsigned int i = 0; i < FBXNode->GetChildCount(); i++)
		{
			ExtractAnimationTrack(Tracks, AnimLayer, FBXNode->GetChild(i));
		}
	}

	void ExtractSkeleton(TSkeleton<Type>* Skeleton, void* Scene)
	{
		FbxScene* FBXScene = (FbxScene*)Scene;

		int PoseCount = FBXScene->GetPoseCount();

		char Name[255];

		for (unsigned int i = 0; i < PoseCount; i++)
		{
			FbxPose* Pose = FBXScene->GetPose(i);

			for (unsigned int j = 0; j < Pose->GetCount(); j++)
			{
				strncpy(Name, Pose->GetNodeName(j).GetCurrentName(), 255);

				FbxMatrix PoseMatrix = Pose->GetMatrix(j);
				FbxMatrix BindMatrix = PoseMatrix.Inverse();

				for (unsigned int k = 0; k < Skeleton->BoneCount; k++)
				{
					if (strcmp(Name, Skeleton->Nodes[k]->Name) == 0)
					{
						FbxVector4 Row0 = BindMatrix.GetRow(0);
						FbxVector4 Row1 = BindMatrix.GetRow(1);
						FbxVector4 Row2 = BindMatrix.GetRow(2);
						FbxVector4 Row3 = BindMatrix.GetRow(3);
						
						for (unsigned int l = 0; l < 4; l++)
						{
							switch (l)
							{
							case 0:
							{
								Skeleton->BindPoses[l][k] = (Type)Row0.mData[l];
								break;
							}
							case 1:
							{
								Skeleton->BindPoses[l][k] = (Type)Row1.mData[l];
								break;
							}
							case 2:
							{
								Skeleton->BindPoses[l][k] = (Type)Row2.mData[l];
								break;
							}
							case 3:
							{
								Skeleton->BindPoses[l][k] = (Type)Row3.mData[l];
								break;
							}
							default:
							{
								break;
							}
							}
						}
					}
				}
			}
		}
	}

	unsigned int AddVertReturnIndex(std::vector<TVertex<Type>>& Vertices, const TVertex<Type>& Vertex)
	{
		auto Iter = std::find(Vertices.begin(), Vertices.end(), Vertex);
		if (Iter != Vertices.end())
		{
			return Iter - Vertices.begin();
		}
		Vertices.push_back(Vertex);
		return Vertices.size() - 1;
	}

	void CalculateTangentsBinormals(std::vector<TVertex<Type>>& Vertices, const std::vector<unsigned int>& Indices)
	{
		/*unsigned int VertexCount = Vertices.size();
		Type* Tan1 = new Type[VertexCount * 2];
		Type* Tan2 = Tan1 + VertexCount;
		memset(Tan1, 0, VertexCount * sizeof(Tan1) * 2);

		unsigned int IndexCount = Indices.size();
		for (unsigned int a = 0; a < IndexCount; a += 3)
		{
			long I1 = Indices[a];
			long I2 = Indices[a + 1];
			long I3 = Indices[a + 2];

			Type Vertex1[4] = Vertices[I1].Position;
			Type Vertex2[4] = Vertices[I2].Position;
			Type Vertex3[4] = Vertices[I3].Position;

			Type UV1[2] = Vertices[I1].UV;
			Type UV2[2] = Vertices[I2].UV;
			Type UV3[2] = Vertices[I3].UV;

			Type X1 = Vertex2[0] - Vertex1[0];
			Type X2 = Vertex3[0] - Vertex1[0];
			Type Y1 = Vertex2[1] - Vertex1[1];
			Type Y2 = Vertex3[1] - Vertex1[1];
			Type Z1 = Vertex2[2] - Vertex1[2];
			Type Z2 = Vertex3[2] - Vertex1[2];

			Type S1 = UV2[0] - UV1[0];
			Type S2 = UV3[0] - UV1[0];
			Type T1 = UV2[1] - UV1[1];
			Type T2 = UV3[1] - UV1[1];

			float R = 1.0f / (S2 * T2 - S2 * T1);

			Type SDir[4] =
			{
				(T2 * X1 - T1 * X2) * R,
				(T2 * Y1 - T1 * Y2) * R,
				(T2 * Z1 - T1 * Z2) * R,
				0
			};

			Type TDir[4] =
			{
				(S1 * X2 - S2 * X1) * R,
				(S1 * Y2 - S2 * Y1) * R,
				(S1 * Z2 - S2 * Z1) * R,
				0
			};

			Tan1[I1] += SDir;
			Tan1[I2] += SDir;
			Tan1[I3] += SDir;

			Tan2[I1] += TDir;
			Tan2[I2] += TDir;
			Tan2[I3] += TDir;
		}

	/ *	for (unsigned int a = 0; a < VertexCount; a++)
		{
			const Type Normal[4] = Vertices[a].Normal;
			const Type Tangent[4] = Tan1[a];

			Vertices[a].Tangent = (Tangent - Normal * DotProduct(Normal, Tangent));
			Vertices[a].Tangent->normalize;

			Vertices[a].Tangent[3] = (DotProduct(CrossProduct(Normal, Tangent, Tan2[a])) < 0.0f) ? -1.0f : 1.0f;

			Vertices[a].BiNormal[0]
		}* /

		delete[] Tan1;*/
	}

	unsigned int NodeCount(TNode<Type>* Node)
	{
		unsigned int NumNodes = 1;
		for (unsigned int NodeIter = 0; NodeIter < Node->Children.size(); NodeIter++)
		{
			NumNodes += NodeCount(Node->Children[NodeIter]);
		}
		return NumNodes;
	}

	bool SaveTinyModel(const char* FileName)
	{
		FILE* File = fopen(FileName, "wb");

		unsigned int I, J = 0;
		unsigned int Address = 0;

		fwrite(&AmbientLight, sizeof(Type), 4, File);

		fwrite(&Materials.size(), sizeof(unsigned int), 1, File);

#if defined(_MSC_VER)
		for each(auto MaterialIter in Materials)
#else
		for (auto MaterialIter : Materials)
#endif
		{
			Address = (unsigned int)MaterialIter->second;

			fwrite(&Address, sizeof(unsigned int), 1, File);

			fwrite(&MaterialIter->second, sizeof(TMaterial<Type>), 1, File);
		}

		unsigned int ObjectCount = NodeCount(Root);
		fwrite(&ObjectCount, sizeof(unsigned int), 1, File);
		SaveNodeData(Root, File);

		ObjectCount = Skeletons.size();
		fwrite(&ObjectCount, sizeof(unsigned int), 1, File);

		for (unsigned int SkeletonIter = 0; SkeletonIter < Skeletons.size(); SkeletonIter++)
		{
			fwrite(&Skeletons[SkeletonIter]->BoneCount, sizeof(unsigned int), 1, File);
			fwrite(&Skeletons[SkeletonIter]->BindPoses, sizeof(Type) * 16, Skeletons[SkeletonIter].BoneCount, File);
			fwrite(&Skeletons[SkeletonIter]->Nodes, sizeof(TNode<Type>*), Skeletons[SkeletonIter].BoneCount, File);
		}

		ObjectCount = Animations.size();
		fwrite(&ObjectCount, sizeof(unsigned int), 1, File);

#if defined(_MSC_VER)
		for each(auto Iter in Animations)
#else
		for (auto Iter : Animations)
#endif
		{
			fwrite(Iter->second->Name, sizeof(char), 255, File);
			fwrite(&Iter->second->StartFrame, sizeof(unsigned int), 255, File);
			fwrite(&Iter->second->EndFrame, sizeof(unsigned int), 255, File);
			fwrite(&Iter->second->TrackCount, sizeof(unsigned int), 255, File);

			for (unsigned int TrackIter = 0; TrackIter < Iter->second->TrackCount; TrackIter++)
			{
				fwrite(&Iter->second->Tracks[TrackIter].BoneIndex, sizeof(unsigned int), 1, File);
				fwrite(&Iter->second->Tracks[TrackIter].KeyFrameCount, sizeof(unsigned int), 1, File);
				fwrite(Iter->second->Tracks[TrackIter].KeyFrames, sizeof(TKeyFrame<Type>), Iter->second->Tracks[TrackIter].KeyFrameCount, File);
			}
		}

		fclose(File);
		return true;
	}

	void SaveNodeData(TNode<Type>* Node, FILE* File)
	{
		unsigned int Address = (unsigned int)Node;
		fwrite(&Address, sizeof(unsigned int), 1, File);

		fwrite(&Node->NodeType, sizeof(unsigned int), 1, File);

		fwrite(&Node->Name, sizeof(char), 255, File);

		Address = (unsigned int)Node->Parent;
		fwrite(&Address, sizeof(unsigned int), 1, File);

		fwrite(&Node->LocalTransform, sizeof(Type), 16, File);
		fwrite(&Node->GlobalTransform, sizeof(Type), 16, File);

		switch (Node->NodeType)
		{
		case TNode<Type>::TMESH:
		{
			SaveMeshData((TMeshNode<Type>*)Node, File);
			break;
		}

		case TNode<Type>::TLIGHT:
		{
			SaveLightData((TLightNode<Type>*)Node, File);
			break;
		}

		case TNode<Type>::TCAMERA:
		{
			SaveCameraData((TLightNode<Type>*)Node, File);
			break;
		}

		default:
		{
			break;
		}
		}

		unsigned int ChildCount = Node->Children.size();
		fwrite(&ChildCount, sizeof(unsigned int), 1, File);

		for (unsigned int ChildIter = 0; ChildIter < ChildCount; ChildIter++)
		{
			Address = (unsigned int)Node->Children[ChildIter];
			fwrite(&Address, sizeof(unsigned int), 1, File);
		}

		for (unsigned int NodeIter = 0; NodeIter < Node->Children.size(); NodeIter++)
		{
			SaveNodeData(Node->Children[NodeIter], File);
		}
	}

	void SaveMeshData(TMeshNode<Type>* Mesh, FILE* File)
	{
		unsigned int Address = (unsigned int)Mesh->Material;
		fwrite(&Address, sizeof(unsigned int), 1, File);

		unsigned int VertexCount = Mesh->Vertices.size();
		fwrite(VertexCount, sizeof(unsigned int), 1, File);

		fwrite(Mesh->Vertices.data(), sizeof(TVertex<Type>), VertexCount, File);

		unsigned int IndexCount = Mesh->Indices.size();
		fwrite(IndexCount, sizeof(unsigned int), 1, File);

		fwrite(Mesh->Indices.data(), sizeof(unsigned int), IndexCount, File);
	}

	void SaveLightData(TLightNode<Type>* Light, FILE* File)
	{
		unsigned int Value = Light->LightType;
		fwrite(&Value, sizeof(unsigned int), 1, File);

		Value = Light->On ? 1 : 0;
		fwrite(&Value, sizeof(unsigned int), 1, File);

		fwrite(&Light->Color, sizeof(Type), 4, File);

		fwrite(&Light->InnerAngle, sizeof(Type), 1, File);
		fwrite(&Light->OuterAngle, sizeof(Type), 1, File);

		fwrite(&Light->Attenuation, sizeof(Type), 4, File);
	}

	void SaveCameraData(TCameraNode<Type>* Camera, FILE* File)
	{
		fwrite(&Camera->AspectRatio, sizeof(Type), 1, File);
		fwrite(&Camera->FOV, sizeof(Type), 1, File);
		fwrite(&Camera->Near, sizeof(Type), 1, File);
		fwrite(&Camera->Far, sizeof(Type), 1, File);

		fwrite(&Camera->ViewMatrix, sizeof(Type), 16, File);
	}

	bool LoadTinyModel(const char* FileName)
	{
		FILE* File = fopen(FileName, "rb");

		unsigned int I, J = 0;
		unsigned int Address = 0;
		unsigned int NodeType = TNode<Type>::TNODE;

		fread(&AmbientLight, sizeof(Type), 4, File);

		unsigned int MaterialCount, NodeCount, SkeletonCount, AnimationCount = 0;
		fread(&MaterialCount, sizeof(unsigned int), 1, File);

		std::map<unsigned int, TMaterial<Type>*> MaterialMap;
		std::map<unsigned int, TNode<Type>*> NodeMap;

		for (I = 0; I < MaterialCount; I++)
		{
			TMaterial<Type>* Material = new TMaterial<Type>();

			fread(&Address, sizeof(unsigned int), 1, File);

			fread(&Material, sizeof(TMaterial<Type>), 1, File);

			MaterialMap[Address] = Material;
			Materials[Material->Name] = Material;
		}

		fread(&NodeCount, sizeof(unsigned int), 1, File);
		if (NodeCount > 0)
		{
			LoadNode(NodeMap, MaterialMap, File);
			ReLink(Root, NodeMap);
		}

		fread(&SkeletonCount, sizeof(unsigned int), 1, File);
		for (I = 0; I < SkeletonCount; I++)
		{
			TSkeleton<Type>* Skeleton = new TSkeleton<Type>();

			fread(&Skeleton->BoneCount, sizeof(unsigned int), 1, File);

			Skeleton->BindPoses = new Type[Skeleton->BoneCount][16];
			Skeleton->Bones = new Type[Skeleton->BoneCount][16];
			Skeleton->Nodes = new TNode<Type> *[Skeleton->BoneCount];

			fread(Skeleton->BindPoses, sizeof(Type) * 16, Skeleton->BoneCount, File);
			fread(Skeleton->Nodes, sizeof(TNode<Type>*), Skeleton->BoneCount, File);

			for (J = 0; J < Skeleton->BoneCount; J++)
			{
				Skeleton->Nodes[J] = NodeMap[(unsigned int)Skeleton->Nodes[J]];
			}

			Skeletons.push_back(Skeleton);
		}

		fread(AnimationCount, sizeof(unsigned int), 1, File);

		for (I = 0; I < AnimationCount; I++)
		{
			TAnimation<Type>* Animation = new TAnimation<Type>();
			
			fread(Animation->Name, sizeof(char), 255, File);
			fread(&Animation->StartFrame, sizeof(unsigned int), 1, File);
			fread(&Animation->EndFrame, sizeof(unsigned int), 1, File);
			fread(&Animation->TrackCount, sizeof(unsigned int), 1, File);

			Animation->Tracks = new TTrack<Type>[Animation->TrackCount];

			for (J = 0; J < Animation->TrackCount; J++)
			{
				fread(&Animation->Tracks[J].BoneIndex, sizeof(unsigned int), 1, File);
				fread(&Animation->Tracks[J].KeyFrameCount, sizeof(unsigned int), 1, File);

				Animation->Tracks[J].KeyFrames = new TKeyFrame<Type>[Animation->Tracks[J].KeyFrameCount];
				fread(Animation->Tracks[J].KeyFrames, sizeof(TKeyFrame<Type>), Animation->Tracks[J].KeyFrameCount, File);
			}

			Animations[Animation->Name] = Animation;
		}
		fclose(File);
		return true;
	}

	void LoadNode(std::map<unsigned int, TNode<Type>*>& Nodes,
		std::map<unsigned int, TMaterial<Type>*>& MaterialMap, FILE* File)
	{
		unsigned int Address = 0;
		unsigned int Parent = 0;
		unsigned int NodeType = TNode<Type>::TNODE;

		fread(Address, sizeof(unsigned int), 1, File);
		fread(NodeType, sizeof(unsigned int), 1, File);
		
		char Name[255];
		fread(Name, sizeof(char), 255, File);

		fread(&Parent, sizeof(unsigned int), 1, File);

		Type LocalTransform[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
		Type GlobalTransform[16] = LocalTransform;

		fread(LocalTransform, sizeof(Type), 16, File);
		fread(GlobalTransform, sizeof(Type), 16, File);

		TNode<Type>* TinyNode = nullptr;

		switch (NodeType)
		{
		case TNode<Type>::TNODE:
		{
			TinyNode = new TNode<Type>();
			break;
		}

		case TNode<Type>::TMESH:
		{
			TinyNode = new TMeshNode<Type>();
			LoadMeshData((TMeshNode<Type>*)TinyNode, MaterialMap, File);
			break;
		}

		case TNode<Type>::TLIGHT:
		{
			TinyNode = new TLightNode<Type>();
			LoadLightData((TLightNode<Type>*)TinyNode, File);
			break;
		}

		case TNode<Type>::TCAMERA:
		{
			TinyNode = new TCameraNode<Type>();
			LoadCameraData((TCameraNode<Type>*)TinyNode, File);
			break;
		}

		default:
		{
			break;
		}
		}

		TinyNode->LocalTransform = LocalTransform;
		TinyNode->GlobalTransform = GlobalTransform;
		Nodes[Address] = TinyNode;

		TinyNode->Parent = (TNode<Type>*)Parent;

		if (Root == nullptr)
		{
			Root = TinyNode;
		}
		
		TinyNode->NodeType = NodeType;
		strncpy(TinyNode->Name, Name, 255);

		switch (NodeType)
		{
		case TNode<Type>::TMESH:
		{
			Meshes[TinyNode->Name] = (TMeshNode<Type>*)TinyNode;
			break;
		}

		case TNode<Type>::TLIGHT:
		{
			Lights[TinyNode->Name] = (TLightNode<Type>*)TinyNode;
			break;
		}

		case TNode<Type>::TCAMERA:
		{
			Cameras[TinyNode->Name] = (TCameraNode<Type>*)TinyNode;
			break;
		}

		default:
		{
			break;
		}
		}

		unsigned int ChildCount = 0;
		fread(&ChildCount, sizeof(unsigned int), 1, File);

		unsigned int ChildIter = 0;

		for (ChildIter = 0; ChildIter < ChildCount; ChildIter++)
		{
			fread(&Address, sizeof(unsigned int), 1, File);
			TinyNode->Children.push_back((TNode<Type>*)Address);
		}

		for (ChildIter = 0; ChildIter < ChildCount; ChildIter++)
		{
			LoadNode(Nodes, MaterialMap, File);
		}
	}

	void LoadMeshData(TMeshNode<Type>* Mesh, std::map<unsigned int, TMaterial<Type>*> MaterialMap, FILE* File)
	{
		unsigned int Address = 0;
		fread(&Address, sizeof(unsigned int), 1, File);
		Mesh->Material = MaterialMap[Address];

		unsigned int VertexCount = 0;
		fread(&VertexCount, sizeof(unsigned int), 1, File);

		if (VertexCount > 0)
		{
			TVertex<Type>* Vertices = new TVertex<Type>[VertexCount];
			fread(Vertices, sizeof(TVertex<Type>), VertexCount, File);
			for (unsigned int VertexIter = 0; VertexIter < VertexCount; VertexIter++)
			{
				Mesh->Vertices.push_back(Vertices[VertexIter]);
			}
			delete[] Vertices;
		}

		unsigned int IndexCount = 0;
		fread(&IndexCount, sizeof(unsigned int), 1, File);

		if (IndexCount > 0)
		{
			unsigned int* Indices = new unsigned int[IndexCount];
			fread(Indices, sizeof(unsigned int), IndexCount, File);
			for (unsigned int IndexIter = 0; IndexIter < IndexIter; IndexIter++)
			{
				Mesh->Indices.push_back(Indices[IndexIter]);
			}
			delete[] Indices;
		}

	}

	void LoadLightData(TLightNode<Type>* Light, FILE* File)
	{
		unsigned int Value = 0;
		fread(&Value, sizeof(unsigned int), 1, File);
		Light->LightType = Value;

		Value = 0;
		fread(&Value, sizeof(unsigned int), 1, File);
		Light->LightType = Value != 0;

		fread(&Light->Color, sizeof(Type), 4, File);

		fread(&Light->InnerAngle, sizeof(Type), 1, File);
		fread(&Light->OuterAngle, sizeof(Type), 1, File);

		fread(&Light->Attenuation, sizeof(Type), 4, File);
	}

	void LoadCameraData(TCameraNode<Type>* Camera, FILE* File)
	{
		fread(&Camera->AspectRatio, sizeof(Type), 1, File);
		fread(&Camera->FOV, sizeof(Type), 1, File);
		fread(&Camera->Near, sizeof(Type), 1, File);
		fread(&Camera->Far, sizeof(Type), 1, File);

		fread(&Camera->ViewMatrix, sizeof(Type), 16, File);
	}

	void ReLink(TNode<Type>* Node, std::map<unsigned int, TNode<Type>*>& Nodes)
	{
		if (Node->Parent != nullptr)
		{
			Node->Parent = Nodes[(unsigned int)Node->Parent];
		}
		unsigned int ChildCount = Node->Children.size();
		for (unsigned int ChildIter = 0; ChildIter < ChildCount; ChildIter++)
		{
			Node->Children[ChildIter] = Nodes[(unsigned int)Node->Children[ChildIter]];
			ReLink(Node->Children[ChildIter], Nodes);
		}
	}

	TNode<Type>* Root;

	char* Path;
	Type AmbientLight[4];

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

	struct ImportAssistor
	{
		ImportAssistor() : Evaluator(nullptr){}
		~ImportAssistor()
		{
			Evaluator = nullptr;
		}

		FbxScene* CurrentScene;
		FbxAnimEvaluator* Evaluator;
		std::vector<TNode<Type>*> Bones;

		std::map<const char*, unsigned int> BoneIndexMap;
	};

	ModelManager()
	{
		ModelManager::Assistor = nullptr; 
	}

	ImportAssistor* GetImportAssistor()
	{
		return Assistor;
	}

	static ModelManager* GetInstance()
	{
		if (ModelManager::Instance == nullptr)
		{
			ModelManager::Instance = new ModelManager();
			return ModelManager::Instance;
		}
		return ModelManager::Instance;
	}

private:
	static ModelManager* Instance;
	ImportAssistor* Assistor;
	
	std::map<const char*, TScene<Type>*> Scenes;

	void DisplayContent(TScene<Type>* Scene);
	void DisplayHierarchy(TScene<Type>* Scene);
	void DisplayPose(TScene<Type>* Scene);
	void DisplayAnimation(TScene<Type>* Scene);

};

template<typename Type>
ModelManager<Type>* ModelManager<Type>::Instance = nullptr;
#endif
