//////////////////////////////////////////////////////////////////////////
// Author:	Conan Bourke
// Date:	January 5 2013
// Brief:	Classes to load an FBX scene for use
//////////////////////////////////////////////////////////////////////////
#include "FBXLoader.h"
#include <fbxsdk.h>
#include <algorithm>
#include <set>
#include <glm/glm.hpp>
#include <fbxsdk/fbxsdk_version.h>
#include <fbxsdk/fileio/fbx/fbxio.h>
#include "MathHelper.h"
#include <cmath>

#define GLM_SWIZZLE

	struct ImportAssistor
	{
		ImportAssistor() : evaluator(nullptr) {}
		~ImportAssistor() { evaluator = nullptr; }

		FbxScene*			scene;
		FbxAnimEvaluator*	evaluator;
		std::vector<Node*>	bones;

		std::map<std::string,int> boneIndexList;
	};

	ImportAssistor* g_Assistor = nullptr;

	void DisplayContent(FbxScene* pScene);
	void DisplayHierarchy(FbxScene* pScene);
	void DisplayPose(FbxScene* pScene);
	void DisplayAnimation(FbxScene* pScene);

	//////////////////////////////////////////////////////////////////////////
	FBXMeshNode* FBXScene::GetMeshByName(const char* a_name)
	{
		auto oIter = m_meshes.find(a_name);
		if (oIter != m_meshes.end())
			return oIter->second;
		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	FBXLightNode* FBXScene::GetLightByName(const char* a_name)
	{
		auto oIter = m_lights.find(a_name);
		if (oIter != m_lights.end())
			return oIter->second;
		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	FBXCameraNode* FBXScene::GetCameraByName(const char* a_name)
	{
		auto oIter = m_cameras.find(a_name);
		if (oIter != m_cameras.end())
			return oIter->second;
		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	FBXMaterial* FBXScene::GetMaterialByName(const char* a_name)
	{
		auto oIter = m_materials.find(a_name);
		if (oIter != m_materials.end())
			return oIter->second;
		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	FBXAnimation* FBXScene::GetAnimationByName(const char* a_name)
	{
		auto oIter = m_animations.find(a_name);
		if (oIter != m_animations.end())
			return oIter->second;
		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	FBXMeshNode* FBXScene::GetMeshByIndex(unsigned int a_index)
	{
		FBXMeshNode* mesh = nullptr;
		auto iter = m_meshes.begin();
		unsigned int size = m_meshes.size();

		for ( unsigned int i = 0 ; i <= a_index && a_index < size ; ++i, ++iter )
			mesh = iter->second; 

		return mesh;
	}

	//////////////////////////////////////////////////////////////////////////
	FBXLightNode* FBXScene::GetLightByIndex(unsigned int a_index)
	{
		FBXLightNode* light = nullptr;
		auto iter = m_lights.begin();
		unsigned int size = m_lights.size();

		for ( unsigned int i = 0 ; i <= a_index && a_index < size ; ++i, ++iter )
			light = iter->second; 

		return light;
	}

	//////////////////////////////////////////////////////////////////////////
	FBXCameraNode* FBXScene::GetCameraByIndex(unsigned int a_index)
	{
		FBXCameraNode* camera = nullptr;
		auto iter = m_cameras.begin();
		unsigned int size = m_cameras.size();

		for ( unsigned int i = 0 ; i <= a_index && a_index < size ; ++i, ++iter )
			camera = iter->second; 

		return camera;
	}

	//////////////////////////////////////////////////////////////////////////
	FBXMaterial* FBXScene::GetMaterialByIndex(unsigned int a_index)
	{
		FBXMaterial* material = nullptr;
		auto iter = m_materials.begin();
		unsigned int size = m_materials.size();

		for ( unsigned int i = 0 ; i <= a_index && a_index < size ; ++i, ++iter )
			material = iter->second; 

		return material;
	}

	//////////////////////////////////////////////////////////////////////////
	FBXAnimation* FBXScene::GetAnimationByIndex(unsigned int a_index)
	{
		FBXAnimation* animation = nullptr;
		auto iter = m_animations.begin();
		unsigned int size = m_animations.size();

		for ( unsigned int i = 0 ; i <= a_index && a_index < size ; ++i, ++iter )
			animation = iter->second; 

		return animation;
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::Unload()
	{
		delete m_root;
		m_root = nullptr;
		
		#ifdef _MSC_VER

		for (std::map<std::string, FBXMaterial*>::iterator l_Iter = m_materials.begin(); l_Iter != m_materials.end(); l_Iter++)
		{
			delete l_Iter._Ptr->_Myval.second;
		}
		
		#else
		
		for(auto l_Iter : m_materials)
		{
			delete l_Iter.second;
		}
		
		#endif

		for (unsigned int i = 0; i < m_skeletons.size(); i++)
		{
			delete m_skeletons[i];
		}
		
		#ifdef _MSC_VER

		for (std::map<std::string, FBXAnimation*>::iterator l_Iter = m_animations.begin(); l_Iter != m_animations.end(); l_Iter++)
		{
			for (unsigned int i = 0 ; i < l_Iter._Ptr->_Myval.second->m_trackCount ; ++i )
				delete[] l_Iter._Ptr->_Myval.second->m_tracks[i].m_keyframes;
			delete[] l_Iter._Ptr->_Myval.second->m_tracks;
			delete l_Iter._Ptr->_Myval.second;
		}
		
		#else 
		
		for (auto l_Iter : m_animations)
		{
			for (unsigned int i = 0 ; i < l_Iter.second->m_trackCount ; ++i )
				delete[] l_Iter.second->m_tracks[i].m_keyframes;
			delete[] l_Iter.second->m_tracks;
			delete l_Iter.second;
			
		}
		
		#endif

		m_meshes.clear();
		m_lights.clear();
		m_cameras.clear();
		m_materials.clear();
		m_skeletons.clear();
		m_animations.clear();
	}
	
	void FBXScene::GatherBones(void* a_object)
	{
		FbxNode* fbxNode = (FbxNode*)a_object;

		if (fbxNode->GetNodeAttribute() != nullptr &&
			fbxNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			unsigned int index = g_Assistor->boneIndexList.size();

			char name[MAX_PATH];
			strncpy(name,fbxNode->GetName(),MAX_PATH);
			g_Assistor->boneIndexList[ name ] = index;
		}

		for (int i = 0; i < fbxNode->GetChildCount(); i++)
		{
			GatherBones((void*)fbxNode->GetChild(i));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	bool FBXScene::Load(const char* a_filename)
	{
		if (m_root != nullptr)
		{
			printf("Scene already loaded!\n");
			return false;
		}

		FbxManager* lSdkManager = nullptr;
		FbxScene* lScene = nullptr;

		// The first thing to do is to create the FBX SDK manager which is the 
		// object allocator for almost all the classes in the SDK.
		lSdkManager = FbxManager::Create();
		if ( !lSdkManager )
		{
			printf("Unable to create the FBX SDK manager\n");
			return false;
		}

		// create an IOSettings object
		FbxIOSettings * ios = FbxIOSettings::Create(lSdkManager, IOSROOT );
		lSdkManager->SetIOSettings(ios);

		// Load plugins from the executable directory
		FbxString lPath = FbxGetApplicationDirectory();
		lSdkManager->LoadPluginsDirectory(lPath.Buffer());

		// Create the entity that will hold the scene.
		lScene = FbxScene::Create(lSdkManager,"");

		int lFileMajor, lFileMinor, lFileRevision;
		int lSDKMajor,  lSDKMinor,  lSDKRevision;

		unsigned int i;
		bool lStatus;

		// Get the file version number generate by the FBX SDK.
		FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

		// Create an importer.
		FbxImporter* lImporter = FbxImporter::Create(lSdkManager,"");

		// Initialize the importer by providing a filename.
		const bool lImportStatus = lImporter->Initialize(a_filename, -1, lSdkManager->GetIOSettings());
		lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

		if ( !lImportStatus )
		{
			/*printf("Call to FbxImporter::Initialize() failed.\n");
			printf("Error returned: %s\n\n", lImporter->GetLastErrorString());

			if (lImporter->GetLastErrorID() == FbxIOBase::eFileVersionNotSupportedYet ||
				lImporter->GetLastErrorID() == FbxIOBase::eFileVersionNotSupportedAnymore)
			{
				printf("FBX version number for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
				printf("FBX version number for file %s is %d.%d.%d\n\n", a_filename, lFileMajor, lFileMinor, lFileRevision);
			}*/

			lImporter->Destroy();
			return false;
		}

		// Import the scene.
		lStatus = lImporter->Import(lScene);
		lImporter->Destroy();
		if (lStatus == false)
		{
			printf("Unable to open FBX file!\n");
			return false;
		}

		// convert the scene to OpenGL axis (right-handed Y up)
		FbxAxisSystem::OpenGL.ConvertScene(lScene);

		FbxNode* lNode = lScene->GetRootNode();

		if (lNode != nullptr)
		{
			g_Assistor = new ImportAssistor();

			g_Assistor->scene = lScene;
			//g_Assistor->evaluator = lScene->GetEvaluator();

			m_root = new Node();
			strcpy(m_root->m_name,"root");

			// the root node's transform needs to be switched from LH to RH
			m_root->m_globalTransform = m_root->m_localTransform = mat4(1,0,0,0,0,1,0,0,0,0,-1,0,0,0,0,1);

			// grab the ambient light data from the scene
			m_ambientLight.x = (float)lScene->GetGlobalSettings().GetAmbientColor().mRed;
			m_ambientLight.y = (float)lScene->GetGlobalSettings().GetAmbientColor().mGreen;
			m_ambientLight.z = (float)lScene->GetGlobalSettings().GetAmbientColor().mBlue;
			m_ambientLight.w = (float)lScene->GetGlobalSettings().GetAmbientColor().mAlpha;

			for (i = 0; i < (unsigned int)lNode->GetChildCount(); ++i)
			{
				GatherBones((void*)lNode->GetChild(i));
			}

			// extract children
			for (i = 0; i < (unsigned int)lNode->GetChildCount(); ++i)
			{
				ExtractObject(m_root, (void*)lNode->GetChild(i));
			}

			if (g_Assistor->bones.size() > 0)
			{
				FBXSkeleton* skeleton = new FBXSkeleton();
				skeleton->m_boneCount = g_Assistor->bones.size();
				skeleton->m_nodes = new Node * [ skeleton->m_boneCount ];
				skeleton->m_bones = new mat4[ skeleton->m_boneCount ];
				skeleton->m_bindPoses = new mat4[ skeleton->m_boneCount ];

				for ( i = 0 ; i < skeleton->m_boneCount ; ++i )
				{
					skeleton->m_nodes[ i ] = g_Assistor->bones[ i ];
					skeleton->m_bones[ i ] = skeleton->m_nodes[ i ]->m_localTransform;
				}

				ExtractSkeleton(skeleton, lScene);

				m_skeletons.push_back(skeleton);

				ExtractAnimation(lScene);
			}

	//		DisplayContent(lScene);
	/*		DisplayPose(lScene);
			DisplayHierarchy(lScene);
			DisplayAnimation(lScene);
	*/

			delete g_Assistor;
		}

		lSdkManager->Destroy();

		// store the folder path of the scene
		m_path = a_filename;
		int iLastForward = m_path.find_last_of('/');
		int iLastBackward = m_path.find_last_of('\\');
		if (iLastForward > iLastBackward)
		{
			m_path.resize(iLastForward + 1);
		}
		else if (iLastBackward != 0)
		{
			m_path.resize(iLastBackward + 1);
		}
		else
		{
			m_path = "";
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::ExtractObject(Node* a_parent, void* a_object)
	{
		FbxNode* fbxNode = (FbxNode*)a_object;

		Node* node = nullptr;

		FbxNodeAttribute::EType lAttributeType;
		int i;

		bool bIsBone = false;

		if (fbxNode->GetNodeAttribute() != nullptr)
		{
			lAttributeType = (fbxNode->GetNodeAttribute()->GetAttributeType());

			switch (lAttributeType)
			{
			case FbxNodeAttribute::eMarker:		break;
			case FbxNodeAttribute::eSkeleton:	
				{
					bIsBone = true;
				}
				break;

			case FbxNodeAttribute::eMesh:      
				{
					node = new FBXMeshNode();
					ExtractMesh((FBXMeshNode*)node,fbxNode);

					if (strlen(fbxNode->GetName()) > 0)
						strncpy(node->m_name,fbxNode->GetName(),MAX_PATH-1);

					m_meshes[node->m_name] = (FBXMeshNode*)node;
				}
				break;

			case FbxNodeAttribute::eNurbs:	break;
			case FbxNodeAttribute::ePatch:	break;

			case FbxNodeAttribute::eCamera:   
				{
					node = new FBXCameraNode();
					ExtractCamera((FBXCameraNode*)node,fbxNode);

					if (strlen(fbxNode->GetName()) > 0)
						strncpy(node->m_name,fbxNode->GetName(),MAX_PATH-1);

					m_cameras[node->m_name] = (FBXCameraNode*)node;
				}
				break;

			case FbxNodeAttribute::eLight:   
				{
					node = new FBXLightNode();
					ExtractLight((FBXLightNode*)node,fbxNode);

					if (strlen(fbxNode->GetName()) > 0)
						strncpy(node->m_name,fbxNode->GetName(),MAX_PATH-1);

					m_lights[node->m_name] = (FBXLightNode*)node;
				}
				break;
			}   
		}

		// if null then use it as a plain 3D node
		if (node == nullptr)
		{
			node = new Node();
			if (strlen(fbxNode->GetName()) > 0)
				strncpy(node->m_name,fbxNode->GetName(),MAX_PATH-1);
		}

		// add to parent's children and update parent
		a_parent->m_children.push_back(node);
		node->m_parent = a_parent;

		// build local transform
		// use anim evaluator as bones store transforms in a different spot
		FbxAMatrix lLocal = g_Assistor->evaluator->GetNodeLocalTransform(fbxNode);
		
		FbxVector4 row0 = lLocal.GetRow(0);
		FbxVector4 row1 = lLocal.GetRow(1);
		FbxVector4 row2 = lLocal.GetRow(2);
		FbxVector4 row3 = lLocal.GetRow(3);

		node->m_localTransform[0] = vec4( (float)row0[0], (float)row0[1], (float)row0[2], (float)row0[3] );
		node->m_localTransform[1] = vec4( (float)row1[0], (float)row1[1], (float)row1[2], (float)row1[3] );
		node->m_localTransform[2] = vec4( (float)row2[0], (float)row2[1], (float)row2[2], (float)row2[3] );
		node->m_localTransform[3] = vec4( (float)row3[0], (float)row3[1], (float)row3[2], (float)row3[3] );

		// set global based off local and parent's global
		node->m_globalTransform = node->m_localTransform * a_parent->m_globalTransform;

		if (bIsBone == true)
		{
			g_Assistor->bones.push_back(node);
		}

		// children
		for (i = 0; i < fbxNode->GetChildCount(); i++)
		{
			ExtractObject(node, (void*)fbxNode->GetChild(i));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::ExtractMesh(FBXMeshNode* a_mesh, void* a_object)
	{
		FbxNode* fbxNode = (FbxNode*)a_object;
		FbxMesh* fbxMesh = (FbxMesh*)fbxNode->GetNodeAttribute();

		int i, j, lPolygonCount = fbxMesh->GetPolygonCount();
		FbxVector4* lControlPoints = fbxMesh->GetControlPoints(); 

		FBXVertex vertex;
		unsigned int vertexIndex[4] = {};
		
		int vertexId = 0;
		for (i = 0; i < lPolygonCount; i++)
		{
			int l;
			int lPolygonSize = fbxMesh->GetPolygonSize(i);

			for (j = 0; j < lPolygonSize && j < 4 ; j++)
			{
				int lControlPointIndex = fbxMesh->GetPolygonVertex(i, j);

				vertex.fbxControlPointIndex = lControlPointIndex;

				// POSITION
				FbxVector4 vPos = lControlPoints[lControlPointIndex];
				vertex.position.x = (float)vPos[0];
				vertex.position.y = (float)vPos[1];
				vertex.position.z = (float)vPos[2];
				
				for (l = 0; l < fbxMesh->GetElementVertexColorCount(); l++)
				{
					FbxGeometryElementVertexColor* leVtxc = fbxMesh->GetElementVertexColor( l);

					switch (leVtxc->GetMappingMode())
					{
					case FbxGeometryElement::eByControlPoint:
						switch (leVtxc->GetReferenceMode())
						{
						case FbxGeometryElement::eDirect:
							{						
								FbxColor colour = leVtxc->GetDirectArray().GetAt(lControlPointIndex);

								vertex.colour.x = (float)colour.mRed;
								vertex.colour.y = (float)colour.mGreen;
								vertex.colour.z = (float)colour.mBlue;
								vertex.colour.w = (float)colour.mAlpha;
							}
							break;
						case FbxGeometryElement::eIndexToDirect:
							{
								int id = leVtxc->GetIndexArray().GetAt(lControlPointIndex);
								FbxColor colour = leVtxc->GetDirectArray().GetAt(id);

								vertex.colour.x = (float)colour.mRed;
								vertex.colour.y = (float)colour.mGreen;
								vertex.colour.z = (float)colour.mBlue;
								vertex.colour.w = (float)colour.mAlpha;
							}
							break;
						default:
							break; // other reference modes not shown here!
						}
						break;

					case FbxGeometryElement::eByPolygonVertex:
						{
							switch (leVtxc->GetReferenceMode())
							{
							case FbxGeometryElement::eDirect:
								{							
									FbxColor colour = leVtxc->GetDirectArray().GetAt(vertexId);

									vertex.colour.x = (float)colour.mRed;
									vertex.colour.y = (float)colour.mGreen;
									vertex.colour.z = (float)colour.mBlue;
									vertex.colour.w = (float)colour.mAlpha;
								}
								break;
							case FbxGeometryElement::eIndexToDirect:
								{
									int id = leVtxc->GetIndexArray().GetAt(vertexId);
									FbxColor colour = leVtxc->GetDirectArray().GetAt(id);

									vertex.colour.x = (float)colour.mRed;
									vertex.colour.y = (float)colour.mGreen;
									vertex.colour.z = (float)colour.mBlue;
									vertex.colour.w = (float)colour.mAlpha;
								}
								break;
							default:
								break; // other reference modes not shown here!
							}
						}
						break;

					case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
					case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
					case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
						break;
					}
				}
				for (l = 0; l < fbxMesh->GetElementUVCount(); ++l)
				{
					FbxGeometryElementUV* leUV = fbxMesh->GetElementUV( l);

					switch (leUV->GetMappingMode())
					{
					case FbxGeometryElement::eByControlPoint:
						switch (leUV->GetReferenceMode())
						{
						case FbxGeometryElement::eDirect:
							{
								FbxVector2 uv = leUV->GetDirectArray().GetAt(lControlPointIndex);

								vertex.uv.x = (float)uv[0];
								vertex.uv.y = (float)uv[1];
							}
							break;
						case FbxGeometryElement::eIndexToDirect:
							{
								int id = leUV->GetIndexArray().GetAt(lControlPointIndex);
								FbxVector2 uv = leUV->GetDirectArray().GetAt(id);

								vertex.uv.x = (float)uv[0];
								vertex.uv.y = (float)uv[1];
							}
							break;
						default:
							break; // other reference modes not shown here!
						}
						break;

					case FbxGeometryElement::eByPolygonVertex:
						{
							int lTextureUVIndex = fbxMesh->GetTextureUVIndex(i, j);
							switch (leUV->GetReferenceMode())
							{
							case FbxGeometryElement::eDirect:
							case FbxGeometryElement::eIndexToDirect:
								{
									FbxVector2 uv = leUV->GetDirectArray().GetAt(lTextureUVIndex);

									vertex.uv.x = (float)uv[0];
									vertex.uv.y = (float)uv[1];
								}								
								break;
							default:
								break; // other reference modes not shown here!
							}
						}
						break;

					case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
					case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
					case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
						break;
					}
				}
				for( l = 0; l < fbxMesh->GetElementNormalCount(); ++l)
				{
					FbxGeometryElementNormal* leNormal = fbxMesh->GetElementNormal( l);

					if (leNormal->GetMappingMode() == FbxGeometryElement::eByControlPoint)
					{
						switch (leNormal->GetReferenceMode())
						{
						case FbxGeometryElement::eDirect:
							{
								FbxVector4 normal = leNormal->GetDirectArray().GetAt(lControlPointIndex);

								vertex.normal.x = (float)normal[0];
								vertex.normal.y = (float)normal[1];
								vertex.normal.z = (float)normal[2];
							}
							break;
						case FbxGeometryElement::eIndexToDirect:
							{
								int id = leNormal->GetIndexArray().GetAt(lControlPointIndex);
								FbxVector4 normal = leNormal->GetDirectArray().GetAt(id);

								vertex.normal.x = (float)normal[0];
								vertex.normal.y = (float)normal[1];
								vertex.normal.z = (float)normal[2];
							}
							break;
						default:
							break; // other reference modes not shown here!
						}
					}
					else if(leNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
					{
						switch (leNormal->GetReferenceMode())
						{
						case FbxGeometryElement::eDirect:
							{
								FbxVector4 normal = leNormal->GetDirectArray().GetAt(vertexId);

								vertex.normal.x = (float)normal[0];
								vertex.normal.y = (float)normal[1];
								vertex.normal.z = (float)normal[2];
							}
							break;
						case FbxGeometryElement::eIndexToDirect:
							{
								int id = leNormal->GetIndexArray().GetAt(vertexId);
								FbxVector4 normal = leNormal->GetDirectArray().GetAt(id);

								vertex.normal.x = (float)normal[0];
								vertex.normal.y = (float)normal[1];
								vertex.normal.z = (float)normal[2];
							}
							break;
						default:
							break; // other reference modes not shown here!
						}
					}
				}

				// add to 
				vertexIndex[j] = AddVertGetIndex(a_mesh->m_vertices,vertex);
				vertexId++;
			}

			// add triangle indices
			a_mesh->m_indices.push_back(vertexIndex[0]);
			a_mesh->m_indices.push_back(vertexIndex[1]);
			a_mesh->m_indices.push_back(vertexIndex[2]);

			// handle quads
			if (lPolygonSize == 4)
			{
				a_mesh->m_indices.push_back(vertexIndex[0]);
				a_mesh->m_indices.push_back(vertexIndex[2]);
				a_mesh->m_indices.push_back(vertexIndex[3]);
			}
		}

		CalculateTangentsBinormals(a_mesh->m_vertices,a_mesh->m_indices);

		ExtractSkin(a_mesh,(void*)fbxMesh);

		// get materials
		a_mesh->m_material = ExtractMaterial(fbxMesh);
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::ExtractSkin(FBXMeshNode* a_mesh, void* a_node)
	{
		FbxGeometry* pGeometry = (FbxGeometry*)a_node;

		int i, j, k;
		int lClusterCount=0;
		FbxCluster* lCluster;
		char name[MAX_PATH];

		int vertCount = a_mesh->m_vertices.size();

		FbxSkin* pSkin = (FbxSkin *) pGeometry->GetDeformer(0, FbxDeformer::eSkin);

		if (pSkin != nullptr)
		{
			lClusterCount = pSkin->GetClusterCount();
			for (j = 0; j != lClusterCount; ++j)
			{
				lCluster = pSkin->GetCluster(j);
				if (lCluster->GetLink() == nullptr)
					continue;

				strncpy(name,lCluster->GetLink()->GetName(),MAX_PATH);
				int boneIndex = g_Assistor->boneIndexList[name];

				int lIndexCount = lCluster->GetControlPointIndicesCount();
				int* lIndices = lCluster->GetControlPointIndices();
				double* lWeights = lCluster->GetControlPointWeights();

				for (k = 0; k < lIndexCount; k++)
				{
					// find vert using this bone
					for ( i = 0 ; i < vertCount ; ++i )
					{
						if (a_mesh->m_vertices[i].fbxControlPointIndex == lIndices[k])
						{
							// add weight and index
							if (a_mesh->m_vertices[i].weights.x == 0)
							{
								a_mesh->m_vertices[i].weights.x = (float) lWeights[k];
								a_mesh->m_vertices[i].indices.x = (float)boneIndex;
							}
							else if (a_mesh->m_vertices[i].weights.y == 0)
							{
								a_mesh->m_vertices[i].weights.y = (float) lWeights[k];
								a_mesh->m_vertices[i].indices.y = (float)boneIndex;
							}
							else if (a_mesh->m_vertices[i].weights.z == 0)
							{
								a_mesh->m_vertices[i].weights.z = (float) lWeights[k];
								a_mesh->m_vertices[i].indices.z = (float)boneIndex;
							}
							else
							{
								a_mesh->m_vertices[i].weights.w = (float) lWeights[k];
								a_mesh->m_vertices[i].indices.w = (float)boneIndex;
							}
						}
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::ExtractLight(FBXLightNode* a_light, void* a_object)
	{
		FbxNode* fbxNode = (FbxNode*)a_object;
		FbxLight* fbxLight = (FbxLight*)fbxNode->GetNodeAttribute();

		// get type, if on, and colour
		a_light->m_type = (FBXLightNode::LightType)fbxLight->LightType.Get();
		a_light->m_on = fbxLight->CastLight.Get();
		a_light->m_colour.x = (float)fbxLight->Color.Get()[0];
		a_light->m_colour.y = (float)fbxLight->Color.Get()[1];
		a_light->m_colour.z = (float)fbxLight->Color.Get()[2];
		a_light->m_colour.w = (float)fbxLight->Intensity.Get();

		// get spot light angles (will return data even for non-spotlights)
		a_light->m_innerAngle = (float)fbxLight->InnerAngle.Get() * DEG2RAD;
		a_light->m_outerAngle = (float)fbxLight->OuterAngle.Get() * DEG2RAD;

		// get falloff data (none,linear, quadratic), cubic is ignored
		switch (fbxLight->DecayType.Get())
		{
		case 0:
			a_light->m_attenuation = vec4(1,0,0,0);
			break;
		case 1:
			break;
			a_light->m_attenuation = vec4(0,1,0,0);
		case 2:
			break;
			a_light->m_attenuation = vec4(0,0,1,0);
		default:
			break;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::ExtractCamera(FBXCameraNode* a_camera, void* a_object)
	{
		FbxNode* fbxNode = (FbxNode*)a_object;
		FbxCamera* fbxCamera = (FbxCamera*)fbxNode->GetNodeAttribute();

		// get field of view
		if (fbxCamera->ProjectionType.Get() != FbxCamera::eOrthogonal)
		{
			a_camera->m_fieldOfView = (float)fbxCamera->FieldOfView.Get() * DEG2RAD;
		}
		else
		{
			a_camera->m_fieldOfView = 0;
		}

		// get aspect ratio if one was defined
		if (fbxCamera->GetAspectRatioMode() != FbxCamera::eWindowSize)
		{
			a_camera->m_aspectRatio = (float)fbxCamera->AspectWidth.Get() / (float)fbxCamera->AspectHeight.Get();
		}
		else
		{
			a_camera->m_aspectRatio = 0;
		}

		// get near/far
		a_camera->m_near = (float)fbxCamera->NearPlane.Get();
		a_camera->m_far = (float)fbxCamera->FarPlane.Get();

		// build view matrix
		vec3 vEye, vTo, vUp;
		//vEye.w = 1;
		//vTo.w = 1;

		vEye.x = (float)fbxCamera->Position.Get()[0];
		vEye.y = (float)fbxCamera->Position.Get()[1];
		vEye.z = (float)fbxCamera->Position.Get()[2];

		if (fbxNode->GetTarget() != nullptr)
		{
			vTo.x = (float)fbxNode->GetTarget()->LclTranslation.Get()[0];
			vTo.y = (float)fbxNode->GetTarget()->LclTranslation.Get()[1];
			vTo.z = (float)fbxNode->GetTarget()->LclTranslation.Get()[2];
		}
		else
		{
			vTo.x = (float)fbxCamera->InterestPosition.Get()[0];
			vTo.y = (float)fbxCamera->InterestPosition.Get()[1];
			vTo.z = (float)fbxCamera->InterestPosition.Get()[2];
		}

		if (fbxNode->GetTargetUp())
		{
			vUp.x = (float)fbxNode->GetTargetUp()->LclTranslation.Get()[0];
			vUp.y = (float)fbxNode->GetTargetUp()->LclTranslation.Get()[1];
			vUp.z = (float)fbxNode->GetTargetUp()->LclTranslation.Get()[2];

		}
		else
		{
			vUp.x = (float)fbxCamera->UpVector.Get()[0];
			vUp.y = (float)fbxCamera->UpVector.Get()[1];
			vUp.z = (float)fbxCamera->UpVector.Get()[2];
		}

		glm::normalize(vUp);

		//a_camera->m_viewMatrix.ViewLookAt(vEye,vTo,vUp);

		a_camera->m_viewMatrix = glm::lookAt(vEye, vTo, vUp);
	}

	//////////////////////////////////////////////////////////////////////////
	FBXMaterial* FBXScene::ExtractMaterial(void* a_mesh)
	{
		FbxGeometry* pGeometry = (FbxGeometry*)a_mesh;

		// get material count
		int lMaterialCount = 0;
		FbxNode* lNode = nullptr;
		if(pGeometry)
		{
			lNode = pGeometry->GetNode();
			if (lNode)
				lMaterialCount = lNode->GetMaterialCount();    
		}

		// only process first material (for now)
		if (lMaterialCount > 0)
		{
			FbxSurfaceMaterial *lMaterial = lNode->GetMaterial(0);

			char matName[MAX_PATH];
			strncpy(matName,lMaterial->GetName(),MAX_PATH-1);

			auto oIter = m_materials.find(matName);
			if (oIter != m_materials.end())
			{
				return oIter->second;
			}
			else
			{
				FBXMaterial* material = new FBXMaterial;
				memcpy(material->name,matName,MAX_PATH);

				// get the implementation to see if it's a hardware shader.
				const FbxImplementation* lImplementation = GetImplementation(lMaterial, FBXSDK_IMPLEMENTATION_HLSL);
				if (lImplementation == nullptr)
				{
					lImplementation = GetImplementation(lMaterial, FBXSDK_IMPLEMENTATION_CGFX);
				}
				if (lImplementation != nullptr)
				{
					FbxBindingTable const* lRootTable = lImplementation->GetRootTable();
					FbxString lFileName = lRootTable->DescAbsoluteURL.Get();
					FbxString lTechniqueName = lRootTable->DescTAG.Get(); 

					FBXSDK_printf("Unsupported hardware shader material!\n");
					FBXSDK_printf("File: %s\n",lFileName.Buffer());
					FBXSDK_printf("Technique: %s\n\n",lTechniqueName.Buffer());
				}
				else if (lMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
				{
					// We found a Phong material.  Display its properties.
					FbxSurfacePhong* pPhong = (FbxSurfacePhong*)lMaterial;

					// Display the Ambient Color
					material->ambient.x = (float)pPhong->Ambient.Get()[0];
					material->ambient.y = (float)pPhong->Ambient.Get()[1];
					material->ambient.z = (float)pPhong->Ambient.Get()[2];
					material->ambient.w = (float)pPhong->AmbientFactor.Get();

					// Display the Diffuse Color
					material->diffuse.x = (float)pPhong->Diffuse.Get()[0];
					material->diffuse.y = (float)pPhong->Diffuse.Get()[1];
					material->diffuse.z = (float)pPhong->Diffuse.Get()[2];
					material->diffuse.w = 1.0f - (float)pPhong->TransparencyFactor.Get();

					// Display the Specular Color (unique to Phong materials)
					material->specular.x = (float)pPhong->Specular.Get()[0];
					material->specular.y = (float)pPhong->Specular.Get()[1];
					material->specular.z = (float)pPhong->Specular.Get()[2];
					material->specular.w = (float)pPhong->Shininess.Get();

					// Display the Emissive Color
					material->emissive.x = (float)pPhong->Emissive.Get()[0];
					material->emissive.y = (float)pPhong->Emissive.Get()[1];
					material->emissive.z = (float)pPhong->Emissive.Get()[2];
					material->emissive.w = (float)pPhong->EmissiveFactor.Get();
				}
				else if(lMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId) )
				{
					// We found a Lambert material. Display its properties.
					FbxSurfaceLambert* pLambert = (FbxSurfaceLambert*)lMaterial;

					// Display the Ambient Color
					material->ambient.x = (float)pLambert->Ambient.Get()[0];
					material->ambient.y = (float)pLambert->Ambient.Get()[1];
					material->ambient.z = (float)pLambert->Ambient.Get()[2];
					material->ambient.w = (float)pLambert->AmbientFactor.Get();

					// Display the Diffuse Color
					material->diffuse.x = (float)pLambert->Diffuse.Get()[0];
					material->diffuse.y = (float)pLambert->Diffuse.Get()[1];
					material->diffuse.z = (float)pLambert->Diffuse.Get()[2];
					material->diffuse.w = 1.0f - (float)pLambert->TransparencyFactor.Get();

					// No specular in lambert materials
					material->specular.x = 0;
					material->specular.y = 0;
					material->specular.z = 0;
					material->specular.w = 0;

					// Display the Emissive Color
					material->emissive.x = (float)pLambert->Emissive.Get()[0];
					material->emissive.y = (float)pLambert->Emissive.Get()[1];
					material->emissive.z = (float)pLambert->Emissive.Get()[2];
					material->emissive.w = (float)pLambert->EmissiveFactor.Get();
				}
				else
				{
					FBXSDK_printf("Unknown type of Material!\n");
				}

				// attach textures
					/*DisplayDouble("            Scale U: ", pTexture->GetScaleU());
					DisplayDouble("            Scale V: ", pTexture->GetScaleV());
					DisplayDouble("            Translation U: ", pTexture->GetTranslationU());
					DisplayDouble("            Translation V: ", pTexture->GetTranslationV());
					DisplayBool("            Swap UV: ", pTexture->GetSwapUV());
					DisplayDouble("            Rotation U: ", pTexture->GetRotationU());
					DisplayDouble("            Rotation V: ", pTexture->GetRotationV());
					DisplayDouble("            Rotation W: ", pTexture->GetRotationW()); */  

				unsigned int auiTextureLookup[] =
				{
					FbxLayerElement::eTextureDiffuse - FbxLayerElement::sTypeTextureStartIndex,
					FbxLayerElement::eTextureAmbient - FbxLayerElement::sTypeTextureStartIndex,
					FbxLayerElement::eTextureEmissive - FbxLayerElement::sTypeTextureStartIndex,
					FbxLayerElement::eTextureSpecular - FbxLayerElement::sTypeTextureStartIndex,
					FbxLayerElement::eTextureShininess - FbxLayerElement::sTypeTextureStartIndex,
					FbxLayerElement::eTextureNormalMap - FbxLayerElement::sTypeTextureStartIndex,
					FbxLayerElement::eTextureTransparency - FbxLayerElement::sTypeTextureStartIndex,
					FbxLayerElement::eTextureDisplacement - FbxLayerElement::sTypeTextureStartIndex,
				};

				for ( unsigned int i = 0 ; i < FBXMaterial::TextureTypes_Count ; ++i )
				{
					FbxProperty pProperty = lMaterial->FindProperty(FbxLayerElement::sTextureChannelNames[auiTextureLookup[i]]);						
					if ( pProperty.IsValid() &&
						pProperty.GetSrcObjectCount<FbxTexture>() > 0)
					{
						FbxFileTexture *lFileTexture = FbxCast<FbxFileTexture>(pProperty.GetSrcObject<FbxTexture>(0));
						if (lFileTexture)
						{
							const char* szLastForward = strrchr(lFileTexture->GetFileName(),'/');
							const char* szLastBackward = strrchr(lFileTexture->GetFileName(),'\\');
							const char* szFilename = lFileTexture->GetFileName();

							if (szLastForward != nullptr && szLastForward > szLastBackward)
								szFilename = szLastForward + 1;
							else if (szLastBackward != nullptr)
								szFilename = szLastBackward + 1;

							if (strlen(szFilename) >= MAX_PATH)
							{
								FBXSDK_printf("Texture filename too long!: %s\n", szFilename);
							}
							else
							{
								strcpy(material->textureFilenames[i],szFilename);
							}
						}   
					}
				}

				m_materials[material->name] = material;
				return material;
			}
		}
		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::ExtractAnimation(void* a_scene)
	{
		FbxScene* fbxScene = (FbxScene*)a_scene;

		for (int i = 0; i < fbxScene->GetSrcObjectCount<FbxAnimStack>(); i++)
		{
			FbxAnimStack* lAnimStack = fbxScene->GetSrcObject<FbxAnimStack>(i);

			FBXAnimation* anim = new FBXAnimation();
			strncpy(anim->m_name,lAnimStack->GetName(),MAX_PATH);

			std::vector<FBXTrack> tracks;

			// MIGHT ONLY DO ONE!
			int nbAnimLayers = lAnimStack->GetMemberCount(FbxCriteria::ObjectType(FbxAnimLayer::ClassId));
			for (int l = 0; l < nbAnimLayers; l++)
			{
				FbxAnimLayer* lAnimLayer = lAnimStack->GetMember<FbxAnimLayer>(l);
				ExtractAnimationTrack(tracks, lAnimLayer, fbxScene->GetRootNode());
			}

			anim->m_startFrame = 99999999;
			anim->m_endFrame = 0;

			anim->m_trackCount = tracks.size();
			if (anim->m_trackCount > 0)
			{
				anim->m_tracks = new FBXTrack[ anim->m_trackCount ];
				memcpy(anim->m_tracks,tracks.data(),sizeof(FBXTrack) * anim->m_trackCount);

				for ( unsigned int j = 0 ; j < anim->m_trackCount ; ++j )
				{
					for ( unsigned int k = 0 ; k < anim->m_tracks[ j ].m_keyframeCount ; ++k )
					{
						anim->m_startFrame = min(anim->m_startFrame,anim->m_tracks[ j ].m_keyframes[k].m_key);
						anim->m_endFrame = max(anim->m_endFrame,anim->m_tracks[ j ].m_keyframes[k].m_key);
					}
				}
			}

			m_animations[ anim->m_name ] = anim;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::ExtractAnimationTrack(std::vector<FBXTrack>& a_tracks, void* a_layer, void* a_node)
	{
		FbxAnimLayer* fbxAnimLayer = (FbxAnimLayer*)a_layer;
		FbxNode* fbxNode = (FbxNode*)a_node;

		FBXSkeleton* skeleton = m_skeletons[0];

		int boneIndex = -1;

		// find node index in skeleton
		for ( unsigned int i = 0 ; i < skeleton->m_boneCount ; ++i )
		{
			if (strncmp(skeleton->m_nodes[i]->m_name,fbxNode->GetName(),MAX_PATH) == 0)
			{
				boneIndex = i;
				break;
			}
		}

		if (boneIndex >= 0)
		{
			FbxAnimCurve* lAnimCurve = nullptr;
			std::map<int,FbxTime> keyFrameTimes;

			int lKeyCount = 0;
			int     lCount;
			char    lTimeString[256];

			// extract curves
			lAnimCurve = fbxNode->LclTranslation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
			if (lAnimCurve)
			{
				lKeyCount = lAnimCurve->KeyGetCount();

				for (lCount = 0; lCount < lKeyCount; lCount++)
				{
					int key = atoi( lAnimCurve->KeyGetTime(lCount).GetTimeString(lTimeString, FbxUShort(256)) );

					keyFrameTimes[ key ] = lAnimCurve->KeyGetTime(lCount);
				}
			}
			lAnimCurve = fbxNode->LclTranslation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
			if (lAnimCurve)
			{
				lKeyCount = lAnimCurve->KeyGetCount();

				for (lCount = 0; lCount < lKeyCount; lCount++)
				{
					int key = atoi( lAnimCurve->KeyGetTime(lCount).GetTimeString(lTimeString, FbxUShort(256)) );

					keyFrameTimes[ key ] = lAnimCurve->KeyGetTime(lCount);
				}
			}
			lAnimCurve = fbxNode->LclTranslation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
			if (lAnimCurve)
			{
				lKeyCount = lAnimCurve->KeyGetCount();

				for (lCount = 0; lCount < lKeyCount; lCount++)
				{
					int key = atoi( lAnimCurve->KeyGetTime(lCount).GetTimeString(lTimeString, FbxUShort(256)) );

					keyFrameTimes[ key ] = lAnimCurve->KeyGetTime(lCount);
				}
			}

			lAnimCurve = fbxNode->LclRotation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
			if (lAnimCurve)
			{
				lKeyCount = lAnimCurve->KeyGetCount();

				for (lCount = 0; lCount < lKeyCount; lCount++)
				{
					int key = atoi( lAnimCurve->KeyGetTime(lCount).GetTimeString(lTimeString, FbxUShort(256)) );

					keyFrameTimes[ key ] = lAnimCurve->KeyGetTime(lCount);
				}
			}
			lAnimCurve = fbxNode->LclRotation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
			if (lAnimCurve)
			{
				lKeyCount = lAnimCurve->KeyGetCount();

				for (lCount = 0; lCount < lKeyCount; lCount++)
				{
					int key = atoi( lAnimCurve->KeyGetTime(lCount).GetTimeString(lTimeString, FbxUShort(256)) );

					keyFrameTimes[ key ] = lAnimCurve->KeyGetTime(lCount);
				}
			}
			lAnimCurve = fbxNode->LclRotation.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
			if (lAnimCurve)
			{
				lKeyCount = lAnimCurve->KeyGetCount();

				for (lCount = 0; lCount < lKeyCount; lCount++)
				{
					int key = atoi( lAnimCurve->KeyGetTime(lCount).GetTimeString(lTimeString, FbxUShort(256)) );

					keyFrameTimes[ key ] = lAnimCurve->KeyGetTime(lCount);
				}
			}

			lAnimCurve = fbxNode->LclScaling.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
			if (lAnimCurve)
			{
				lKeyCount = lAnimCurve->KeyGetCount();

				for (lCount = 0; lCount < lKeyCount; lCount++)
				{
					int key = atoi( lAnimCurve->KeyGetTime(lCount).GetTimeString(lTimeString, FbxUShort(256)) );

					keyFrameTimes[ key ] = lAnimCurve->KeyGetTime(lCount);
				}
			}    
			lAnimCurve = fbxNode->LclScaling.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
			if (lAnimCurve)
			{
				lKeyCount = lAnimCurve->KeyGetCount();

				for (lCount = 0; lCount < lKeyCount; lCount++)
				{
					int key = atoi( lAnimCurve->KeyGetTime(lCount).GetTimeString(lTimeString, FbxUShort(256)) );

					keyFrameTimes[ key ] = lAnimCurve->KeyGetTime(lCount);
				}
			}
			lAnimCurve = fbxNode->LclScaling.GetCurve(fbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
			if (lAnimCurve)
			{
				lKeyCount = lAnimCurve->KeyGetCount();

				for (lCount = 0; lCount < lKeyCount; lCount++)
				{
					int key = atoi( lAnimCurve->KeyGetTime(lCount).GetTimeString(lTimeString, FbxUShort(256)) );

					keyFrameTimes[ key ] = lAnimCurve->KeyGetTime(lCount);
				}
			}

			// turn into array
			if (keyFrameTimes.size() > 0)
			{
				FBXTrack track;

				track.m_boneIndex = boneIndex;
				track.m_keyframeCount = keyFrameTimes.size();
				track.m_keyframes = new FBXKeyFrame[ track.m_keyframeCount ];

				int index = 0;
				
				
				#ifdef _MSC_VER

				for (std::map<int, FbxTime>::iterator l_Iter = keyFrameTimes.begin(); l_Iter != keyFrameTimes.end(); l_Iter++)
				{
					track.m_keyframes[ index ].m_key = l_Iter._Ptr->_Myval.first;

					// OMG THIS IS STUPIDLY SLOW!!! WTF AUTODESK!!!?!?!
					FbxAMatrix localMatrix = g_Assistor->evaluator->GetNodeLocalTransform(fbxNode, l_Iter._Ptr->_Myval.second);

					FbxQuaternion R = localMatrix.GetQ();
					FbxVector4 T = localMatrix.GetT();
					FbxVector4 S = localMatrix.GetS();

					track.m_keyframes[ index ].m_rotation = vec4((float)R[0],(float)R[1],(float)R[2],(float)R[3]);
					track.m_keyframes[ index ].m_translation = vec4((float)T[0],(float)T[1],(float)T[2],0);
					track.m_keyframes[ index ].m_scale = vec4((float)S[0],(float)S[1],(float)S[2],0);
					++index;
				}
				
				#else
				
				for(auto l_Iter : keyFrameTimes)
				{					
					track.m_keyframes[ index ].m_key = l_Iter.first;

					// OMG THIS IS STUPIDLY SLOW!!! WTF AUTODESK!!!?!?!
					FbxAMatrix localMatrix = g_Assistor->evaluator->GetNodeLocalTransform(fbxNode, l_Iter.second);

					FbxQuaternion R = localMatrix.GetQ();
					FbxVector4 T = localMatrix.GetT();
					FbxVector4 S = localMatrix.GetS();

					track.m_keyframes[ index ].m_rotation = vec4((float)R[0],(float)R[1],(float)R[2],(float)R[3]);
					track.m_keyframes[ index ].m_translation = vec4((float)T[0],(float)T[1],(float)T[2],0);
					track.m_keyframes[ index ].m_scale = vec4((float)S[0],(float)S[1],(float)S[2],0);
					++index;					
				}
				
				
				#endif

				a_tracks.push_back(track);
			}
		}

		for (int i = 0; i < fbxNode->GetChildCount(); ++i)
		{
			ExtractAnimationTrack(a_tracks, fbxAnimLayer, fbxNode->GetChild(i));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::ExtractSkeleton(FBXSkeleton* a_skeleton, void* a_scene)
	{
		FbxScene* fbxScene = (FbxScene*)a_scene;

		int poseCount = fbxScene->GetPoseCount();

		char name[MAX_PATH];

		for (int i = 0; i < poseCount; ++i)
		{
			FbxPose* lPose = fbxScene->GetPose(i);

			for ( int j = 0 ; j < lPose->GetCount() ; ++j )
			{
				strncpy(name,lPose->GetNodeName(j).GetCurrentName(),MAX_PATH);
				
				FbxMatrix lMatrix = lPose->GetMatrix(j);
				FbxMatrix lBindMatrix = lMatrix.Inverse();

				for ( unsigned int k = 0 ; k < a_skeleton->m_boneCount ; ++k )
				{
					if ( strcmp(name, a_skeleton->m_nodes[ k ]->m_name) == 0 )
					{
						FbxVector4 row0 = lBindMatrix.GetRow(0);
						FbxVector4 row1 = lBindMatrix.GetRow(1);
						FbxVector4 row2 = lBindMatrix.GetRow(2);
						FbxVector4 row3 = lBindMatrix.GetRow(3);

						a_skeleton->m_bindPoses[ k ][0] = vec4( (float)row0[0], (float)row0[1], (float)row0[2], (float)row0[3] );
						a_skeleton->m_bindPoses[ k ][1] = vec4( (float)row1[0], (float)row1[1], (float)row1[2], (float)row1[3] );
						a_skeleton->m_bindPoses[ k ][2] = vec4( (float)row2[0], (float)row2[1], (float)row2[2], (float)row2[3] );
						a_skeleton->m_bindPoses[ k ][3] = vec4( (float)row3[0], (float)row3[1], (float)row3[2], (float)row3[3] );
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXSkeleton::Evaluate(const FBXAnimation* a_animation, float a_time, bool a_loop, float a_FPS)
	{
		if (a_animation != nullptr)
		{
			// determine frame we're on
			int iAnimFrames = a_animation->m_endFrame - a_animation->m_startFrame;
			float fAnimDuration = iAnimFrames / a_FPS;

			// get time through frame
			float fFrameTime = 0;
			if (a_loop)
				fFrameTime = Maxf(fmod(a_time,fAnimDuration),0);
			else
				fFrameTime = Minf(Maxf(a_time,0),fAnimDuration);

			unsigned int iFrame = a_animation->m_startFrame + (int)(fFrameTime * a_FPS);
		
			const FBXTrack* track = nullptr;
			const FBXKeyFrame* start = nullptr;
			const FBXKeyFrame* end = nullptr;

			for ( unsigned int i = 0 ; i < a_animation->m_trackCount ; ++i )
			{
				track = &(a_animation->m_tracks[i]);

				start = nullptr;
				end = nullptr;

				// determine the two keyframes we're between
				for ( unsigned int j = 0 ; j < track->m_keyframeCount - 1 ; ++j )
				{
					if (track->m_keyframes[j].m_key <= iFrame &&
						track->m_keyframes[j + 1].m_key >= iFrame)
					{
						// found?
						start = &(track->m_keyframes[j]);
						end = &(track->m_keyframes[j+1]);
						break;
					}
				}

				// interpolate between them
				if (start != nullptr &&
					end != nullptr)
				{
					float fStartTime = (start->m_key - a_animation->m_startFrame) / a_FPS;
					float fEndTime = (end->m_key - a_animation->m_startFrame) / a_FPS;

					float fScale = Maxf(0,Minf(1,(fFrameTime - fStartTime) / (fEndTime - fStartTime)));

					// translation
					vec4 T = start->m_translation * (1 - fScale) + end->m_translation * fScale;
				
					// scale
					vec4 S = start->m_scale * (1 - fScale) + end->m_scale * fScale;
				
					// rotation (quaternion slerp)
					FbxQuaternion qStart(start->m_rotation.x,start->m_rotation.y,start->m_rotation.z,start->m_rotation.w);
					FbxQuaternion qEnd(end->m_rotation.x,end->m_rotation.y,end->m_rotation.z,end->m_rotation.w);
					FbxQuaternion qRot;

					double cosHalfTheta = qStart[3] * qEnd[3] + qStart[0] * qEnd[0] + qStart[1] * qEnd[1] + qStart[2] * qEnd[2];

					if (abs(cosHalfTheta) >= 1.0)
					{
						qRot[3] = qStart[3];
						qRot[0] = qStart[0];
						qRot[1] = qStart[1];
						qRot[2] = qStart[2];
					}
					else
					{
						double halfTheta = acos(cosHalfTheta);
						double sinHalfTheta = std::sqrt(1.0 - cosHalfTheta*cosHalfTheta);
						// if theta = 180 degrees then result is not fully defined
						// we could rotate around any axis normal to qa or qb
						if (fabs(sinHalfTheta) < 0.001)
						{
							qRot[3] = (qStart[3] * 0.5 + qEnd[3] * 0.5);
							qRot[0] = (qStart[0] * 0.5 + qEnd[0] * 0.5);
							qRot[1] = (qStart[1] * 0.5 + qEnd[1] * 0.5);
							qRot[2] = (qStart[2] * 0.5 + qEnd[2] * 0.5);
						}
						else
						{
							double ratioA = sin((1 - fScale) * halfTheta) / sinHalfTheta;
							double ratioB = sin(fScale * halfTheta) / sinHalfTheta; 

							qRot[3] = (qStart[3] * ratioA + qEnd[3] * ratioB);
							qRot[0] = (qStart[0] * ratioA + qEnd[0] * ratioB);
							qRot[1] = (qStart[1] * ratioA + qEnd[1] * ratioB);
							qRot[2] = (qStart[2] * ratioA + qEnd[2] * ratioB);
						}
					}

					qRot.Normalize();

					// build matrix and set as local
					FbxAMatrix m;
					m.SetTQS( FbxVector4(T.x,T.y,T.z), qRot, FbxVector4(S.x,S.y,S.z) );

					FbxVector4 row0 = m.GetRow(0);
					FbxVector4 row1 = m.GetRow(1);
					FbxVector4 row2 = m.GetRow(2);
					FbxVector4 row3 = m.GetRow(3);
				
					m_nodes[ track->m_boneIndex ]->m_localTransform[0] = vec4( (float)row0[0], (float)row0[1], (float)row0[2], (float)row0[3] );
					m_nodes[ track->m_boneIndex ]->m_localTransform[1] = vec4( (float)row1[0], (float)row1[1], (float)row1[2], (float)row1[3] );
					m_nodes[ track->m_boneIndex ]->m_localTransform[2]= vec4( (float)row2[0], (float)row2[1], (float)row2[2], (float)row2[3] );
					m_nodes[ track->m_boneIndex ]->m_localTransform[3] = vec4( (float)row3[0], (float)row3[1], (float)row3[2], (float)row3[3] );
				
					// update global array
					if (m_nodes[ track->m_boneIndex ]->m_parent != nullptr)
						m_nodes[ track->m_boneIndex ]->m_globalTransform = m_nodes[ track->m_boneIndex ]->m_localTransform * m_nodes[ track->m_boneIndex ]->m_parent->m_globalTransform;
					else
						m_nodes[ track->m_boneIndex ]->m_globalTransform = m_nodes[ track->m_boneIndex ]->m_localTransform;
				}
			}
		}

		// update bones, removing the Z axis fix as well
		static mat4 M(1,0,0,0,0,1,0,0,0,0,-1,0,0,0,0,1);
		for ( unsigned int i = 0 ; i < m_boneCount ; ++i )
			m_bones[ i ] = m_bindPoses[ i ] * m_nodes[ i ]->m_globalTransform * M;
	}

	//////////////////////////////////////////////////////////////////////////
	// SLOWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
	// TODO: Find a faster way to only add unique verts :(
	unsigned int FBXScene::AddVertGetIndex(std::vector<FBXVertex>& a_vertices, const FBXVertex& a_vertex)
	{
		auto oIter = std::find(a_vertices.begin(),a_vertices.end(),a_vertex);
		if (oIter != a_vertices.end())
		{
			return oIter - a_vertices.begin();
		}
		a_vertices.push_back(a_vertex);
		return a_vertices.size() - 1;
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::CalculateTangentsBinormals(std::vector<FBXVertex>& a_vertices, const std::vector<unsigned int>& a_indices)
	{
		unsigned int vertexCount = a_vertices.size();
		vec4* tan1 = new vec4[vertexCount * 2];
		vec4* tan2 = tan1 + vertexCount;
		memset(tan1, 0, vertexCount * sizeof(vec4) * 2);

		unsigned int indexCount = a_indices.size();
		for (unsigned int a = 0; a < indexCount; a += 3)
		{
			long i1 = a_indices[a];
			long i2 = a_indices[a + 1];
			long i3 = a_indices[a + 2];

			const vec4& v1 = a_vertices[i1].position;
			const vec4& v2 = a_vertices[i2].position;
			const vec4& v3 = a_vertices[i3].position;

			const vec2& w1 = a_vertices[i1].uv;
			const vec2& w2 = a_vertices[i2].uv;
			const vec2& w3 = a_vertices[i3].uv;

			float x1 = v2.x - v1.x;
			float x2 = v3.x - v1.x;
			float y1 = v2.y - v1.y;
			float y2 = v3.y - v1.y;
			float z1 = v2.z - v1.z;
			float z2 = v3.z - v1.z;

			float s1 = w2.x - w1.x;
			float s2 = w3.x - w1.x;
			float t1 = w2.y - w1.y;
			float t2 = w3.y - w1.y;

			float r = 1.0F / (s1 * t2 - s2 * t1);
			vec4 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
				(t2 * z1 - t1 * z2) * r,0);
			vec4 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
				(s1 * z2 - s2 * z1) * r,0);

			tan1[i1] += sdir;
			tan1[i2] += sdir;
			tan1[i3] += sdir;

			tan2[i1] += tdir;
			tan2[i2] += tdir;
			tan2[i3] += tdir;
		}

		for (unsigned int a = 0; a < vertexCount; a++)
		{
			const vec4& n = a_vertices[a].normal;
			const vec4& t = tan1[a];

			// Gram-Schmidt orthogonalize
			a_vertices[a].tangent = (t - n * glm::dot(n, t));
			glm::normalize(a_vertices[a].tangent);

			// Calculate handedness
			a_vertices[a].tangent.w = (dot(cross(vec3(n), vec3(t)), vec3(tan2[a])) < 0.0F) ? -1.0F : 1.0F;

			// calculate binormal
			a_vertices[a].binormal = vec4(cross(vec3(a_vertices[a].normal.x, a_vertices[a].normal.y, a_vertices[a].normal.z),
			 vec3(a_vertices[a].tangent.x, a_vertices[a].tangent.y, a_vertices[a].tangent.z)), a_vertices[a].tangent.w);
		}

		delete[] tan1;
	}

	//////////////////////////////////////////////////////////////////////////
	unsigned int FBXScene::NodeCount(Node* a_node)
	{
		unsigned int uiCount = 1;

		for (unsigned int i = 0; i < a_node->m_children.size(); i++)
			uiCount += NodeCount(a_node->m_children[i]);

		return uiCount;
	}

	//////////////////////////////////////////////////////////////////////////
	bool FBXScene::SaveAIE(const char* a_filename)
	{
		FILE* pFile = fopen(a_filename,"wb");

		unsigned int i = 0, j = 0;
		unsigned int uiAddress = 0;

		// ambient light
		fwrite(&m_ambientLight,sizeof(vec4),1,pFile);

		// material count
		unsigned int uiCount = m_materials.size();
		fwrite(&uiCount,sizeof(unsigned int),1,pFile);


		#ifdef _MSC_VER		
		
		// for each material
		for (std::map<std::string, FBXMaterial*>::iterator l_Iter = m_materials.begin(); l_Iter != m_materials.end(); l_Iter++)
		{
			uiAddress = (unsigned int)l_Iter._Ptr->_Myval.second;

			// write pointer address
			fwrite(&uiAddress,sizeof(unsigned int),1,pFile);
			// write material data
			fwrite(l_Iter._Ptr->_Myval.second,sizeof(FBXMaterial),1,pFile);
		}
		
		#else
		
		for(auto l_Iter : m_materials)
		{
			uiAddress = (unsigned int)l_Iter.second;

			// write pointer address
			fwrite(&uiAddress,sizeof(unsigned int),1,pFile);
			// write material data
			fwrite(l_Iter.second,sizeof(FBXMaterial),1,pFile);			
		}	
		
		#endif

		// write node count
		uiCount = NodeCount(m_root);
		fwrite(&uiCount,sizeof(unsigned int),1,pFile);
		SaveNode(m_root, pFile);

		// write skeleton count
		uiCount = m_skeletons.size();
		fwrite(&uiCount,sizeof(unsigned int),1,pFile);

		// write skeleton data (nodes as pointer addresses)
		for (unsigned int i = 0; i < m_skeletons.size(); i++)
		{
			fwrite(&m_skeletons[i]->m_boneCount,sizeof(unsigned int),1,pFile);
			fwrite(m_skeletons[i]->m_bindPoses,sizeof(mat4),m_skeletons[i]->m_boneCount,pFile);
			fwrite(m_skeletons[i]->m_nodes,sizeof(Node*),m_skeletons[i]->m_boneCount,pFile);
		}

		// animation count
		uiCount = m_animations.size();
		fwrite(&uiCount,sizeof(unsigned int),1,pFile);
		
		#ifdef _MSC_VER

		for (std::map<std::string, FBXAnimation*>::iterator l_Iter = m_animations.begin(); l_Iter != m_animations.end(); l_Iter++)
		{
			// write anim data, then each track in order
			fwrite(l_Iter._Ptr->_Myval.second->m_name,sizeof(char),MAX_PATH,pFile);
			fwrite(&l_Iter._Ptr->_Myval.second->m_startFrame,sizeof(unsigned int),1,pFile);
			fwrite(&l_Iter._Ptr->_Myval.second->m_endFrame,sizeof(unsigned int),1,pFile);
			fwrite(&l_Iter._Ptr->_Myval.second->m_trackCount,sizeof(unsigned int),1,pFile);

			// each track writes keyframes in order
			for ( i = 0 ; i < l_Iter._Ptr->_Myval.second->m_trackCount ; ++i )
			{
				fwrite(&l_Iter._Ptr->_Myval.second->m_tracks[i].m_boneIndex,sizeof(unsigned int),1,pFile);
				fwrite(&l_Iter._Ptr->_Myval.second->m_tracks[i].m_keyframeCount,sizeof(unsigned int),1,pFile);
				fwrite(l_Iter._Ptr->_Myval.second->m_tracks[i].m_keyframes,sizeof(FBXKeyFrame),l_Iter._Ptr->_Myval.second->m_tracks[i].m_keyframeCount,pFile);				
			}
		}
		
		#else
		for(auto l_Iter : m_animations)
		{
			// write anim data, then each track in order
			fwrite(l_Iter.second->m_name,sizeof(char),MAX_PATH,pFile);
			fwrite(&l_Iter.second->m_startFrame,sizeof(unsigned int),1,pFile);
			fwrite(&l_Iter.second->m_endFrame,sizeof(unsigned int),1,pFile);
			fwrite(&l_Iter.second->m_trackCount,sizeof(unsigned int),1,pFile);

			// each track writes keyframes in order
			for ( i = 0 ; i < l_Iter.second->m_trackCount ; ++i )
			{
				fwrite(&l_Iter.second->m_tracks[i].m_boneIndex,sizeof(unsigned int),1,pFile);
				fwrite(&l_Iter.second->m_tracks[i].m_keyframeCount,sizeof(unsigned int),1,pFile);
				fwrite(l_Iter.second->m_tracks[i].m_keyframes,sizeof(FBXKeyFrame),l_Iter.second->m_tracks[i].m_keyframeCount,pFile);				
			}			
		}		
		
		#endif

		fclose(pFile);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::SaveNode(Node* a_node, FILE* a_file)
	{
		// write pointer address
		unsigned int uiAddress = (unsigned int)a_node;
		fwrite(&uiAddress,sizeof(unsigned int),1,a_file);

		// write type
		fwrite(&a_node->m_nodeType,sizeof(unsigned int),1,a_file);

		// write name
		fwrite(a_node->m_name,sizeof(char),MAX_PATH,a_file);

		uiAddress = (unsigned int)a_node->m_parent;
		fwrite(&uiAddress,sizeof(unsigned int),1,a_file);

		// save transforms
		fwrite(&a_node->m_localTransform,sizeof(mat4),1,a_file);
		fwrite(&a_node->m_globalTransform,sizeof(mat4),1,a_file);

		// write type specific data
		switch (a_node->m_nodeType)
		{
		case Node::MESH:	SaveMeshData((FBXMeshNode*)a_node,a_file);		break;
		case Node::LIGHT:	SaveLightData((FBXLightNode*)a_node,a_file);	break;
		case Node::CAMERA:	SaveCameraData((FBXCameraNode*)a_node,a_file);	break;
		default:	break;
		};

		// write child count
		unsigned int uiCount = a_node->m_children.size();
		fwrite(&uiCount,sizeof(unsigned int),1,a_file);

		// write child pointer addresses
		for (unsigned int i = 0; i <a_node->m_children.size(); i++)
		{
			uiAddress = (unsigned int)a_node->m_children[i];
			fwrite(&uiAddress,sizeof(unsigned int),1,a_file);
		}

		// write child nodes
		for (unsigned int i = 0; i <a_node->m_children.size(); i++)
		{
			SaveNode(a_node->m_children[i],a_file);
		}
	}

	void FBXScene::SaveMeshData(FBXMeshNode* a_mesh, FILE* a_file)
	{
		// write material address
		unsigned int uiAddress = (unsigned int)a_mesh->m_material;
		fwrite(&uiAddress,sizeof(unsigned int),1,a_file);

		// write vertex count
		unsigned int uiCount = a_mesh->m_vertices.size();
		fwrite(&uiCount,sizeof(unsigned int),1,a_file);

		// write vertices
		fwrite(a_mesh->m_vertices.data(),sizeof(FBXVertex),uiCount,a_file);

		// write index count
		uiCount = a_mesh->m_indices.size();
		fwrite(&uiCount,sizeof(unsigned int),1,a_file);

		// write indices
		fwrite(a_mesh->m_indices.data(),sizeof(unsigned int),uiCount,a_file);
	}

	void FBXScene::SaveLightData(FBXLightNode* a_light, FILE* a_file)
	{
		// save light type
		unsigned int uiValue = a_light->m_type;
		fwrite(&uiValue,sizeof(unsigned int),1,a_file);

		// save if on / off (as unsigned int for packing)
		uiValue = a_light->m_on ? 1 : 0;
		fwrite(&uiValue,sizeof(unsigned int),1,a_file);

		// save colour
		fwrite(&a_light->m_colour,sizeof(vec4),1,a_file);

		// save both angles
		fwrite(&a_light->m_innerAngle,sizeof(float),1,a_file);
		fwrite(&a_light->m_outerAngle,sizeof(float),1,a_file);

		// save attenuation
		fwrite(&a_light->m_attenuation,sizeof(vec4),1,a_file);
	}

	void FBXScene::SaveCameraData(FBXCameraNode* a_camera, FILE* a_file)
	{
		// save aspect, FOV, near, far
		fwrite(&a_camera->m_aspectRatio,sizeof(float),1,a_file);
		fwrite(&a_camera->m_fieldOfView,sizeof(float),1,a_file);
		fwrite(&a_camera->m_near,sizeof(float),1,a_file);
		fwrite(&a_camera->m_far,sizeof(float),1,a_file);

		// save view matrix
		fwrite(&a_camera->m_viewMatrix,sizeof(mat4),1,a_file);
	}

	//////////////////////////////////////////////////////////////////////////
	bool FBXScene::LoadAIE(const char* a_filename)
	{
		FILE* pFile = fopen(a_filename,"rb");

		unsigned int i = 0, j = 0;
		unsigned int uiAddress = 0, type = Node::NODE;

		// ambient light
		fread(&m_ambientLight,sizeof(vec4),1,pFile);

		// material count
		unsigned int uiCount = 0;
		fread(&uiCount,sizeof(unsigned int),1,pFile);

		std::map<unsigned int, FBXMaterial*> materials;
		std::map<unsigned int, Node*> nodes;

		// for each material
		for ( i = 0 ; i < uiCount ; ++i )
		{
			FBXMaterial* m = new FBXMaterial();

			// write pointer address
			fread(&uiAddress,sizeof(unsigned int),1,pFile);

			// write material data
			fread(m,sizeof(FBXMaterial),1,pFile);

			materials[ uiAddress ] = m;
			m_materials[ m->name ] = m;
		}

		// node count
		fread(&uiCount,sizeof(unsigned int),1,pFile);
		if (uiCount > 0)
		{
			LoadNode(nodes, materials, pFile);
			ReLink(m_root,nodes);
		}

		// read skeleton count
		fread(&uiCount,sizeof(unsigned int),1,pFile);

		// read skeleton data (nodes as pointer addresses)
		for ( i = 0 ; i < uiCount ; ++i )
		{
			FBXSkeleton* s = new FBXSkeleton();

			fread(&s->m_boneCount,sizeof(unsigned int),1,pFile);

			s->m_bindPoses = new mat4[ s->m_boneCount ];
			s->m_bones = new mat4[ s->m_boneCount ];
			s->m_nodes = new Node * [ s->m_boneCount ];

			fread(s->m_bindPoses,sizeof(mat4),s->m_boneCount,pFile);
			fread(s->m_nodes,sizeof(Node*),s->m_boneCount,pFile);

			// relink nodes
			for ( j = 0 ; j < s->m_boneCount ; ++j )
			{
				s->m_nodes[j] = nodes[ (unsigned int)s->m_nodes[j] ];
			}

			m_skeletons.push_back(s);
		}

		// read animation count
		fread(&uiCount,sizeof(unsigned int),1,pFile);

		for ( i = 0 ; i < uiCount ; ++i )
		{
			FBXAnimation* anim = new FBXAnimation();

			// read anim data, then each track in order
			fread(anim->m_name,sizeof(char),MAX_PATH,pFile);
			fread(&anim->m_startFrame,sizeof(unsigned int),1,pFile);
			fread(&anim->m_endFrame,sizeof(unsigned int),1,pFile);
			fread(&anim->m_trackCount,sizeof(unsigned int),1,pFile);

			anim->m_tracks = new FBXTrack[ anim->m_trackCount ];

			// each track writes keyframes in order
			for ( j = 0 ; j < anim->m_trackCount ; ++j )
			{
				fread(&anim->m_tracks[j].m_boneIndex,sizeof(unsigned int),1,pFile);
				fread(&anim->m_tracks[j].m_keyframeCount,sizeof(unsigned int),1,pFile);

				anim->m_tracks[j].m_keyframes = new FBXKeyFrame[ anim->m_tracks[j].m_keyframeCount ];
				fread(anim->m_tracks[j].m_keyframes,sizeof(FBXKeyFrame),anim->m_tracks[j].m_keyframeCount,pFile);				
			}

			m_animations[ anim->m_name ] = anim;
		}

		fclose(pFile);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void FBXScene::LoadNode(std::map<unsigned int,Node*>& nodes, 
		std::map<unsigned int, FBXMaterial*>& materials, FILE* a_file)
	{
		unsigned int uiAddress = 0, uiParent = 0, uiType = Node::NODE;

		// read address and type for node
		fread(&uiAddress,sizeof(unsigned int),1,a_file);
		fread(&uiType,sizeof(unsigned int),1,a_file);

		// read name
		char name[MAX_PATH];
		fread(name,sizeof(char),MAX_PATH,a_file);

		fread(&uiParent,sizeof(unsigned int),1,a_file);

		mat4 mLocal(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1), mGlobal(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
		// save transforms
		fread(&mLocal,sizeof(mat4),1,a_file);
		fread(&mGlobal,sizeof(mat4),1,a_file);

		Node* pNode = nullptr;

		switch (uiType)
		{
		case Node::NODE:
			{
				pNode = new Node();
				break;
			}
		case Node::MESH:
			{
				pNode = new FBXMeshNode();
				LoadMeshData((FBXMeshNode*)pNode,materials,a_file);
				break;
			}
		case Node::LIGHT:
			{
				pNode = new FBXLightNode();
				LoadLightData((FBXLightNode*)pNode,a_file);
				break;
			}
		case Node::CAMERA:
			{
				pNode = new FBXCameraNode();
				LoadCameraData((FBXCameraNode*)pNode,a_file);
				break;
			}
		default:	break;
		};

		pNode->m_localTransform = mLocal;
		pNode->m_globalTransform = mGlobal;

		nodes[ uiAddress ] = pNode;

		pNode->m_parent = (Node*)uiParent;

		if (m_root == nullptr)
			m_root = pNode;

		pNode->m_nodeType = (Node::NodeType)uiType;
		strncpy(pNode->m_name,name,MAX_PATH);

		switch (uiType)
		{
		case Node::MESH:	m_meshes[ pNode->m_name ] = (FBXMeshNode*)pNode;	break;
		case Node::LIGHT:	m_lights[ pNode->m_name ] = (FBXLightNode*)pNode;	break;
		case Node::CAMERA:	m_cameras[ pNode->m_name ] = (FBXCameraNode*)pNode;	break;
		default:	break;
		};

		// write child count
		unsigned int uiCount = 0;
		fread(&uiCount,sizeof(unsigned int),1,a_file);

		// write child pointer addresses
		for ( unsigned int i = 0 ; i < uiCount ; ++i )
		{
			fread(&uiAddress,sizeof(unsigned int),1,a_file);
			pNode->m_children.push_back((Node*)uiAddress);
		}

		// read child nodes
		for ( unsigned int i = 0 ; i < uiCount ; ++i )
		{
			LoadNode(nodes,materials,a_file);
		}
	}

	void FBXScene::LoadMeshData(FBXMeshNode* a_mesh, std::map<unsigned int, FBXMaterial*>& materials, FILE* a_file)
	{
		// read material address
		unsigned int uiAddress = 0;
		fread(&uiAddress,sizeof(unsigned int),1,a_file);
		a_mesh->m_material = materials[ uiAddress ];

		// read vertex count
		unsigned int uiCount = 0;
		fread(&uiCount,sizeof(unsigned int),1,a_file);

		// read vertices
		if (uiCount > 0)
		{
			FBXVertex* vertices = new FBXVertex[ uiCount ];
			fread(vertices,sizeof(FBXVertex),uiCount,a_file);
			for ( unsigned int i = 0 ; i < uiCount ; ++i )
				a_mesh->m_vertices.push_back( vertices[i] );
			delete[] vertices;
		}
		
		// read index count
		uiCount = 0;
		fread(&uiCount,sizeof(unsigned int),1,a_file);

		// read indices
		if (uiCount > 0)
		{
			unsigned int* indices = new unsigned int[ uiCount ];
			fread(indices,sizeof(unsigned int),uiCount,a_file);
			for ( unsigned int i = 0 ; i < uiCount ; ++i )
				a_mesh->m_indices.push_back( indices[i] );
			delete[] indices;
		}
	}

	void FBXScene::LoadLightData(FBXLightNode* a_light, FILE* a_file)
	{
		// save light type
		unsigned int uiValue = 0;
		fread(&uiValue,sizeof(unsigned int),1,a_file);
		a_light->m_type = (FBXLightNode::LightType)uiValue;

		// save if on / off (as unsigned int for packing)
		uiValue = 0;
		fread(&uiValue,sizeof(unsigned int),1,a_file);
		a_light->m_on = uiValue != 0;

		// save colour
		fread(&a_light->m_colour,sizeof(vec4),1,a_file);

		// save both angles
		fread(&a_light->m_innerAngle,sizeof(float),1,a_file);
		fread(&a_light->m_outerAngle,sizeof(float),1,a_file);

		// save attenuation
		fread(&a_light->m_attenuation,sizeof(vec4),1,a_file);
	}

	void FBXScene::LoadCameraData(FBXCameraNode* a_camera, FILE* a_file)
	{
		// read aspect, FOV, near, far
		fread(&a_camera->m_aspectRatio,sizeof(float),1,a_file);
		fread(&a_camera->m_fieldOfView,sizeof(float),1,a_file);
		fread(&a_camera->m_near,sizeof(float),1,a_file);
		fread(&a_camera->m_far,sizeof(float),1,a_file);

		// read view matrix
		fread(&a_camera->m_viewMatrix,sizeof(mat4),1,a_file);
	}

	void FBXScene::ReLink(Node* a_node, std::map<unsigned int, Node*>& nodes)
	{
		if (a_node->m_parent != nullptr)
		{
			a_node->m_parent = nodes[ (unsigned int)a_node->m_parent ];
		}
		unsigned int uiChildCount = a_node->m_children.size();
		for ( unsigned int i = 0 ; i < uiChildCount ; ++i )
		{
			a_node->m_children[ i ] = nodes[ (unsigned int)a_node->m_children[i] ];
			ReLink(a_node->m_children[ i ],nodes);
		}
	}
