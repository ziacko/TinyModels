all: ./
	g++ -std=c++11 -fpermissive -g ./Example/Example.cpp -o TinyModelTest -I./include/ -I./dependencies/FBX_SDK/2015.1/include/ -L./dependencies/FBX_SDK/2015.1/lib/gcc4/x64/debug/ -lfbxsdk -ldl -lpthread 2> errors.txt

