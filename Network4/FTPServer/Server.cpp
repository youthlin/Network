/*
1.在前三个实验的基础上，将其改造为一个能传输指定文件名称的点对点文件传输软件
2.设计并实现一个支持多个客户端的文件传输服务器
3.客户端等待键盘输入文件名称，然后将文件名称传输给服务器，
  服务器在预先设置好的文件夹下查找该文件，如果发现同名文件，开始传输回客户端，
  客户端接收完文件后将文件以输入的文件名称保存在本地某个目录即可，否则告诉客户端文件不存在。
*/
#include <iostream>
#include <fstream>
#include <WinSock2.h>
#include "header.h"
#pragma comment(lib, "ws2_32.lib")
#define CONNECT_NUM_MAX 10

using namespace std;

/*发送服务器支持的命令*/
void SendCmd(SOCKET sClient)
{
	tMSG_CMD cmd;
	sprintf_s(cmd.commad, "Type filename to get a file, /q to exit.\n>");
	send(sClient, reinterpret_cast<char*>(&cmd), sizeof(cmd), 0);
}

/*客户端输入的是文件名，返回文件长度，或文件不存在错误*/
void SendFileLength(SOCKET sClient, tMSG_HEADER *p_msg_header, int id)
{
	tMSG_FILENAME *p_msg_filename = static_cast<tMSG_FILENAME*>(p_msg_header);
	ifstream filein(p_msg_filename->szFileName, ios::in | ios::binary);
	if (!filein)
	{
		//文件不存在
		cout << "[Client " << id << "]<" << p_msg_filename->szFileName << "> File Not Found!\n";
		tMSG_ERROR ErrMsg(MSG_OPENFILE_ERROR);
		send(sClient, reinterpret_cast<char*>(&ErrMsg), sizeof(ErrMsg), 0);
	}
	else
	{
		//返回文件长度
		filein.seekg(0, ios::end);
		long length = filein.tellg();
		cout << "[Client " << id << "]<" << p_msg_filename->szFileName << "> File Length = " << length << endl;
		tMSG_FILE_LENGTH FileLength(length);
		strcpy(FileLength.szFileName, p_msg_filename->szFileName);
		send(sClient, reinterpret_cast<char*>(&FileLength), sizeof(FileLength), 0);
	}
	filein.close();
}

/*传送文件*/
void SendFile(SOCKET sClient, tMSG_HEADER *p_msg_header, int id)
{
	tMSG_CLIENT_READY *p_msg_client_ready = static_cast<tMSG_CLIENT_READY*>(p_msg_header);
	ifstream fin(p_msg_client_ready->szFileName, ios::in | ios::binary);
	if (!fin) { cout << "[Client " << id << "]<" << p_msg_client_ready->szFileName << "> Error Open File!\n"; }

	/*从上次结束的地方开始传送*/
	fin.seekg(p_msg_client_ready->lLastPosition, ios::beg);

	tMSG_FILE tSendFile;
	tSendFile.tFile.lStart = p_msg_client_ready->lLastPosition;
	tSendFile.tFile.lFileLength = p_msg_client_ready->lLength;
	strcpy(tSendFile.tFile.szFileName, p_msg_client_ready->szFileName);

	if (tSendFile.tFile.lFileLength - tSendFile.tFile.lStart > MAX_FILE_SIZE)
	{	//要传送的长度大于一次传送的长度
		tSendFile.tFile.lSize = MAX_FILE_SIZE;
	}
	else
	{	//要传送的文件长度可在这一次内传送完成
		tSendFile.tFile.lSize = tSendFile.tFile.lFileLength - tSendFile.tFile.lStart;
	}

	fin.read(tSendFile.tFile.szBuff, tSendFile.tFile.lSize);
	fin.close();
	//	cout << (int)tSendFile.cMsgId;
//	tMSG_HEADER *p_header = static_cast<tMSG_HEADER*>(&tSendFile);
//	cout << (int)p_header->cMsgId;
	send(sClient, reinterpret_cast<char*>(&tSendFile), sizeof(tSendFile), 0);
	cout << "[Client " << id << "]<" << tSendFile.tFile.szFileName << "> 已发送" << tSendFile.tFile.lSize + tSendFile.tFile.lStart << "/" << tSendFile.tFile.lFileLength << endl;
}

void SendSucc(SOCKET socket, tMSG_HEADER *p_msg_header, int id)
{
	tMSG_SEND_FILE_SUCC *succ = static_cast<tMSG_SEND_FILE_SUCC*>(p_msg_header);
	cout << "[Client " << id << "]<" << succ->szFileName << "> Send File Success!\n";
	SendCmd(socket);
}

