#include "TinyModels.h"

int main()
{
	TScene<double>* Scene = new TScene<double>();
	Scene->Load("dragon.fbx");
	return 0;
}