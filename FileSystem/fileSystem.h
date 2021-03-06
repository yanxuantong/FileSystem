#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <conio.h>
#include <fstream>
#include <time.h>
#include <stack>
#include <stdio.h>
#include <windows.h>
#include "dataStruct.h"
#include "error.h"

using namespace std;

//内存i节点数组，NUM为该文件系统容纳的文件数  
struct inode	usedinode[7280];
//ROOT的内存i节点  
struct inode *root;
//当前节点
struct inode *current;
//已经打开文件的数组 
struct file* opened[];
//超级块  
struct supblock *super;
//模拟磁盘
FILE * virtualDisk;
//当前用户
struct user* curuser;
//登陆账户根节点
int logindir;
//当前文件或者目录名字（通过文件，获取名字和文件的inode id）
struct direct curdirect;
//当前目录
struct dir* curdir;
//上一层目录名字
char abovedirname[5][20];
int abovenum = 0;
//退出
bool logout = false;
//用户文件节点
struct inode* userinode;
int userpos;

//更新inode，磁盘 ,done
int syncinode(struct inode* inode)
{
	//in: 索引节点地址
	//out：
	//int ino;
	int ret;
	int ipos = BOOTPOS + SUPERSIZE + inode->inodeID * sizeof(struct finode);//节点位置计算
	fseek(virtualDisk, ipos, SEEK_SET);
	ret = fwrite(&inode->finode, sizeof(struct finode), 1, virtualDisk);
	fflush(virtualDisk);//去除无用信息
	if (ret != 1)//成功写入
		return ERROR_SYSINODE_FAIL;
	return ERROR_OK;
}

//更新超级块信息到磁盘,done
int Syncsuperblock()
{
	if (virtualDisk == NULL)
		return ERROR_VM_NOEXIST;
	fseek(virtualDisk, BOOTPOS, SEEK_SET);
	int writeIn = fwrite(super, sizeof(struct supblock), 1, virtualDisk);
	fflush(virtualDisk);
	if (writeIn != 1)
		return ERROR_SYSSUP_FAIL;
	return ERROR_OK;
} 
//加载超级块，磁盘，done
int loadSuper(char *path)
{
	virtualDisk = fopen(path, "r+");
	if (virtualDisk == NULL)
	{
		return ERROR_VM_NOEXIST;
	}
	super = (struct supblock*)calloc(1, sizeof(struct supblock));//内存分配空间
	fseek(virtualDisk, BOOTPOS, SEEK_SET);//跳转位置，the superblock is the #1	block
	int readSize = fread(super, sizeof(struct supblock), 1, virtualDisk);
	if (readSize != sizeof(struct supblock))
		return ERROR_LOADSUP_FAIL;
	return ERROR_OK;
}

//get inodeid为ino的inode，内存
struct inode * iget(int ino)
{
	int ipos = 0;
	int ret = 0;
	if (usedinode[ino].userCount != 0) //确认用户
	{
		usedinode[ino].userCount++;
		return &usedinode[ino];
	}
	if (virtualDisk == NULL)
		return NULL;
	//ipos = BOOTPOS + SUPERSIZE + ino*INODE;
	ipos = BOOTPOS + SUPERSIZE + ino * sizeof(struct finode);
	fseek(virtualDisk, ipos, SEEK_SET);//跳转
	ret = fread(&usedinode[ino], sizeof(struct finode), 1, virtualDisk);
	//ret = fread(&usedinode[ino], sizeof(struct inode), 1, virtualDisk);
	if (ret != 1)
		return NULL;
	if (usedinode[ino].finode.fileLink == 0)		//it is s new file
	{
		usedinode[ino].finode.fileLink++;
		usedinode[ino].finode.fileSize = 0;
		usedinode[ino].inodeID = ino;
		time_t timer;
		time(&timer);
		usedinode[ino].finode.createTime = timer;
		super->freeInodeNum--;
		syncinode(&usedinode[ino]);//更新节点
	}
	usedinode[ino].userCount++;
	usedinode[ino].inodeID = ino;
	return &usedinode[ino];
}

//内存
struct inode* ialloc()
{
	if (super->nextFreeInode == 0)//if there is no free finode in the stack,get free node from the disk
	{
		int num = 0;
		finode *tmp = (struct finode*)calloc(1, sizeof(struct finode));
		fseek(virtualDisk, BOOTPOS + SUPERSIZE, SEEK_SET);
		//cout<<"searching:"<<endl;
		for (int i = 0; i<BLOCKSNUM; i++)
		{
			fread(tmp, sizeof(struct finode), 1, virtualDisk);
			//cout<<"seek:"<<ftell(virtualDisk)<<endl;
			if (tmp->fileLink == 0)
			{
				super->freeInode[num] = i;
				super->nextFreeInode++;
				num++;
			}
			if (num == 20)
				break;
		}
	}
	if (super->nextFreeInode == 0)
		return NULL;
	super->nextFreeInode--;
	Syncsuperblock();
	return iget(super->freeInode[super->nextFreeInode]);
}

//创建新块，磁盘，done
int balloc()
{
	unsigned int bno;
	if (super->freeBlockNum <= 0) //无空闲空间
		return ERROR_BLOCK_EMPTY;
	if (super->nextFreeBlock == 1)//若栈只剩一个空闲盘块，那么采用成组链接法
	{
		bno = super->freeBlock[--super->nextFreeBlock];
		fseek(virtualDisk, super->freeBlock[0] * BLOCKSIZE, SEEK_SET);
		int ret = fread(super->freeBlock, sizeof(unsigned int), BLOCKNUM, virtualDisk);
		super->nextFreeBlock = ret;
		return bno;
	}
	super->freeBlockNum--;
	super->nextFreeBlock--;
	Syncsuperblock();
	return super->freeBlock[super->nextFreeBlock];
}

