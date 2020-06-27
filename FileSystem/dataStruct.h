#pragma once
#include "define.h"
struct supblock
{
	unsigned int size;	//文件系统的盘块数
	unsigned int freeBlock[BLOCKNUM];	//空闲盘块栈
	unsigned int nextFreeBlock;		//空闲盘块栈指针
	unsigned int freeBlockNum;			//可用的空闲结点总数
	unsigned int freeInode[INODENUM];	//空闲结点栈
	unsigned int freeInodeNum;			//空闲盘块数
	unsigned int nextFreeInode;		//栈空闲结点个数	
	unsigned int lastLogin;	//上次登录时间
};
//磁盘索引节点
struct finode
{
	int			mode;//文件类型属性值
	long		int	fileSize;
	int			fileLink;//文件连接数 
	char		owner[MAXNAME];//所属用户
	char		group[GROUPNAME];//所属用户组
	long		int createTime;//创建时间 
	int			addr[6];//盘块地址存储数组
	char		black[45];//空白填充
};
//内存索引结点
struct inode
{
	struct					finode finode;//磁盘索引结点
	struct					inode* parent;//父级索引结点指针
	unsigned short int		inodeID;
	int			            userCount; //打开文件的用户	
};
//direct structure文件目录项
struct direct
{
	char					directName[DIRECTNAME];//文件名
	unsigned short int	inodeID;//文件指向节点指针
};
//the structure of dir目录数据结构
struct dir
{
	int		dirNum;//文件数目 
	struct	direct direct[DIRNUM];//目录内文件
};
//the structure of file
/*struct file
{
	struct	inode* fInode;
	int		f_curpos;
};*/
//the structure of user
struct user
{
	char		userName[MAXNAME];
	char		userPwd[MAXPWD];
	char		userGroup[MAXNAME];
};