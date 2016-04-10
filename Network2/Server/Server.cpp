#include<iostream>
#include<WinSock2.h>
#include<ctime>
#include<vector>
#pragma comment(lib, "ws2_32.lib")
//#pragma comment(lib, "Ws2_32.lib")
#define CONNECT_NUM_MAX 10
#define _WINSOCK_DEPRECATED_NO_WARNINGS
using namespace std;


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

	printf("客户端%d已连接\n", id);
	char sendBuf[100];
	memset(sendBuf, 0, sizeof(sendBuf));
	sprintf_s(sendBuf, "Welcome %s, You are No.%d.  type date to get time,q to exit.", inet_ntoa(clientAddr.sin_addr), id);
	send(connSocket, sendBuf, strlen(sendBuf) + 1, 0);
	char recvBuf[100];
	while (true)
	{
		memset(recvBuf, 0, sizeof(recvBuf));
		recv(connSocket, recvBuf, 100, 0);
		printf("[客户端%d命令] %s\n", id, recvBuf);
		if (strcmp(recvBuf, "date") == 0)
		{
			time_t t = time(0);
			//http://www.cnblogs.com/mfryf/archive/2012/02/13/2349360.html
			strftime(sendBuf, sizeof(sendBuf), "%Y/%m/%d %X %A 本年第 %j 天 %z", localtime(&t));
			send(connSocket, sendBuf, strlen(sendBuf) + 1, 0);
		}
		else if (strcmp(recvBuf, "q") == 0) {
			sprintf_s(sendBuf, "BYE");
			send(connSocket, sendBuf, strlen(sendBuf) + 1, 0);
			closesocket(connSocket);
			//CloseHandle()
			break;
		}
		else {
			sprintf_s(sendBuf, "Type date to get time,q to exit.");
			send(connSocket, sendBuf, strlen(sendBuf) + 1, 0);
		}
	}
	return 0L;
}

//http://www.js-code.com/C_yuyan/20150918/4488.html
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
	vector<HANDLE> threads;
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
		threads.push_back(thread);
	}
	return 0;
}