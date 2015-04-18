#include "TinyModels.h"

int main()
{
	//ModelManager<double>::Initalize();
	TScene<double>* Scene = new TScene<double>();

	Scene->Load("resources/Dragon.fbx");

	/*for(int i = 0; i < Scene->Meshes.size(); i++)
	{
		//printf("%s\n", Scene->Meshes[i].
	}*/

	return 0;
}
