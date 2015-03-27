// rwlock.cpp: 主项目文件。

#include "stdafx.h"

//读者与写者问题继 读写锁SRWLock
#include <stdio.h>
#include <process.h>
#include <windows.h>
#include "WRLock.h"
#include <iostream>

//设置控制台输出颜色
inline BOOL SetConsoleColor(WORD wAttributes)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsole == INVALID_HANDLE_VALUE)
		return FALSE;
	return SetConsoleTextAttribute(hConsole, wAttributes);
}
const int READER_NUM = 7;  //读者个数
//关键段和事件
CRITICAL_SECTION g_cs;
HANDLE  g_srwLock;

//读者线程输出函数(变参函数的实现)
inline void ReaderPrintf(char *pszFormat, ...)
{
	va_list   pArgList;
	va_start(pArgList, pszFormat);
	EnterCriticalSection(&g_cs);
	vfprintf(stdout, pszFormat, pArgList);
	LeaveCriticalSection(&g_cs);
	va_end(pArgList);
}
//读者线程函数
unsigned int __stdcall ReaderThreadFun(PVOID pM)
{
	ReaderPrintf("     编号为%d的读者进入等待中...\n", GetCurrentThreadId());
	//读者申请读取文件
	CWRLock lock(g_srwLock);
	//读取文件
	ReaderPrintf("编号为%d的读者开始读取文件...\n", GetCurrentThreadId());
	Sleep(3000+rand() % 2000);
	ReaderPrintf(" 编号为%d的读者结束读取文件\n", GetCurrentThreadId());
	lock.Unlock();
	return 0;
}
//写者线程输出函数
inline void WriterPrintf(char *pszFormat, ...)
{
	va_list   pArgList;
	va_start(pArgList, pszFormat);
	EnterCriticalSection(&g_cs);
	SetConsoleColor(FOREGROUND_GREEN);
	vfprintf(stdout, pszFormat, pArgList);
	SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	LeaveCriticalSection(&g_cs);
}
//写者线程函数
unsigned int __stdcall WriterThreadFun(PVOID pM)
{
	WriterPrintf("编号为%d写者线程进入等待中...\n", GetCurrentThreadId());
	//写者申请写文件
	CWRLock lock(g_srwLock, false);
	//写文件
	WriterPrintf("  编号为%d写者开始写文件.....\n", GetCurrentThreadId());
	Sleep(3000+ rand() % 3000);
	WriterPrintf("  编号为%d写者结束写文件\n", GetCurrentThreadId());
	lock.Unlock();
	//标记写者结束写文件
	return 0;
}
int main()
{
	printf("  读者写者问题 读写锁RWLock\n");
	//初始化读写锁和关键段
	InitializeCriticalSection(&g_cs);
	g_srwLock = GetWRLock();
	HANDLE hThread[READER_NUM + 1];
	int i;
	//先启动二个读者线程
	for (i = 0; i <= 1; i++)
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, ReaderThreadFun, NULL, 0, NULL);
	Sleep(1000);
	//启动三个写者线程
	for(i = 2; i <= 4; i++ )
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WriterThreadFun, NULL, 0, NULL);
	Sleep(1000);
	//最后启动其它读者结程
	for ( ; i <= READER_NUM; i++)
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, ReaderThreadFun, NULL, 0, NULL);
	WaitForMultipleObjects(READER_NUM + 1, hThread, TRUE, INFINITE);
	for (i = 0; i < READER_NUM + 1; i++)
		CloseHandle(hThread[i]);
	//销毁关键段
	DeleteCriticalSection(&g_cs);
	DestroyWRLock(g_srwLock);
	char c;
	std::cin >> c;
	return 0;
}

