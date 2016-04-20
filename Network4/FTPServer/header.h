#pragma once
#ifndef SEND_FILE_HEADER__
#define SEND_FILE_HEADER__

#define MAX_PACKET_SIZE				10240	// 数据包的最大长度, 单位是 sizeof(char)
#define	FILENAME_LENGTH				256		//文件名长度
#define MAX_FILE_SIZE				((MAX_PACKET_SIZE)-3*sizeof(long)-FILENAME_LENGTH-sizeof(char))
#define CMD_LENGTH					256		//命令描述的最大长度
// 各种消息的宏定义  
#define INVALID_MSG					-1  // 无效的消息标识
#define MSG_CMD						1	// 客户端刚连接时，服务器发送支持的命令
#define MSG_FILENAME				2   // 文件的名称  
#define MSG_FILELENGTH				3   // 传送文件的长度  
#define MSG_CLIENT_READY			4   // 客户端准备接收文件  
#define MSG_FILE					5   // 传送文件  
#define MSG_SEND_FILE_SUCCESS		6   // 传送文件成功  
#define MSG_OPENFILE_ERROR			7   // 打开文件失败, 可能是文件路径错误找不到文件等原因  
#define MSG_FILE_ALREADYEXIT_ERROR	8   // 要保存的文件已经存在了 
#define MSG_BYE						9	//断开
#define MSG_HELLO					10	//客户端发送Hello

#pragma pack(1)
/*消息头*/
struct tMSG_HEADER
{
	/*消息标识*/
	char cMsgId;
	tMSG_HEADER(char id = INVALID_MSG) :cMsgId(id) {};
};

struct tMSG_HELLO :tMSG_HEADER
{
	tMSG_HELLO() :tMSG_HEADER(MSG_HELLO) {}
};
/*服务器发送的命令*/
struct tMSG_CMD :tMSG_HEADER
{
	char commad[CMD_LENGTH];
	tMSG_CMD() :tMSG_HEADER(MSG_CMD) {}
};

/*传送的是文件名*/
struct tMSG_FILENAME :tMSG_HEADER
{
	/*文件名*/
	char szFileName[FILENAME_LENGTH];
	tMSG_FILENAME() :tMSG_HEADER(MSG_FILENAME) {}
};

/*传送的是文件长度*/
struct tMSG_FILE_LENGTH :tMSG_HEADER
{
	/*文件长度*/
	long lLength;
	char szFileName[FILENAME_LENGTH];
	tMSG_FILE_LENGTH(long l) :tMSG_HEADER(MSG_FILELENGTH), lLength(l) {}
};

/*客户端准备好了*/
struct tMSG_CLIENT_READY :tMSG_HEADER
{
	long lLastPosition, lLength;
	char szFileName[FILENAME_LENGTH];
	tMSG_CLIENT_READY(long l, long len) :tMSG_HEADER(MSG_CLIENT_READY), lLastPosition(l), lLength(len) {}
};

/*接下来传送的是文件*/
struct tMSG_FILE :tMSG_HEADER
{
	union // 采用 union 保证了数据包的大小不大于 MAX_PACKET_SIZE * sizeof(char)
	{
		char szBuff[MAX_PACKET_SIZE - sizeof(char)];///<strong>父结构体还有一个char类型的MsgId！！！</strong>
		struct
		{
			long lStart;
			long lSize;
			long lFileLength;
			char szFileName[FILENAME_LENGTH];
			char szBuff[MAX_FILE_SIZE];
		}tFile;
	};
	tMSG_FILE() : tMSG_HEADER(MSG_FILE) {}
};

/*传送文件成功*/
struct tMSG_SEND_FILE_SUCC :tMSG_HEADER
{
	char szFileName[FILENAME_LENGTH];
	tMSG_SEND_FILE_SUCC() :tMSG_HEADER(MSG_SEND_FILE_SUCCESS) {}
};

/*错误消息*/
struct tMSG_ERROR :tMSG_HEADER
{
	tMSG_ERROR(char ErrType) :tMSG_HEADER(ErrType) {}
};

/*断开*/
struct tMSG_BYE :tMSG_HEADER
{
	tMSG_BYE() :tMSG_HEADER(MSG_BYE) {}
};
#pragma pack()
#endif
