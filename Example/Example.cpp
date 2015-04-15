#include <stdio.h>
#include "TinyModels.h"


int main()
{
	TScene<float>* Scene = new TScene<float>;

	Scene->Load("Dragon.fbx");

	return 0;
}
