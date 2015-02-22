#include <stdio.h>
#include "TinyModels.h"
int NumTabs = 0;

void PrintTabs()
{
	for (int Iter = 0; Iter < NumTabs; Iter++)
	{
		printf("\t");
	}
}

FbxString GetAttributeTypeName(FbxNodeAttribute::EType Type)
{
	switch (Type)
	{
	case FbxNodeAttribute::eUnknown: return "unidentified";
	case FbxNodeAttribute::eNull: return "null";
	case FbxNodeAttribute::eMarker: return "marker";
	case FbxNodeAttribute::eSkeleton: return "skeleton";
	case FbxNodeAttribute::eMesh: return "mesh";
	case FbxNodeAttribute::eNurbs: return "nurbs";
	case FbxNodeAttribute::ePatch: return "patch";
	case FbxNodeAttribute::eCamera: return "camera";
	case FbxNodeAttribute::eCameraStereo: return "stereo";
	case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
	case FbxNodeAttribute::eLight: return "light";
	case FbxNodeAttribute::eOpticalReference: return "optical reference";
	case FbxNodeAttribute::eOpticalMarker: return "marker";
	case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
	case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
	case FbxNodeAttribute::eBoundary: return "boundary";
	case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
	case FbxNodeAttribute::eShape: return "shape";
	case FbxNodeAttribute::eLODGroup: return "lodgroup";
	case FbxNodeAttribute::eSubDiv: return "subdiv";
	default: return "unknown";

	}
}

void PrintAttribute(FbxNodeAttribute* Attribute)
{
	if (!Attribute)
	{
		return;
	}

	FbxString TypeName = GetAttributeTypeName(Attribute->GetAttributeType());
	FbxString AttributeName = Attribute->GetName();
	PrintTabs();

	printf("<attribute type= '%s' name='%s'/> \n", TypeName.Buffer(), AttributeName.Buffer());
}

void PrintNode(FbxNode* GivenNode)
{
	PrintTabs();
	const char* NodeName = GivenNode->GetName();
	FbxDouble3 Translation = GivenNode->LclTranslation.Get();
	FbxDouble3 Rotation = GivenNode->LclRotation.Get();
	FbxDouble3 Scaling = GivenNode->LclScaling.Get();

	printf("<node name='%s' translation='(%f, %f, %f)' rotation='(%f, %f, %f)' scale='(%f, %f, %f)'>\n",
		NodeName,
		Translation[0], Translation[1], Translation[2],
		Rotation[0], Rotation[1], Rotation[2],
		Scaling[0], Scaling[1], Scaling[2]);
	NumTabs++;

	for (int Iter = 0; Iter < GivenNode->GetNodeAttributeCount(); Iter++)
	{
		PrintAttribute(GivenNode->GetNodeAttributeByIndex(Iter));
	}

	for (int Iter = 0; Iter < GivenNode->GetChildCount(); Iter++)
	{
		PrintNode(GivenNode->GetChild(Iter));
	}
	printf("Polygon count: %i \n", GivenNode->GetMesh()->GetPolygonCount());
	printf("UV count: %s \n", GivenNode->GetMesh()->GetElementUVCount(FbxLayerElement::eUV));
	NumTabs--;
	PrintTabs();
	printf("</node>\n");
}

int main()
{
	const char* FileName = "Dragon.fbx";

	FbxManager* Manager = FbxManager::Create();

	FbxIOSettings* IOSettings = FbxIOSettings::Create(Manager, IOSROOT);
	Manager->SetIOSettings(IOSettings);

	FbxImporter* Importer = FbxImporter::Create(Manager, "");

	if (!Importer->Initialize(FileName, -1, Manager->GetIOSettings()))
	{
		printf("Call to FbxImporter::Initialize() failed \n");
		printf("Error returned: %s\n\n", Importer->GetStatus().GetErrorString());
		exit(-1);
	}

	FbxScene* Scene = FbxScene::Create(Manager, "NewScene");
	Importer->Import(Scene);
	Importer->Destroy();

	FbxNode* RootNode = Scene->GetRootNode();

	if (RootNode)
	{
		for (int Iter = 0; Iter < RootNode->GetChildCount(); Iter++)
		{
			PrintNode(RootNode);
		}
	}
	Manager->Destroy();

	return 0;
}