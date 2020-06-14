#include "fileSystem.h"
#define _CRT_SECURE_NO_WARNINGS

int main()
{
	cout << "FileSystem beta1.0" << endl;
	loadSuper((char *)"vm.dat"); //加载超级块，读取文件
	root = iget(0);//获取根节点的指针
	while (!login())//登录
	{
		NULL;
	}
	current = root;
	while (!logout)
	{
		dispatcher();
	}
	return 0;
}