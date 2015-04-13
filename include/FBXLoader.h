//////////////////////////////////////////////////////////////////////////
// Author:	Conan Bourke
// Date:	January 5 2013
// Brief:	Classes to load an FBX scene for use.
//			All angles are in radians.
//////////////////////////////////////////////////////////////////////////
#ifndef __FBXLOADER_H_
#define __FBXLOADER_H_
//////////////////////////////////////////////////////////////////////////

#include <map>
#include <vector>
#include <string>
#include "Paths.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Utilities.h"



/*
// warning on using templates within a DLL
#pragma warning( disable : 4251 )
// warning on not using microsoft "safe" functions
#pragma warning( disable : 4996 )

// DLL declaration for import/export
#ifdef AIE_DLL_EXPORT
	#define AIE_DLL __declspec(dllexport)
#else
	#define AIE_DLL __declspec(dllimport)
#endif // AIE_DLL_EXPORT

*/

#define MAX_PATH 260
#pragma warning( disable : 4049 )
#pragma warning( disable : 4996 )
//////////////////////////////////////////////////////////////////////////

	// A complete vertex structure with all the data needed from the FBX file
	struct FBXVertex
	{
		FBXVertex() : position(0,0,0,1), colour(1,1,1,1), normal(0,0,0,0), 
			tangent(0,0,0,0), binormal(0,0,0,0), indices(0,0,0,0), 
			weights(0,0,0,0), uv(0,0), uv2(0,0) {}

		enum Offsets
		{
			PositionOffset	= 0,
			ColourOffset	= PositionOffset + sizeof(glm::vec4),
			NormalOffset	= ColourOffset + sizeof(glm::vec4),
			TangentOffset	= NormalOffset + sizeof(glm::vec4),
			BiNormalOffset	= TangentOffset + sizeof(glm::vec4),
			IndicesOffset	= BiNormalOffset + sizeof(glm::vec4),
			WeightsOffset	= IndicesOffset + sizeof(glm::vec4),
			UVOffset	= WeightsOffset + sizeof(glm::vec4),			
		};

		glm::vec4	position;
		glm::vec4	colour;
		glm::vec4	normal;
		glm::vec4	tangent;
		glm::vec4	binormal;
		glm::vec4	indices;
		glm::vec4	weights;
		glm::vec2	uv;
		glm::vec2	uv2;

		// don't touch!
		int fbxControlPointIndex;

	bool operator == (const FBXVertex& a_Vertex)
	{
		if(a_Vertex.position == position &&
		a_Vertex.colour == colour &&
		a_Vertex.normal == normal &&
		a_Vertex.tangent == tangent &&
		a_Vertex.binormal == binormal &&
		a_Vertex.indices == indices &&
		a_Vertex.weights == weights &&
		a_Vertex.uv == uv &&
		a_Vertex.uv2 == uv2)
		{
			return true;
		}

		else
		{
	
			return false;
		}

	}

};

	// A simple FBX material that supports 8 texture channels
	struct FBXMaterial
	{
		enum TextureTypes
		{
			DiffuseTexture	= 0,
			AmbientTexture,
			GlowTexture,
			SpecularTexture,
			GlossTexture,
			NormalTexture,
			AlphaTexture,
			DisplacementTexture,

			TextureTypes_Count
		};

		FBXMaterial() : ambient(0,0,0,0), diffuse(1,1,1,1), specular(1,1,1,1), emissive(0,0,0,0)
		{
			memset(name,0,MAX_PATH);
			memset(textureFilenames,0,TextureTypes_Count*MAX_PATH);
			memset(textureIDs,0,TextureTypes_Count * sizeof(unsigned int));
		}

		char			name[MAX_PATH];
		glm::vec4			ambient;	// RGB + Ambient Factor stored in A
		glm::vec4			diffuse;	// RGBA
		glm::vec4			specular;	// RGB + Shininess/Gloss stored in A
		glm::vec4			emissive;	// RGB + Emissive Factor stored in A

		char			textureFilenames[TextureTypes_Count][MAX_PATH];

		// GL texture IDs all 0 until textures are assigned by the application
		unsigned int	textureIDs[TextureTypes_Count];
	};

	// Simple tree node with local/global transforms and children
	// Also has a void* user data that the application can make use of
	class Node
	{
	public:

		enum NodeType : unsigned int
		{
			NODE = 0,
			MESH,
			LIGHT,
			CAMERA
		};

		Node() : m_nodeType(NODE), m_localTransform(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1), 
			m_globalTransform(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1), m_parent(nullptr), 
			m_userData(nullptr)
		{ 
			m_name[0] = 0;
		}
		virtual ~Node()
		{
			for (std::vector<Node*>::iterator l_Iter = m_children.begin();  l_Iter != m_children.end(); l_Iter++) 
				delete *l_Iter;
		}

		NodeType			m_nodeType;
		char				m_name[MAX_PATH];

		glm::mat4				m_localTransform;
		glm::mat4				m_globalTransform;

		Node*				m_parent;
		std::vector<Node*>	m_children;

		void*				m_userData;
	};

	// A simple mesh node that contains an array of vertices and indices used
	// to represent a triangle mesh.
	// Also points to a shared material
	class FBXMeshNode : public Node
	{
	public:

		FBXMeshNode() : m_material(nullptr) { m_nodeType = MESH; }
		virtual ~FBXMeshNode() {}

		FBXMaterial*				m_material;
		std::vector<FBXVertex>		m_vertices;
		std::vector<unsigned int>	m_indices;
	};

	// A light node that can represent a point, directional, or spot light
	class FBXLightNode : public Node
	{
	public:

		FBXLightNode() { m_nodeType = LIGHT; }
		virtual ~FBXLightNode() {}

		enum LightType : unsigned int
		{
			Point	=	0,
			Directional,
			Spot,
		};

		LightType	m_type;

		bool		m_on;

		glm::vec4		m_colour;	// RGB + Light Intensity stored in A

		float		m_innerAngle;	// spotlight inner cone angle
		float		m_outerAngle;	// spotlight outer cone angle

		glm::vec4		m_attenuation;	// (constant,linear,quadratic,0)
	};

	// A camera node with information to create projection matrix
	class FBXCameraNode : public Node
	{
	public:

		FBXCameraNode() { m_nodeType = CAMERA; }
		virtual ~FBXCameraNode() {}

		float	m_aspectRatio;	// if 0 then ratio based off screen resolution
		float	m_fieldOfView; // if 0 then orthographic rather than perspective
		float	m_near;
		float	m_far;
		glm::mat4	m_viewMatrix;
	};

	//////////////////////////////////////////////////////////////////////////
	struct FBXKeyFrame
	{
		FBXKeyFrame() : m_key(0), m_rotation(0,0,0,1), m_translation(0,0,0,1), m_scale(1,1,1,0) {}

		unsigned int	m_key;
		glm::vec4			m_rotation;
		glm::vec4			m_translation;
		glm::vec4			m_scale;
	};

	struct FBXTrack
	{
		FBXTrack() : m_boneIndex(0), m_keyframeCount(0), m_keyframes(nullptr) {}
		~FBXTrack() {  }

		unsigned int	m_boneIndex;
		unsigned int	m_keyframeCount;
		FBXKeyFrame*	m_keyframes;
	};

	struct FBXAnimation
	{
		FBXAnimation() : m_trackCount(0), m_tracks(nullptr), m_startFrame(0), m_endFrame(0) {}
		~FBXAnimation() { }

		unsigned int	TotalFrames() const	{	return m_endFrame - m_startFrame;	}
		float			TotalTime(float a_fps = 24.0f) const {	return (m_endFrame - m_startFrame) / a_fps; }

		char			m_name[MAX_PATH];
		unsigned int	m_trackCount;
		FBXTrack*		m_tracks;
		unsigned int	m_startFrame;
		unsigned int	m_endFrame;
	};

	class FBXSkeleton
	{
	public:

		FBXSkeleton() : m_boneCount(0), m_nodes(nullptr), m_bones(nullptr), m_bindPoses(nullptr), m_userData(nullptr) {}
		~FBXSkeleton() 
		{
			delete[] m_nodes;
			delete[] m_bones;
			delete[] m_bindPoses;
		}

		void			Evaluate(const FBXAnimation* a_animation, float a_time, bool a_loop = true, float a_fps = 24.0f);

		unsigned int	m_boneCount;
		Node**			m_nodes;

		glm::mat4*			m_bones;	// ready for use in skinning! (bind pose combined)
		glm::mat4*			m_bindPoses;

		void*			m_userData;
	};

	// An FBX scene representing the contents on an FBX file.
	// Stores individual items within maps, with names as the key.
	// Also has a pointer to the root of the scene's node tree.
	class FBXScene
	{
	public:

		FBXScene() : m_root(nullptr) {}
		~FBXScene() 
		{
			Unload();
		}

		// must unload a scene before loading a new one over top
		bool			Load(const char* a_filename);
		void			Unload();

		// save/load from binary format that does not need to be parsed
		bool			SaveAIE(const char* a_filename);
		bool			LoadAIE(const char* a_filename);

		// the folder path of the FBX file
		// useful for accessing texture locations
		const char*		GetPath() const				{	return m_path.c_str();	}

		// the scene arranged in a tree graph
		Node*			GetRoot() const				{	return m_root;			}

		// the ambient light of the scene
		const glm::vec4&		GetAmbientLight() const		{	return m_ambientLight;	}

		unsigned int	GetMeshCount() const		{	return m_meshes.size();		}
		unsigned int	GetLightCount() const		{	return m_lights.size();		}
		unsigned int	GetCameraCount() const		{	return m_cameras.size();	}
		unsigned int	GetMaterialCount() const	{	return m_materials.size();	}
		unsigned int	GetSkeletonCount() const	{	return m_skeletons.size();	}
		unsigned int	GetAnimationCount() const	{	return m_animations.size();	}

		FBXMeshNode*	GetMeshByName(const char* a_name);
		FBXLightNode*	GetLightByName(const char* a_name);
		FBXCameraNode*	GetCameraByName(const char* a_name);
		FBXMaterial*	GetMaterialByName(const char* a_name);
		FBXAnimation*	GetAnimationByName(const char* a_name);

		// these methods are slow as the items are stored in a map
		FBXMeshNode*	GetMeshByIndex(unsigned int a_index);
		FBXLightNode*	GetLightByIndex(unsigned int a_index);
		FBXCameraNode*	GetCameraByIndex(unsigned int a_index);
		FBXMaterial*	GetMaterialByIndex(unsigned int a_index);
		FBXSkeleton*	GetSkeletonByIndex(unsigned int a_index)	{	return m_skeletons[a_index];	}
		FBXAnimation*	GetAnimationByIndex(unsigned int a_index);

	private:

		void	ExtractObject(Node* a_parent, void* a_object);
		void	ExtractMesh(FBXMeshNode* a_mesh, void* a_object);
		void	ExtractLight(FBXLightNode* a_light, void* a_object);
		void	ExtractCamera(FBXCameraNode* a_camera, void* a_object);

		void	GatherBones(void* a_object);
		void	ExtractSkeleton(FBXSkeleton* a_skeleton, void* a_scene);

		void	ExtractAnimation(void* a_scene);
		void	ExtractAnimationTrack(std::vector<FBXTrack>& a_tracks, void* a_layer, void* a_node);

		void	ExtractSkin(FBXMeshNode* a_mesh, void* a_node);
		
		FBXMaterial*	ExtractMaterial(void* a_mesh);

		// helpers used for building meshes
		unsigned int AddVertGetIndex(std::vector<FBXVertex>& a_vertices, const FBXVertex& a_vertex);
		void CalculateTangentsBinormals(std::vector<FBXVertex>& a_vertices, const std::vector<unsigned int>& a_indices);

		void	SaveNode(Node* a_node, FILE* a_file);
		void	SaveMeshData(FBXMeshNode* a_mesh, FILE* a_file);
		void	SaveLightData(FBXLightNode* a_light, FILE* a_file);
		void	SaveCameraData(FBXCameraNode* a_camera, FILE* a_file);

		void	LoadNode(std::map<unsigned int,Node*>& nodes, std::map<unsigned int, FBXMaterial*>& materials, FILE* a_file);
		void	LoadMeshData(FBXMeshNode* a_mesh, std::map<unsigned int, FBXMaterial*>& materials, FILE* a_file);
		void	LoadLightData(FBXLightNode* a_light, FILE* a_file);
		void	LoadCameraData(FBXCameraNode* a_camera, FILE* a_file);

		void	ReLink(Node* a_node, std::map<unsigned int, Node*>& nodes);

		unsigned int	NodeCount(Node* a_node);

	private:

		Node*									m_root;

		std::string								m_path;

		glm::vec4								m_ambientLight;
		std::map<std::string,FBXMeshNode*>		m_meshes;
		std::map<std::string,FBXLightNode*>		m_lights;
		std::map<std::string,FBXCameraNode*>	m_cameras;
		std::map<std::string,FBXMaterial*>		m_materials;

		std::vector<FBXSkeleton*>				m_skeletons;
		std::map<std::string,FBXAnimation*>		m_animations;
	};



//////////////////////////////////////////////////////////////////////////
#endif // __FBXLOADER_H_
//////////////////////////////////////////////////////////////////////////
