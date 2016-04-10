#include<iostream>
#include<WinSock2.h>
#include<ctime>
#pragma comment(lib, "ws2_32.lib")
//#pragma comment(lib, "Ws2_32.lib")
#define CONNECT_NUM_MAX 10
#define _WINSOCK_DEPRECATED_NO_WARNINGS
using namespace std;


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
	iRet = bind(serverSocket, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
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
	//while (1)
	//{
	SOCKET connSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &len);
	if (connSocket == INVALID_SOCKET)
	{
		cout << "accept(serverSocket, (SOCKADDR*)&clientAddr, &len) execute failed!" << endl;
		return -1;
	}
	// 'inet_ntoa': Use inet_ntop() or InetNtop() instead or define _WINSOCK_DEPRECATED_NO_WARNINGS to disable deprecated API warnings
	// http://stackoverflow.com/questions/26947496/deprecated-commands-in-visual-c 使用#include<Ws2tcpip.h>下的inet_ntop()
	// 属性——C/C＋＋——常规——SDK检查 改为否
	// http://jingyan.baidu.com/article/1709ad8097e5904634c4f03e.html

	char sendBuf[100];
	memset(sendBuf, 0, sizeof(sendBuf));
	sprintf_s(sendBuf, "Welcome %s.  type date to get time,q to exit.", inet_ntoa(clientAddr.sin_addr));
	send(connSocket, sendBuf, strlen(sendBuf) + 1, 0);
	char recvBuf[100];
	while (true)
	{
		memset(recvBuf, 0, sizeof(recvBuf));
		recv(connSocket, recvBuf, 100, 0);
		printf("%s\n", recvBuf);
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
			exit(0);
		}
		else {
			sprintf_s(sendBuf, "Type date to get time,q to exit.");
			send(connSocket, sendBuf, strlen(sendBuf) + 1, 0);
		}
	}

	//}

	system("pause");
	return 0;
}