//时间获取
void getTime(long int timeStamp)
{
	time_t timer;
	timer = timeStamp;
	struct tm *p;
	p = gmtime(&timer);
	char s[80];
	strftime(s, 80, "%Y-%m-%d %H:%M:%S", p);
	printf("%s", s);
}

//写入盘块，磁盘，done
int bwrite(void * _Buf, unsigned short int bno, long int offset, int size, int count = 1)
{
	long int pos;
	int ret;
	pos = bno*BLOCKSIZE + offset;
	fseek(virtualDisk, pos, SEEK_SET);
	ret = fwrite(_Buf, size, count, virtualDisk);
	fflush(virtualDisk);
	if (ret != count)
		return ERROR_BWRITE_FAIL;
	return ERROR_OK;
}

//读取盘块，磁盘，done
int bread(void * _Buf, unsigned short int bno, int offset, int size, int count = 1)
{
	long int pos;
	int ret;
	pos = bno*BLOCKSIZE + offset;
	fseek(virtualDisk, pos, SEEK_SET);
	ret = fread(_Buf, size, count, virtualDisk);
	if (ret != count)
		return ERROR_BREAD_FAIL;
	return ERROR_OK;
}

//删除盘块，磁盘，done
int bfree(int bno)
{
	if (super->nextFreeBlock == 20) //复制空闲块管理索引表，转移表
	{
		bwrite(&super->freeBlock, bno, 0, sizeof(unsigned int), 20);
		super->nextFreeBlock = 1;
		super->freeBlock[0] = bno;
	}
	else
	{
		super->freeBlock[super->nextFreeBlock] = bno;
		super->nextFreeBlock++;
	}
	super->freeBlockNum++;
	Syncsuperblock();
	return 1;
}

//显示文件权限信息，文件
void getMode(int mode)
{
	int type = mode / 1000;
	int auth = mode % 1000;
	(type == 1)?  cout << "d" :  cout << "-";
	int div = 100;
	int a[3];
	for (int i = 0; i<3; i++){
		a[0] = 0;
		a[1] = 0;
		a[2] = 0;
		int num = auth / div;
		auth = auth % div;
		int time = 2;
		while (num != 0)
		{
			a[time--] = num % 2;
			num = num / 2;
		}
		for (int i = 0; i<3; i++)
		{
			if (a[i] == 1)
			{
				if (i == 2)
					cout << "x";
				else if (i == 1)
					cout << "w";
				else if (i == 0)
					cout << "r";
			}
			else
				cout << "-";
		}
		div /= 10;
	}
}
//显示信息，文件
void info(inode* inode)
{
	cout << "mode:" << inode->finode.mode << endl;
	getMode(inode->finode.mode);
	cout << endl;
	cout << "owner:";
	cout.write(inode->finode.owner, MAXNAME);
	cout << endl;
	cout << "group:";
	cout.write(inode->finode.group, GROUPNAME);
	cout << endl;
	getTime(inode->finode.createTime);
	cout << endl;
	cout << "FileLink:" << inode->finode.fileLink << endl;
	cout << "fileSize:" << inode->finode.fileSize << endl;
	for (int i = 0; i<6; i++)
		cout << "addr" << i << ":" << inode->finode.addr[i] << endl;
}
//显示超级块信息
void superInfo()
{
	cout << "size: " << super->size << endl;
	cout << "freeBlock:" << endl;
	for (int i = 0; i<super->nextFreeBlock; i++)
		cout << super->freeBlock[i] << " ";
	cout << endl;
	cout << "freeInode:" << endl;
	for (int i = 0; i<super->nextFreeInode; i++)
		cout << super->freeInode[i] << " ";
	cout <<endl<< "nextFreeInode:" << super->nextFreeInode << endl;
	cout << "nextFreeBlock:" << super->nextFreeBlock << endl;
	cout << "freeBlockNum:" << super->freeBlockNum << endl;
	cout << "freeInodeNum:" << super->freeInodeNum << endl;
}
//登录
int login()
{
	char ch;
	userinode = iget(0);
	int usernum = userinode->finode.fileSize / sizeof(user);
	struct user* users = (struct user*)calloc(usernum, sizeof(struct user));
	bread(users, userinode->finode.addr[0], 0, sizeof(struct user), usernum);
	char user[MAXNAME] = { 0 }, pwd[MAXPWD] = { 0 };
	cout << "username:";
	cin >> user;
	for (int i = 0; i<usernum; i++)
	{
		if (strcmp(users[i].userName, user) == 0)
		{
			cout << "password:";
			for (int i = 0; i<MAXPWD; i++)
			{
				ch = _getch();
				if (ch == 13)
				{
					break;
				}
				pwd[i] = ch;
				cout << " ";
			}
			cout << endl;
			if (strcmp(users[i].userPwd, pwd) == 0)
			{
				curuser = &users[i];
				userpos = i;
				time_t timer;
				time(&timer);
				cout << "last login:";
				getTime(super->lastLogin);
				cout << endl;
				super->lastLogin = timer + 8 * 60 * 60;
				Syncsuperblock();
				return true;
			}
			else
			{
				cout << "password is not match!" << endl;
				return false;
			}
		}
	}
	cout << "User does not exist" << endl;
	return false;
}
//查找某个字符在字符串中的位置
int strPos(char* str, int start, const char needle)
{
	for (int i = start; i<strlen(str); i++)
	{
		if (str[i] == needle)
			return i;
	}
	return -1;
}
//复制str
int strCpy(char *dst, char *src, int offset)
{
	int len = strlen(src);
	if (len <= offset)
		return 0;
	int i;
	for (i = 0; i<len - offset; i++)
	{
		dst[i] = src[i + offset];
		//cout<<"dst["<<i<<"]"<<dst[i]<<endl;
	}
	dst[i] = 0;
	return 1;
}
//从start开始复制字符串
int subStr(char* src, char*dst, int start, int end = -1)
{
	int pos = 0;
	end == -1 ? end = strlen(src) : NULL;
	for (int i = start; i<end; i++)
		dst[pos++] = src[i];
	dst[pos] = 0;
	return 1;
}

