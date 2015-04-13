all: ./
	g++ -std=c++11 -fpermissive -c include/TinyModels.h -I./dependencies/FBX_SDK/2015.1/include/ -L./dependencies/FBX_SDK/2015.1/lib/gcc4/x86/debug/ -lfbxsdk 2> errors.txt