/*处理消息，当要断开连接时返回false*/
bool ProcessMsg(SOCKET sClient, int id)
{
	char recvBuf[MAX_PACKET_SIZE];
	//接收消息
	int nRecv = recv(sClient, recvBuf, sizeof(recvBuf) + 1, 0);
	if (nRecv > 0)
	{
		recvBuf[nRecv] = '\0';
	}
	//解析
	tMSG_HEADER *p_msg_header = reinterpret_cast<tMSG_HEADER*>(recvBuf);
	switch (p_msg_header->cMsgId)
	{
	case MSG_HELLO:					SendCmd(sClient);							break;
	case MSG_FILENAME:				SendFileLength(sClient, p_msg_header, id);	break;
	case MSG_CLIENT_READY:			SendFile(sClient, p_msg_header, id);		break;
	case MSG_SEND_FILE_SUCCESS:		SendSucc(sClient, p_msg_header, id);		break;
	case MSG_FILE_ALREADYEXIT_ERROR:break;//覆盖客户端文件，因此这个暂时不需处理
	case MSG_BYE:return false;
	}
	return true;
}

//http://blog.csdn.net/luoweifu/article/details/46835437
#define NAME_LINE   40
//定义线程函数传入参数的结构体
typedef struct __THREAD_DATA
{
	int id;
	SOCKET connSocket;
	SOCKADDR_IN clientAddr;

	__THREAD_DATA(SOCKET socket, SOCKADDR_IN client, int _id = 0)
	{
		connSocket = socket;
		clientAddr = client;
		id = _id;
	}
}THREAD_DATA;

//线程函数
DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	THREAD_DATA* pThreadData = static_cast<THREAD_DATA*>(lpParameter);
	SOCKET connSocket = pThreadData->connSocket;
	SOCKADDR_IN clientAddr = pThreadData->clientAddr;
	int id = pThreadData->id;

	cout << "[Client " << id << "] Connected.已连接.\n";

	SendCmd(connSocket);

	while (true)
	{
		if (!ProcessMsg(connSocket, id))break;
	}
	cout << "[Client " << id << "] Disconnected.断开连接.\n";
	tMSG_BYE p_bye;
	send(connSocket, reinterpret_cast<char*>(&p_bye), sizeof(p_bye), 0);
	closesocket(connSocket);
	return 0L;
}

int main(int argc, char** argv)
{
	int port = 1234;
	if (argc > 1)
	{
		port = atoi(argv[1]);
		if (port < 1024 || port>65534)
		{
			printf("Port error. Use %s port\n", argv[0]);
		}
	}
	//加载套接字库
	WSADATA wsaData;
	int iRet = 0;
	iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iRet != 0)
	{
		cout << "WSAStartup(MAKEWORD(2, 2), &wsaData) execute failed!" << endl;;
		return -1;
	}
	if (2 != LOBYTE(wsaData.wVersion) || 2 != HIBYTE(wsaData.wVersion))
	{
		WSACleanup();
		cout << "WSADATA version is not correct!" << endl;
		return -1;
	}

	//创建套接字
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		cout << "serverSocket = socket(AF_INET, SOCK_STREAM, 0) execute failed!" << endl;
		return -1;
	}

	//初始化服务器地址族变量
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);
	//绑定
	iRet = bind(serverSocket, reinterpret_cast<SOCKADDR*>(&addrSrv), sizeof(SOCKADDR));
	if (iRet == SOCKET_ERROR)
	{
		cout << "bind(serverSocket, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) execute failed!" << endl;
		return -1;
	}


	//监听
	iRet = listen(serverSocket, CONNECT_NUM_MAX);
	if (iRet == SOCKET_ERROR)
	{
		cout << "listen(serverSocket," << CONNECT_NUM_MAX << ") execute failed!" << endl;
		return -1;
	}

	//等待连接_接收_发送
	SOCKADDR_IN clientAddr;
	int len = sizeof(SOCKADDR);
	int count = 0;
	cout << "Server is started\n";
	while (1)
	{
		SOCKET connSocket = accept(serverSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &len);
		if (connSocket == INVALID_SOCKET)
		{
			cout << "accept(serverSocket, (SOCKADDR*)&clientAddr, &len) execute failed!" << endl;
			return -1;
		}

		THREAD_DATA threadData1(connSocket, clientAddr, count++);
		HANDLE thread = CreateThread(nullptr, 0, ThreadProc, &threadData1, 0, nullptr);
		CloseHandle(thread);
		//		//C++ 中 map 容器的说明和使用技巧
		//		//http://www.cnblogs.com/anywei/archive/2011/10/27/2226830.html
		//		g_Map->insert[threadData1.id] = thread;
	}
	return 0;
}