//改变当前位置，内存
inode* cd(char* path, inode* inode)
{
	if (path[0] == '/'&&strlen(path) == 1)
	{
		current = root;
		return NULL;
	}
	char tmp[16] = { 0 };
	int start = path[0] == '/';
	int pos = strPos(path, 1, '/');
	subStr(path, tmp, start, pos);
	int type = inode->finode.mode / 1000;
	if (type == 1)
	{
		int count = inode->finode.fileSize / sizeof(struct direct);
		int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
		dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
		addrnum>4 ? addrnum = 4 : NULL;
		for (int addr = 0; addr<addrnum; addr++)
		{
			bread(dir, inode->finode.addr[addr], 0, sizeof(struct dir));
			for (int i = 0; i<dir->dirNum; i++)
			{
				if (strcmp(dir->direct[i].directName, tmp) == 0)
				{
					count = -1;
					strcpy(abovedirname[abovenum++],curdirect.directName);
					curdirect = dir->direct[i];
					struct inode * tmpnode = iget(dir->direct[i].inodeID);
					tmpnode->parent = inode;
					inode = tmpnode;
					break;
				}
			}
			if (count == -1)
				break;
		}
		if (count != -1)
			inode = NULL;
	}
	else
	{
		inode = NULL;
		return inode;
	}
	if (pos != -1 && inode != NULL)
	{
		subStr(path, tmp, pos + 1);
		return cd(tmp, inode);
	}
	if (pos == -1)
		return inode;
}
//显示文件目录，文件
int ls()
{
	int type = current->finode.mode / 1000;
	if (type != 1)
	{
		cout << "this is not a dir" << endl;
		return 0;
	}
	int count = current->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
			puts(dir->direct[i].directName);
	}
	return 1;

}
//显示文件目录详细，文件
int ll()
{
	int type = current->finode.mode / 1000;
	if (type != 1)
	{
		cout << "this is not a dir" << endl;
		return 0;
	}
	int count = current->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
		{
			inode * inode = iget(dir->direct[i].inodeID);
			getMode(inode->finode.mode);
			cout << " ";
			cout.write(inode->finode.owner, strlen(inode->finode.owner));
			cout << " ";
			cout.write(inode->finode.group, strlen(inode->finode.group));
			cout << " ";
			puts(dir->direct[i].directName);
		}
	}
	return 1;

}
//查找普通用户的根节点
inode* namespaceroot(string username)
{
	int type = root->finode.mode / 1000;
	int count = root->finode.fileSize / sizeof(struct direct);
	dir* dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum > 4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr < addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i < dir->dirNum; i++)
		{
			inode* inode = iget(dir->direct[i].inodeID);
			string name = dir->direct[i].directName;
			if (name == username)
			{
				return inode;
			}
		}
	}
}
//新建目录，文件
int mkdir(char * dirname)
{
	if (current->finode.mode / 1000 != 1)
	{
		cout << "it is not a dir" << endl;
		return -1;
	}
	int count = current->finode.fileSize / sizeof(struct direct);
	if (count>252)//63*4=252
	{
		cout << "can not make more dir in the current dir!" << endl;
		return -1;
	}
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
			if (strcmp(dir->direct[i].directName, dirname) == 0)
			{
				cout.write(dirname, strlen(dirname));
				cout << " is exist in current dir!" << endl;
				return -1;
			}
	}
	current->finode.fileSize += sizeof(struct direct);
	syncinode(current);
	int addr = count / 63;
	bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
	strcpy(dir->direct[dir->dirNum].directName, dirname);
	struct inode * tmpinode = ialloc();
	tmpinode->finode.addr[0] = balloc();
	tmpinode->finode.mode = 1774;
	strcpy(tmpinode->finode.owner, curuser->userName);
	strcpy(tmpinode->finode.group, curuser->userGroup);
	syncinode(tmpinode);
	dir->direct[dir->dirNum].inodeID = tmpinode->inodeID;
	dir->dirNum += 1;
	bwrite(dir, current->finode.addr[addr], 0, sizeof(struct dir));
	return 1;
}

