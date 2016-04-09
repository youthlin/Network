#include<iostream>
#include <winsock2.h>
using namespace std;
#define BYE "BYE"
#pragma comment(lib, "ws2_32.lib")
int main(int argc, char** argv)
{
	string host = "127.0.0.1";
	int port = 1234;
	if (argc == 3)
	{
		host = argv[1];
		port = atoi(argv[2]);
		if (port < 1024 || port>65534)
		{
			printf("Port error.Use %s host port\n", argv[0]);
		}
	}
	else if (argc != 1)
	{
		printf("Error. Use %s host port\n", argv[0]);
	}
	//加载套接字库
	WSADATA wsaData;
	int iRet = 0;
	iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iRet != 0)
	{
		cout << "WSAStartup(MAKEWORD(2, 2), &wsaData) execute failed!" << endl;
		return -1;
	}
	if (2 != LOBYTE(wsaData.wVersion) || 2 != HIBYTE(wsaData.wVersion))
	{
		WSACleanup();
		cout << "WSADATA version is not correct!" << endl;
		return -1;
	}

	//创建套接字
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		cout << "clientSocket = socket(AF_INET, SOCK_STREAM, 0) execute failed!" << endl;
		return -1;
	}

	//初始化服务器端地址族变量
	SOCKADDR_IN srvAddr;
	srvAddr.sin_addr.S_un.S_addr = inet_addr(host.c_str());
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(port);

	//连接服务器
	iRet = connect(clientSocket, (SOCKADDR*)&srvAddr, sizeof(SOCKADDR));
	if (0 != iRet)
	{
		cout << "connect(clientSocket, (SOCKADDR*)&srvAddr, sizeof(SOCKADDR)) execute failed!" << endl;
		return -1;
	}

	char recvBuf[100];
	char sendBuf[100];
	int count = 0;
	while (true)
	{
		memset(recvBuf, 0, sizeof(recvBuf));
		memset(sendBuf, 0, sizeof(sendBuf));
		//接收消息
		recv(clientSocket, recvBuf, 100, 0);
		printf("%s\n", recvBuf);
		if (strcmp(recvBuf, BYE) == 0)
		{
			//清理
			closesocket(clientSocket);
			WSACleanup();
			break;
		}
		scanf("%d", &count);

		if (count == 1)
			sprintf(sendBuf, "date");
		else if (count==0)
			sprintf(sendBuf, "q");
		else
			sprintf(sendBuf, "unknow");
		count++;
		send(clientSocket, sendBuf, strlen(sendBuf) + 1, 0);
	}
	return 0;
}