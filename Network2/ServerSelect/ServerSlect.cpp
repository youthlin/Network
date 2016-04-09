//http://www.cnblogs.com/lidabo/p/3804411.html
//windows 和 linux 套接字中的 select 机制浅析

// server.cpp :   
// 程序中加入了套接字管理队列，这样管理起来更加清晰、方便，当然也可以不用这个东西  

#include "winsock.h"  
#include "stdio.h"
#include <ctime>
#pragma comment (lib,"wsock32.lib")  
#define MAX 64

//Socket列表
struct socket_list {
	SOCKET MainSock;
	int num;
	SOCKET sock_array[MAX];
};
//初始化列表
void init_list(socket_list *list)
{
	int i;
	list->MainSock = 0;
	list->num = 0;
	for (i = 0; i < MAX; i++) {
		list->sock_array[i] = 0;
	}
}
//插入一个Socket到列表
void insert_list(SOCKET s, socket_list *list)
{
	int i;
	for (i = 0; i < MAX; i++) {
		if (list->sock_array[i] == 0) {
			list->sock_array[i] = s;
			list->num += 1;
			break;
		}
	}
}
//从列表删除一个Socket
void delete_list(SOCKET s, socket_list *list)
{
	int i;
	for (i = 0; i < MAX; i++) {
		if (list->sock_array[i] == s) {
			list->sock_array[i] = 0;
			list->num -= 1;
			break;
		}
	}
}

void make_fdlist(socket_list *list, fd_set *fd_list)
{
	int i;
	//FD_SET(s,*set)，向集合中加入一个套接口描述符
	//（如果该套接口描述符 s 没在集合中，并且数组中已经设置的个数小于最大个数时，就把该描述符加入到集合中，集合元素个数加 1）。
	//这里是将 s 的值直接放入数组中。
	FD_SET(list->MainSock, fd_list);
	for (i = 0; i < MAX; i++) {
		if (list->sock_array[i] > 0) {
			FD_SET(list->sock_array[i], fd_list);
		}
	}
}

/*
1. 调用 FD_ZERO 来初始化套接字状态；
2. 调用 FD_SET 将感兴趣的套接字描述符加入集合中（每次循环都要重新加入，因为 select 更新后，会将一些没有满足条件的套接字移除队列）；
3. 设置等待时间后，调用 select 函数 -- 更新套接字的状态；
4. 调用 FD_ISSET，来判断套接字是否有相应状态，然后做相应操作，比如，如果套接字可读，就调用 recv 函数去接收数据。
关键技术：套接字队列和状态的表示与处理。*/

int main(int argc, char* argv[])
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
	//s是Server,sock是客户端
	SOCKET s, sock;
	struct sockaddr_in ser_addr, remote_addr;
	int len;
	char buf[128];
	WSAData wsa;
	int retval;
	struct socket_list sock_list;
	fd_set readfds, writefds, exceptfds;
	timeval timeout;        //select 的最多等待时间，防止一直等待  
	int i;
	unsigned long arg;

	WSAStartup(0x101, &wsa);
	s = socket(AF_INET, SOCK_STREAM, 0);
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	ser_addr.sin_port = htons(port);
	bind(s, reinterpret_cast<sockaddr*>(&ser_addr), sizeof(ser_addr));

	listen(s, 5);
	timeout.tv_sec = 5;     // 如果套接字集合中在 1s 内没有数据，select 就会返回，超时 select 返回 0  
	timeout.tv_usec = 0;
	init_list(&sock_list);

	//FD_ZERO(*set)，是把集合清空（初始化为 0，确切的说，是把集合中的元素个数初始化为 0，并不修改描述符数组). 
	//使用集合前，必须用 FD_ZERO 初始化，否则集合在栈上作为自动变量分配时，fd_set 分配的将是随机值，导致不可预测的问题。
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	sock_list.MainSock = s;
	arg = 1;
	ioctlsocket(sock_list.MainSock, FIONBIO, &arg);

	while (1) {
		make_fdlist(&sock_list, &readfds);
		//make_fdlist(&sock_list,&writefds);  
		//make_fdlist(&sock_list,&exceptfds);  

		retval = select(0, &readfds, &writefds, &exceptfds, &timeout);// 超过这个时间，就不阻塞在这里，返回一个 0 值。  
		if (retval == SOCKET_ERROR) {
			retval = WSAGetLastError();
			printf("error code=%d", retval);
			break;
		}
		if (retval == 0) {
			//printf("select() is time-out! There is no data or new-connect coming!\n");
			continue;
		}

		char sendBuf[128];
		//FD_ISSET(s,*set)，检查描述符是否在集合中，如果在集合中返回非 0 值，否则返回 0.
		//它的宏定义并没有给出具体实现，但实现的思路很简单，就是搜索集合，判断套接字 s 是否在数组中。
		if (FD_ISSET(sock_list.MainSock, &readfds)) {
			len = sizeof(remote_addr);
			sock = accept(sock_list.MainSock, reinterpret_cast<sockaddr*>(&remote_addr), &len);
			if (sock == SOCKET_ERROR)
				continue;
			printf("accept a connection\n");
			insert_list(sock, &sock_list);

			memset(sendBuf, 0, sizeof(sendBuf));
			sprintf_s(sendBuf, "Welcome.  type 1 to get time,0 to exit.");
			send(sock, sendBuf, strlen(sendBuf) + 1, 0);

		}
		for (i = 0; i < MAX; i++) {
			if (sock_list.sock_array[i] == 0)
				continue;
			sock = sock_list.sock_array[i];

			if (FD_ISSET(sock, &readfds)) {
				//接受客户端数据，返回数据长度
				retval = recv(sock, buf, 128, 0);
				if (retval == 0) {
					closesocket(sock);
					printf("close a socket\n");
					delete_list(sock, &sock_list);
					continue;
				}
				if (retval == -1) {
					retval = WSAGetLastError();
					if (retval == WSAEWOULDBLOCK)
						continue;
					closesocket(sock);
					printf("close a socket\n");
					delete_list(sock, &sock_list);   // 连接断开后，从队列中移除该套接字  
					continue;
				}
				buf[retval] = 0;
				printf("->%s\n", buf);

				memset(sendBuf, 0, sizeof(sendBuf));
				if (strcmp(buf, "date") == 0)
				{
					time_t t = time(nullptr);
					strftime(sendBuf, sizeof(sendBuf), "%Y/%m/%d %X %A 本年第 %j 天 %z", localtime(&t));
					send(sock, sendBuf, strlen(sendBuf) + 1, 0);
				}
				else if (strcmp(buf, "q") == 0)
				{
					sprintf_s(sendBuf, "BYE");
					send(sock, sendBuf, strlen(sendBuf) + 1, 0);
					closesocket(sock);
					printf("close a socket\n");
					delete_list(sock, &sock_list);
				}
				else
				{
					sprintf_s(sendBuf, "Type 1 to get time,0 to exit.");
					send(sock, sendBuf, strlen(sendBuf) + 1, 0);
				}
			}
			//if(FD_ISSET(sock,&writefds)){  
			//}  
			//if(FD_ISSET(sock,&exceptfds)){  

		}
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);
	}
	closesocket(sock_list.MainSock);
	WSACleanup();
	return 0;
}