//返回上一层目录，文件
void cd__()
{
	int inodeid = current->inodeID;
	inode* inode = current->parent;
	int count = inode->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, inode->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
		{
			if (dir->direct[i].inodeID == inodeid)
			{
				count = -1;
				if (inode->inodeID == logindir) {
                      strcpy(curdirect.directName," ");
					  curdirect.inodeID = logindir;
				}
				else
				{
                      curdirect = dir->direct[i];
					  strcpy(curdirect.directName, abovedirname[--abovenum]);
				}
				      

				current = inode;
				break;
			}
		}
	}
}
//密码修改，用户
void passwd()
{
	char ch;
	char passwd1[20] = { 0 };
	char passwd2[20] = { 0 };
	char passwd[20] = { 0 };
	cout << "please insert the old password:";
	for (int i = 0; i<MAXPWD; i++)
	{
		ch = _getch();
		if (ch == 13)
		{
			break;
		}
		passwd[i] = ch;
		cout << "*";
	}
	cout << endl;
	if (strcmp(passwd, curuser->userPwd) == 0)
	{
		cout << "new password:";
		for (int i = 0; i<MAXPWD; i++)
		{
			ch = _getch();
			if (ch == 13)
			{
				break;
			}
			passwd1[i] = ch;
			cout << "*";
		}
		cout << endl;
		cout << "repeat the new password:";
		for (int i = 0; i<MAXPWD; i++)
		{
			ch = _getch();
			if (ch == 13)
			{
				break;
			}
			passwd2[i] = ch;
			cout << "*";
		}
		cout << endl;
		if (strcmp(passwd1, passwd2) == 0)
		{
			strcpy(curuser->userPwd, passwd1);
			bwrite(curuser, userinode->finode.addr[0], userpos * sizeof(struct user), sizeof(struct user));
		}
		else
		{
			cout << "the password is not matched!" << endl;
		}
	}
	else
		cout << "the old password is not right!" << endl;

}

//改变文件权限，文件
int chmod(char * para)
{
	int mode = 0;
	char dirname[100] = { 0 };
	mode += ((para[0] - '0')>7 ? 7 : (para[0] - '0')) * 100 + ((para[1] - '0')>7 ? 7 : (para[1] - '0')) * 10 + ((para[2] - '0')>7 ? 7 : (para[2] - '0'));
	subStr(para, dirname, 4);
	int count = current->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
		{
			if (strcmp(dir->direct[i].directName, dirname) == 0)
			{
				inode* tmp = iget(dir->direct[i].inodeID);
				if (logindir!=1 && strcmp(tmp->finode.owner, "root") == 0)
				{
					cout << "privilege error" << endl;
					return -1;
				}
				tmp->finode.mode = (tmp->finode.mode / 1000) * 1000 + mode;
				syncinode(tmp);
				//info(tmp);
				return 1;
			}
		}
	}
	return -1;
}
//显示当前目录，内存
int pwd()
{
	stack<char*> pwd;
	struct inode * inode = current;
	struct inode * parent = NULL;
	while (inode != root)
	{
		int inodeid = inode->inodeID;
		parent = inode->parent;
		int count = parent->finode.fileSize / sizeof(struct direct);
		dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
		int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
		addrnum>4 ? addrnum = 4 : NULL;
		for (int addr = 0; addr<addrnum; addr++)
		{
			bread(dir, parent->finode.addr[addr], 0, sizeof(struct dir));
			for (int i = 0; i<dir->dirNum; i++)
			{
				if (dir->direct[i].inodeID == inodeid)
				{
					pwd.push(dir->direct[i].directName);
					count = -1;
					break;
				}
			}
			if (count == -1)
				break;
		}
		inode = inode->parent;
	}
	if (pwd.empty())
		cout << "/";
	else
	{
		while (!pwd.empty())
		{
			cout << "/";
			cout.write(pwd.top(), strlen(pwd.top()));
			pwd.pop();
		}
	}
	cout << endl;
	return 1;
}
//改变文件名，文件
int mv(char* names)
{
	char oldname[16] = { 0 }, newname[16] = { 0 };
	int pos = strPos(names, 0, ' ');
	subStr(names, oldname, 0, pos);
	subStr(names, newname, pos + 1);
	int count = current->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
		{
			if (strcmp(dir->direct[i].directName, oldname) == 0)
			{
				strcpy(dir->direct[i].directName, newname);
				bwrite(dir, current->finode.addr[addr], 0, sizeof(struct dir));
				return 1;
			}
		}
	}
	return 1;
}
//新建文件，文件
int touch(char * filename)
{
	if (current->finode.mode / 1000 != 1)
	{
		cout << "it is not a dir" << endl;
		return -1;
	}
	int count = current->finode.fileSize / sizeof(struct direct);
	if (count>252)
	{
		cout << "can not make more dir in the current dir!" << endl;
		return -1;
	}
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
			if (strcmp(dir->direct[i].directName, filename) == 0)
			{
				cout.write(filename, strlen(filename));
				cout << " is exist in current dir!" << endl;
				return -1;
			}
	}
	current->finode.fileSize += sizeof(struct direct);
	syncinode(current);
	int addr = count / 63;
	bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
	strcpy(dir->direct[dir->dirNum].directName, filename);
	struct inode * tmpinode = ialloc();
	tmpinode->finode.addr[0] = balloc();
	tmpinode->finode.mode = (int)774;
	strcpy(tmpinode->finode.owner, curuser->userName);
	strcpy(tmpinode->finode.group, curuser->userGroup);
	syncinode(tmpinode);
	dir->direct[dir->dirNum].inodeID = tmpinode->inodeID;
	dir->dirNum += 1;
	bwrite(dir, current->finode.addr[addr], 0, sizeof(struct dir));
	return 1;
}

