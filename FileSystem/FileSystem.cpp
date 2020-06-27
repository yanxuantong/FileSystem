#include "fileSystem.h"
#define _CRT_SECURE_NO_WARNINGS

int main()
{
	cout << "FileSystem" << endl;
	int inital = loadSuper((char *)"disk"); //加载超级块，读取文件
	if (inital == ERROR_VM_NOEXIST) {
		cout << "正在新建文件卷" << endl;
		format();
		loadSuper((char*)"disk");
	}
	root = iget(1); //root根目录
	while (!login())//登录
	{
		NULL;
	}
	if (strcmp(curuser->userName, "root") == 0) {
		logindir = 1;
        current = root;
	}
	else
	{
		dir* dir = (struct dir*)calloc(1, sizeof(struct dir));
		bread(dir, root->finode.addr[0], 0, sizeof(struct dir));
		for (int i = 0; i < dir->dirNum; i++)
			if (strcmp(dir->direct[i].directName, curuser->userName) == 0)
			{
				root = iget(dir->direct[i].inodeID);
				current = root;
				logindir = dir->direct[i].inodeID;
				break;
			}
	}
	while (!logout)
	{
		dispatcher();
	}
	return 0;
}