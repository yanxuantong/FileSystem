#pragma once
#include "define.h"
struct supblock
{
	unsigned int size;	//�ļ�ϵͳ���̿���
	unsigned int freeBlock[BLOCKNUM];	//�����̿�ջ
	unsigned int nextFreeBlock;		//�����̿�ջָ��
	unsigned int freeBlockNum;			//���õĿ��н������
	unsigned int freeInode[INODENUM];	//���н��ջ
	unsigned int freeInodeNum;			//�����̿���
	unsigned int nextFreeInode;		//ջ���н�����	
	unsigned int lastLogin;	//�ϴε�¼ʱ��
};
//���������ڵ�
struct finode
{
	int			mode;//�ļ���������ֵ
	long		int	fileSize;
	int			fileLink;//�ļ������� 
	char		owner[MAXNAME];//�����û�
	char		group[GROUPNAME];//�����û���
	long		int createTime;//����ʱ�� 
	int			addr[6];//�̿��ַ�洢����
	char		black[45];//�հ����
};
//�ڴ��������
struct inode
{
	struct					finode finode;//�����������
	struct					inode* parent;//�����������ָ��
	unsigned short int		inodeID;
	int			            userCount; //���ļ����û�	
};
//direct structure�ļ�Ŀ¼��
struct direct
{
	char					directName[DIRECTNAME];//�ļ���
	unsigned short int	inodeID;//�ļ�ָ��ڵ�ָ��
};
//the structure of dirĿ¼���ݽṹ
struct dir
{
	int		dirNum;//�ļ���Ŀ 
	struct	direct direct[DIRNUM];//Ŀ¼���ļ�
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