//文件添加内容，文件
int append(char * src)
{
	char filename[16] = { 0 }, content[8192] = { 0 };
	int pos = strPos(src, 0, ' ');
	int inodeid = -1;
	subStr(src, filename, 0, pos);
	subStr(src, content, pos + 1);
	int count = current->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
		{
			if (strcmp(dir->direct[i].directName, filename) == 0)
			{
				inodeid = dir->direct[i].inodeID;
				count = -1;
				break;
			}
		}
		if (count == -1)
			break;
	}
	if (inodeid == -1)
	{
		cout << "can not found the file ";
		cout.write(filename, strlen(filename));
		cout << endl;
		return -1;
	}
	struct inode * inode = iget(inodeid);
	int mode = inode->finode.mode;
	if (logindir == 1)
	{
		if (mode / 100 <= 5)//只读
		{
			cout << "privilege error"<<endl;
			return -1;
		}
	}
	else
	{
		mode = mode % 100;
		if (mode / 10 <= 5)
		{
			cout << "privilege error" << endl;
			return -1;
		}
	}
	int fileSize = inode->finode.fileSize;
	int index = 0;
	int addr = fileSize / 1024;
	int offset = fileSize % 1024;
	int charCount = strlen(content) - index;
	int writeCount = (charCount>1024 ? 1024 - offset : charCount);
	bwrite(&content[index], inode->finode.addr[addr], offset, sizeof(char), writeCount);
	index += writeCount;
	inode->finode.fileSize += writeCount;
	while (index<strlen(content) && addr<4)
	{
		inode->finode.addr[++addr] = balloc();
		offset = inode->finode.fileSize % 1024;
		charCount = strlen(content) - index;
		writeCount = (charCount>1024 ? 1024 - offset : charCount);
		bwrite(&content[index], inode->finode.addr[addr], offset, sizeof(char), writeCount);
		inode->finode.fileSize += writeCount;
	}
	syncinode(inode);
	return 1;
}
//连接显示文件内容，内存
int cat(char * filename)
{
	int inodeid = 0;
	int count = current->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
		{
			if (strcmp(dir->direct[i].directName, filename) == 0)
			{
				inodeid = dir->direct[i].inodeID;
				count = -1;
				break;
			}
		}
		if (count == -1)
			break;
	}
	if (inodeid == 0)
	{
		cout << "can not found the file ";
		cout.write(filename, strlen(filename));
		cout << endl;
		return -1;
	}
	struct inode* inode = iget(inodeid);
	int addr = inode->finode.fileSize / 1024;
	int lastCount = inode->finode.fileSize % 1024;
	int i;
	for (i = 0; i<addr; i++)
	{
		char content[1024] = { 0 };
		bread(&content, inode->finode.addr[i], 0, sizeof(char), 1024);
		cout.write(content, 1024);
	}
	char content[1024] = { 0 };
	bread(&content, inode->finode.addr[i], 0, sizeof(char), lastCount);
	cout.write(content, strlen(content));
	cout << endl;
	return 1;
}
//文件目录递归删除，文件
int _rmdir(struct inode* inode)
{
	struct inode * rminode = NULL;
	int count = inode->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, inode->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
		{
			rminode = iget(dir->direct[i].inodeID);
			if (rminode->finode.mode / 1000 == 1)
			{
				_rmdir(rminode);
			}
			int rmaddr = rminode->finode.fileSize / 1024 + (rminode->finode.fileSize % 1024 == 0 ? 0 : 1);
			if (rminode->finode.fileSize == 0)
				rmaddr = 1;
			for (int i = 0; i<rmaddr; i++)
				bfree(rminode->finode.addr[i]);
			rminode->finode.fileLink = 0;
			syncinode(rminode);
		}
	}
	return 1;
}
//文件目录删除，文件
int rmdir(struct inode* inode, char* filename)
{
	struct inode* rminode = NULL;
	int count = inode->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, inode->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
		{
			if (strcmp(dir->direct[i].directName, filename) == 0)
			{
				rminode = iget(dir->direct[i].inodeID);
				for (int j = i; j<dir->dirNum; j++)
					dir->direct[j] = dir->direct[j + 1];
				dir->dirNum--;
				bwrite(dir, inode->finode.addr[addr], 0, sizeof(struct dir));
				count = -1;
				break;
			}
		}
		if (count == -1)
			break;
	}
	_rmdir(rminode);
	int rmaddr = rminode->finode.fileSize / 1024 + (rminode->finode.fileSize % 1024 == 0 ? 0 : 1);
	if (rminode->finode.fileSize == 0)
		rmaddr = 1;
	for (int i = 0; i<rmaddr; i++)
		bfree(rminode->finode.addr[i]);
	rminode->finode.fileLink = 0;
	super->freeInodeNum++;
	Syncsuperblock();
	syncinode(rminode);
	return 1;
}
//改变文件所属组，文件
int chgrp(char *src)
{
	char filename[16] = { 0 }, groupname[GROUPNAME] = { 0 };
	int pos = strPos(src, 0, ' ');
	int inodeid = -1;
	subStr(src, filename, 0, pos);
	subStr(src, groupname, pos + 1);
	//puts(filename);
	//puts(groupname);
	if (strcmp(curuser->userName, "root") != 0)
	{
		cout << "Access deny!" << endl;
		return -1;
	}
	int count = current->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
		{
			if (strcmp(dir->direct[i].directName, filename) == 0)
			{
				inode * inode = iget(dir->direct[i].inodeID);
				strcpy(inode->finode.group, groupname);
				syncinode(inode);
				//bwrite(dir,current->finode.addr[addr],0,sizeof(struct dir));
				count = -1;
				return 1;
			}
		}
		if (count == -1)
			break;
	}
	cout << "can not find file ";
	puts(filename);
	return 1;
}
//改变文件拥有者，文件
int chown(char *src)
{
	char filename[16] = { 0 }, owner[MAXNAME] = { 0 };
	int pos = strPos(src, 0, ' ');
	int inodeid = -1;
	subStr(src, filename, 0, pos);
	subStr(src, owner, pos + 1);
	//puts(filename);
	//puts(groupname);
	if (strcmp(curuser->userName, "root") != 0)
	{
		cout << "Access deny!" << endl;
		return -1;
	}
	int count = current->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
		{
			if (strcmp(dir->direct[i].directName, filename) == 0)
			{
				inode * inode = iget(dir->direct[i].inodeID);
				strcpy(inode->finode.owner, owner);
				syncinode(inode);
				//bwrite(dir,current->finode.addr[addr],0,sizeof(struct dir));
				count = -1;
				return 1;
			}
		}
		if (count == -1)
			break;
	}
	cout << "can not find file ";
	puts(filename);
	return 1;
}
//文件拷贝，文件
int cp(char * src)
{
	char srcfile[16] = { 0 }, newfile[GROUPNAME] = { 0 };
	int pos = strPos(src, 0, ' ');
	subStr(src, srcfile, 0, pos);
	subStr(src, newfile, pos + 1);
	int inodeid = 0;
	int count = current->finode.fileSize / sizeof(struct direct);
	dir * dir = (struct dir*)calloc(1, sizeof(struct dir));
	int addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
		{
			if (strcmp(dir->direct[i].directName, srcfile) == 0)
			{
				inodeid = dir->direct[i].inodeID;
				count = -1;
				break;
			}
		}
		if (count == -1)
			break;
	}
	if (inodeid == 0)
	{
		cout << "can not found the file ";
		cout.write(srcfile, strlen(srcfile));
		cout << endl;
		return -1;
	}
	inode* srcinode = iget(inodeid);
	count = current->finode.fileSize / sizeof(struct direct);
	if (count>252)
	{
		cout << "can not make more dir in the current dir!" << endl;
		return -1;
	}
	addrnum = count / 63 + (count % 63 >= 1 ? 1 : 0);
	addrnum>4 ? addrnum = 4 : NULL;
	for (int addr = 0; addr<addrnum; addr++)
	{
		bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
		for (int i = 0; i<dir->dirNum; i++)
			if (strcmp(dir->direct[i].directName, newfile) == 0)
			{
				cout.write(newfile, strlen(newfile));
				cout << " is exist in current dir!" << endl;
				return -1;
			}
	}
	current->finode.fileSize += sizeof(struct direct);
	syncinode(current);
	int addr = count / 63;
	bread(dir, current->finode.addr[addr], 0, sizeof(struct dir));
	strcpy(dir->direct[dir->dirNum].directName, newfile);
	struct inode * tmpinode = ialloc();
	//addr distribut
	count = srcinode->finode.fileSize / 1024;
	int srcaddr = srcinode->finode.fileSize / 1024 + (srcinode->finode.fileSize % 1024 == 0 ? 0 : 1);
	if (srcinode->finode.fileSize == 0)
		srcaddr = 1;
	for (int i = 0; i<srcaddr; i++)
	{
		tmpinode->finode.addr[i] = balloc();
		char content[1024] = { 0 };
		bread(&content, srcinode->finode.addr[i], 0, sizeof(char), 1024);
		bwrite(&content, tmpinode->finode.addr[i], 0, sizeof(char), 1024);
	}
	tmpinode->finode.fileSize = srcinode->finode.fileSize;
	tmpinode->finode.mode = srcinode->finode.mode;
	strcpy(tmpinode->finode.owner, srcinode->finode.owner);
	strcpy(tmpinode->finode.group, srcinode->finode.group);
	syncinode(tmpinode);
	dir->direct[dir->dirNum].inodeID = tmpinode->inodeID;
	dir->dirNum += 1;
	bwrite(dir, current->finode.addr[addr], 0, sizeof(struct dir));
	return 1;
}

