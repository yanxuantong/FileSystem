#pragma once
#include "define.h"
struct supblock
{
	//unsigned int blockNum;				//the number of the block
	unsigned int size;				//�ļ�ϵͳ���̿���
	unsigned int freeBlock[BLOCKNUM];	//�����̿��ջ�������ڼ�¼��ǰ���õĿ����̿��ŵ�ջ��
	unsigned int nextFreeBlock;		//��ǰ�����̿���������ڿ����̿��ջ�б���Ŀ����̿�ŵ���Ŀ����Ҳ���Ա���Ϊ�����̿��ջ��ָ�롣
	unsigned int freeBlockNum;			//���д���i�����ջ������¼�˵�ǰ���õĿ��н���ŵ�ջ��
	unsigned int freeInode[INODENUM];	//���д���i�����Ŀ��ָ�ڴ���i���ջ�б���Ŀ���i����ŵ���Ŀ��Ҳ������Ϊ��ǰ����i���ջ����ָ�롣
	unsigned int freeInodeNum;			//�����̿��������ڼ�¼�����ļ�ϵͳ��δ��������̿������
	unsigned int nextFreeInode;		//����i�����������ڼ�¼�����ļ�ϵͳ��δ������Ľڵ������
	unsigned int lastLogin;	//�ϴε�¼ʱ��
	/**ע�⣺
	*������µ�Ԫ����ӣ���׷�ӣ���Ҫ��������֮��
	**/
};

//node structure in the disk�����������
struct finode
{
	int			mode;//�ļ����ͼ�����
	long		int	fileSize;
	int			fileLink;//�ļ������� 
	char			owner[MAXNAME];//�����û���
	char			group[GROUPNAME];//�����û���
	long		int	modifyTime;//�޸�ʱ�� 
	long		int	createTime;//����ʱ�� 
	int			addr[6];//�̿��ַ����
	char			black[45];				//������Ҳ��ֹ�Ժ����ݽṹ�������µĶ�������Ӱ�����
};

//node structure in the memory�ڴ��������
struct inode
{
	struct					finode finode;//�����������ṹ������Ӵ��̶��������������Ϣ
	struct					inode *parent;//�����ڴ��������ָ��
	unsigned short int		inodeID;				//the node id
	int						userCount;			//the number of process using the inode
};
//direct structure�ļ�Ŀ¼��
struct direct
{
	char					directName[DIRECTNAME];
	unsigned short int	inodeID;
};
//the structure of dirĿ¼���ݽṹ
struct dir
{
	int		dirNum;//Ŀ¼��Ŀ 
	struct	direct direct[DIRNUM];//Ŀ¼������
};
//the structure of file
struct file
{
	struct	inode *fInode;
	int		f_curpos;
};
//the structure of user
struct user
{
	char		userName[MAXNAME];
	char		userPwd[MAXPWD];
	char		userGroup[MAXNAME];
};