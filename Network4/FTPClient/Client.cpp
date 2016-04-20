#include <iostream>
#include <fstream>
#include <string>
#include <winsock2.h>
#include "../FTPServer/header.h"
using namespace std;
#define BYE "BYE"
#pragma comment(lib, "ws2_32.lib")

void ReplayCMD(SOCKET socket, tMSG_HEADER *p_msg_header)
{
	string sClientCommad;
	tMSG_CMD *p_cmd = static_cast<tMSG_CMD*>(p_msg_header);
	cout << p_cmd->commad;
	cin >> sClientCommad;
	if (strcmp(sClientCommad.c_str(), "/q") == 0)
	{
		//cout << "客户端输入的是/q\n";
		/*断开连接*/
		tMSG_BYE p_bye;
		send(socket, reinterpret_cast<char*>(&p_bye), sizeof(p_bye), 0);
	}
	else
	{
		//cout << "客户端输入的是文件名:" << sClientCommad << endl;
		/*发送文件名*/
		tMSG_FILENAME p_filename;
		strcpy(p_filename.szFileName, sClientCommad.c_str());
		send(socket, reinterpret_cast<char*>(&p_filename), sizeof(p_filename), 0);
	}
}

void SayHello(SOCKET socket)
{
	tMSG_HELLO hello;
	send(socket, reinterpret_cast<char*>(&hello), sizeof(hello), 0);
}

void ReadyToRecvFile(SOCKET socket, tMSG_HEADER *p_msg_header)
{
	tMSG_FILE_LENGTH *p_file_length = static_cast<tMSG_FILE_LENGTH*>(p_msg_header);
	cout << "[" << p_file_length->szFileName << "] Length=" << p_file_length->lLength << " ";
	/*若文件已存在则先删除trunc*/
	ofstream fout(p_file_length->szFileName, ios::out | ios::trunc);
	if (!fout) { cout << "[" << p_file_length->szFileName << "] Open File Error!\n"; }
	fout.close();
	cout << "Ready to recv file... 准备接收文件...\n";
	tMSG_CLIENT_READY ready(0L, p_file_length->lLength);
	strcpy(ready.szFileName, p_file_length->szFileName);
	send(socket, reinterpret_cast<char*>(&ready), sizeof(ready), 0);
}

void RecvFile(SOCKET socket, tMSG_HEADER *p_msg_header)
{
	tMSG_FILE *p_msg_file = static_cast<tMSG_FILE*>(p_msg_header);
	long start = p_msg_file->tFile.lStart;
	long sum = p_msg_file->tFile.lFileLength;
	long size = p_msg_file->tFile.lSize;
	cout << "[" << p_msg_file->tFile.szFileName << "] Recveived: " << start + size << "/" << sum << ", ";
	ofstream fout(p_msg_file->tFile.szFileName, ios::out | ios::binary | ios::app);
	fout.write(p_msg_file->tFile.szBuff, p_msg_file->tFile.lSize);
	cout << "File Written.已写入 ";
	if (start + size < sum)//没有接收完
	{
		cout << "准备接受余下内容\n";
		tMSG_CLIENT_READY ready(start + size, sum);
		strcpy(ready.szFileName, p_msg_file->tFile.szFileName);
		send(socket, reinterpret_cast<char*>(&ready), sizeof(ready), 0);
	}
	else//接收完毕
	{
		cout << "All Recveived !已接收\n";
		tMSG_SEND_FILE_SUCC succ;
		strcpy(succ.szFileName, p_msg_file->tFile.szFileName);
		send(socket, reinterpret_cast<char*>(&succ), sizeof(succ), 0);
	}
	fout.close();
}

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

	char szBuff[MAX_PACKET_SIZE + 1];
	while (true)
	{
		memset(szBuff, 0, sizeof(szBuff));
		//接收消息
		int nRecv = recv(clientSocket, szBuff, MAX_PACKET_SIZE, 0);
		//		cout << "接收数据长度=" << nRecv << endl;
		if (nRecv > 0)
		{
			szBuff[nRecv] = '\0';
		}
		else
		{
			closesocket(clientSocket);
			WSACleanup();
			return 0;
		}
		//解析
		tMSG_HEADER *p_msg_header = reinterpret_cast<tMSG_HEADER*>(szBuff);
		//		cout << (int)p_msg_header->cMsgId;
		switch (p_msg_header->cMsgId)
		{
		case MSG_CMD:/*服务器命令*/
			ReplayCMD(clientSocket, p_msg_header);
			break;
		case MSG_OPENFILE_ERROR:/*文件不存在*/
			cout << "File Not Found!服务器上无此文件\n";
			SayHello(clientSocket);
			break;
		case MSG_FILELENGTH:/*文件存在，客户端准备接收*/
			ReadyToRecvFile(clientSocket, p_msg_header);
			break;
		case MSG_FILE:/*接收文件*/
			RecvFile(clientSocket, p_msg_header);
			break;
		case MSG_BYE:
			closesocket(clientSocket);
			WSACleanup();
			return 0;
		default:
			cout << "Invalid data.\n";
			//SayHello(clientSocket);

			break;
		}
		//		Sleep(500);
	}
}