void createuser()
{
	user* rootuser = new user;
	char name, password, group;
	cout << "Please enter username:" << endl;
	cin >> rootuser->userName;
	cout << "Please enter password:" << endl;
	cin >> rootuser->userPwd;
	cout << "Please enter the groupname of the user:" << endl;
	cin >> rootuser->userGroup;
	inode* usernode = iget(0);
	int usernum = usernode->finode.fileSize / sizeof(user);
	usernode->finode.fileSize += sizeof(struct user);
	fseek(virtualDisk, 931*BLOCKSIZE + usernum * sizeof(struct user), SEEK_SET);//跳转到931块
	fwrite(rootuser, sizeof(struct user), 1, virtualDisk);
	syncinode(usernode);
	mkdir(rootuser->userName); //建立用户的目录
}

//命令识别，内存
int dispatcher()
{
	char shell[8192] = { 0 };
	char command[8192] = { 0 };
	char ch;
	cout << "[" << curuser->userName << "@localhost " <<
		(current->inodeID == 0 ? "/" : curdirect.directName) 
		<< "]" << (strcmp(curuser->userName, "root") == 0 ? "#" : "$");
	ch = getchar();
	int i = 0;
	if (ch == 10)
		return 0;
	while (ch != 10)
	{
		shell[i] = ch;
		ch = getchar();
		i++;
	}
	_strlwr(shell);
	strcpy(command, shell);
	strtok(command, " ");
	if (strcmp(command, "ls") == 0)
	{
		ls();
	}
	else if (strcmp(command, "ll") == 0)
	{
		ll();
	}
	else if (strcmp(command, "mkdir") == 0)
	{
		strCpy(command, shell, strlen(command) + 1);
		mkdir(command);
	}
	else if (strcmp(command, "cd") == 0)
	{
		inode* inode = NULL, *ret = NULL;
		strCpy(command, shell, strlen(command) + 1);
		if (command[0] == '/')
			inode = root;
		else
			inode = current;
		ret = cd(command, inode);
		if (ret != NULL)
			current = ret;
	}
	else if (strcmp(command, "cd../") == 0)
	{
		if (current->parent != NULL)
		{
			inode* tmp = current->parent;
			//current = tmp;
			if (tmp->inodeID != 0)
				cd__();
		}
	}
	else if (strcmp(command, "passwd") == 0)
	{
		strCpy(command, shell, strlen(command) + 1);
		passwd();
	}
	else if (strcmp(command, "chmod") == 0)
	{
		strCpy(command, shell, strlen(command) + 1);
		if (chmod(command) == 1)
		{
			cout << "change succeed!" << endl;
		}
		else
		{
			cout << "direct not found!" << endl;
		}
	}
	else if (strcmp(command, "mv") == 0)
	{
		strCpy(command, shell, strlen(command) + 1);
		mv(command);
	}
	else if (strcmp(command, "touch") == 0)
	{
		strCpy(command, shell, strlen(command) + 1);
		touch(command);
	}
	else if (strcmp(command, ">>") == 0)
	{
		strCpy(command, shell, strlen(command) + 1);
		append(command);
	}
	else if (strcmp(command, "cat") == 0)
	{
		strCpy(command, shell, strlen(command) + 1);
		cat(command);
	}
	else if (strcmp(command, "rmdir") == 0)
	{
		strCpy(command, shell, strlen(command) + 1);
		rmdir(current, command);
	}
	else if (strcmp(command, "chgrp") == 0)
	{
		strCpy(command, shell, strlen(command) + 1);
		chgrp(command);
	}
	else if (strcmp(command, "chown") == 0)
	{
		strCpy(command, shell, strlen(command) + 1);
		chown(command);
	}
	else if (strcmp(command, "cp") == 0)
	{
		strCpy(command, shell, strlen(command) + 1);
		cp(command);
	}
	else if (strcmp(command, "help") == 0) 
	{
			cout << " 1．Chgrp	改变文件所属组：" << endl;
			cout << "	1)	命令格式：" << endl;
			cout << "	chgrp[文件][组]" << endl;
			cout << "	2)	命令功能：" << endl;
			cout << "	chgrp命令可采用群组名称或群组识别码的方式改变文件或目录的所属群组。使用权限是超级用户。" << endl;
			cout << "	3)	命令参数：" << endl;
			cout << "	[文件]：文件名称" << endl;
			cout << "	[组]：用户组名称" << endl;
			cout << "2．Chown	改变文件拥有者：" << endl;
			cout << "	1)	命令格式：" << endl;
			cout << "	chown[文件][用户名]" << endl;
			cout << "	2)	命令功能：" << endl;
			cout << "	通过chown改变文件的拥有者。" << endl;
			cout << "	3)	命令参数：" << endl;
			cout << "	[文件]：要修改的文件名" << endl;
			cout << "	[用户名]：要更改的新的用户名" << endl;
			cout << "3．Chmod	改变文件权限：" << endl;
			cout << "	1)	命令格式：" << endl;
			cout << "	chmod[mode][文件名]" << endl;
			cout << "	2)	命令功能：" << endl;
			cout << "	chmod命令用于改变linux系统文件或目录的访问权限。用它控制文件或目录的访问权限。" << endl;
			cout << "	3)	命令参数：" << endl;
			cout << "	[mode] 文件权限码" << endl;
			cout << "	r = 4，w = 2，x = 1" << endl;
			cout << "	若要rwx属性则4 + 2 + 1 = 7" << endl;
			cout << "	若要rw - 属性则4 + 2 = 6；" << endl;
			cout << "	若要r - x属性则4 + 1 = 7。" << endl;
			cout << "	[文件名] 要更改的文件名称" << endl;
			cout << "4．Rmdir	删除文件（兼顾删除子目录功能）：" << endl;
			cout << "	1)	命令格式：" << endl;
			cout << "	rmdir 文件名 / 目录名" << endl;
			cout << "	2)	命令功能：" << endl;
			cout << "	该命令从一个目录中删除一个文件或者是目录项，若是目录项，目录项下的所有文件和目录也将被删除。" << endl;
			cout << "5．Cat		连接显示文件内容：" << endl;
			cout << "	1)	命令格式：" << endl;
			cout << "	cat 文件名" << endl;
			cout << "	2)	命令功能：" << endl;
			cout << "	cat命令的用途是连接文件并打印。这个命令常用来显示文件内容。" << endl;
			cout << "6．Cp		文件拷贝：" << endl;
			cout << "	1)	命令格式：" << endl;
			cout << "	cp 源 目的" << endl;
			cout << "	2)	命令功能：" << endl;
			cout << "	cp命令用来复制文件或者目录，是UNIX系统中最常用的命令之一。" << endl;
			cout << "	3)	命令参数：" << endl;
			cout << "	源：要复制的源文件" << endl;
			cout << "	目的：要复制的目的文件" << endl;
			cout << "7．touch	新建一个文件：" << endl;
			cout << "	1)	命令格式：" << endl;
			cout << "	touch 文件名" << endl;
			cout << "	2)	命令功能：" << endl;
			cout << "	用于创建一个新的不存在文件" << endl;
			cout << "8． >> 向文件里面追加内容：" << endl;
			cout << "	1)	命令格式：" << endl;
			cout << "	>> 文件名 内容" << endl;
			cout << "	2)	命令功能：" << endl;
			cout << "	改指令用于向已尽存在的文件中追加内容，内容将会追加到问价你的末尾。" << endl;
			cout << "9．mv		修改文件名" << endl;
			cout << "	1)	命令格式：" << endl;
			cout << "	mv 文件 新文件名" << endl;
			cout << "	2）命令功能：" << endl;
			cout << "	改指令用于修改目录中已经存在文件名。" << endl;
	}
	else if (strcmp(command, "pwd") == 0)
		pwd();
	else if (strcmp(command, "creat") == 0)
	    createuser();
	else if (strcmp(command, "info") == 0)
		superInfo();
	else if (strcmp(command, "exit") == 0)
		logout = true;
	//strCpy(command,shell,strlen(command)+1);
	return 1;
}

//初始化磁盘
bool format()
{
	FILE* fd = fopen("disk", "wb+");
	if (fd == NULL)
	{
		printf("初始化文件系统失败!\n");
		return false;
	}
	//大小为 8 MB
	int buffer[1024] = { 0 };
	for (int i = 0; i < 8*1024; i++) {
		fwrite(buffer,1024, 1, fd);
	}
	//初始化超级块
	supblock superblock;
	superblock.size = 8 * 1024 * 1024;
	for (int i = 0; i < 20; i++)
		superblock.freeBlock[i] = 912 + i;
	superblock.nextFreeBlock = BLOCKNUM - 2;
	superblock.freeBlockNum = BLOCKSNUM - 2;
	for (int i = 0; i < INODENUM; i++)
	{
		superblock.freeInode[i] = i;
	}
	superblock.freeInodeNum = BLOCKSNUM - 2;
	superblock.nextFreeInode = INODENUM - 2;
	superblock.lastLogin = 0;
	fseek(fd, BOOTPOS, SEEK_SET);
	fwrite(&superblock, sizeof(struct supblock), 1, fd);

	//初始化用户root
	user *rootuser=new user;
	strcpy(rootuser->userName,"root");
	strcpy(rootuser->userPwd, "root");
	strcpy(rootuser->userGroup, "admin");
	fseek(fd, 931*BLOCKSIZE, SEEK_SET);//跳转到913块
	fwrite(rootuser, sizeof(struct user), 1, fd);

	//初始化用户节点
	inode *usernode = new inode;
	usernode->finode.fileLink++;
	usernode->finode.fileSize = sizeof(struct user);
	usernode->inodeID = 0;
	usernode->finode.mode = (int)774;
	usernode->finode.addr[0] = 931;
	strcpy(usernode->finode.owner, "root");
	strcpy(usernode->finode.group, "admin");
	time_t timer;
	time(&timer);
	usernode->finode.createTime = timer;
	//syncinode(usernode);
	int ipos = BOOTPOS + SUPERSIZE + usernode->inodeID * sizeof(struct finode);//节点位置计算
	fseek(fd, ipos, SEEK_SET);
	fwrite(&usernode->finode, sizeof(struct finode), 1, fd);
	fflush(fd);//去除无用信息

	//创建根目录
	dir *home=new dir;
	home->dirNum = 1;
	strcpy(home->direct[0].directName,"userstore");
	home->direct[0].inodeID = 931;
	fseek(fd, 930*BLOCKSIZE, SEEK_SET);//跳转
	fwrite(home, sizeof(struct dir), 1, fd);
	fflush(fd);//去除无用信息
	inode *anode=new inode;
	anode->finode.fileLink++;
	anode->inodeID = 1;
	anode->finode.mode = 1774;
	anode->finode.addr[0] = 930;
	strcpy(anode->finode.owner, "root");
	strcpy(anode->finode.group, "admin");
	anode->finode.fileSize = sizeof(struct direct);
	timer = time(NULL);
	anode->finode.createTime = (long int)timer;
	//syncinode(anode);
	ipos = BOOTPOS + SUPERSIZE + anode->inodeID * sizeof(struct finode);//节点位置计算
	fseek(fd, ipos, SEEK_SET);
	fwrite(&anode->finode, sizeof(struct finode), 1, fd);
	fflush(fd);//去除无用信息
	
	//初始化文件数据区，成组链接表首实现
	unsigned int group[BLOCKNUM];
	for (int i = 0; i < BLOCKNUM; i++)
	{
		group[i] = i + 912;
	}
	for (int i = 0; i < 363; i++)
	{
		for (int j = 0; j < BLOCKNUM; j++)
		{
			group[j] += BLOCKNUM;
		}
		fseek(fd, (BLOCKSTART + i * 20) * 1024, SEEK_SET);			//cout<<BLOCKSTART+i*20;
		fwrite(group, sizeof(unsigned int), BLOCKNUM, fd);
	}
	fclose(fd);
}