// rwlock.cpp: ����Ŀ�ļ���

#include "stdafx.h"

//������д������� ��д��SRWLock
#include <stdio.h>
#include <process.h>
#include <windows.h>
#include "WRLock.h"
#include <iostream>

//���ÿ���̨�����ɫ
inline BOOL SetConsoleColor(WORD wAttributes)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsole == INVALID_HANDLE_VALUE)
		return FALSE;
	return SetConsoleTextAttribute(hConsole, wAttributes);
}
const int READER_NUM = 7;  //���߸���
//�ؼ��κ��¼�
CRITICAL_SECTION g_cs;
HANDLE  g_srwLock;

//�����߳��������(��κ�����ʵ��)
inline void ReaderPrintf(char *pszFormat, ...)
{
	va_list   pArgList;
	va_start(pArgList, pszFormat);
	EnterCriticalSection(&g_cs);
	vfprintf(stdout, pszFormat, pArgList);
	LeaveCriticalSection(&g_cs);
	va_end(pArgList);
}
//�����̺߳���
unsigned int __stdcall ReaderThreadFun(PVOID pM)
{
	ReaderPrintf("     ���Ϊ%d�Ķ��߽���ȴ���...\n", GetCurrentThreadId());
	//���������ȡ�ļ�
	CWRLock lock(g_srwLock);
	//��ȡ�ļ�
	ReaderPrintf("���Ϊ%d�Ķ��߿�ʼ��ȡ�ļ�...\n", GetCurrentThreadId());
	Sleep(3000+rand() % 2000);
	ReaderPrintf(" ���Ϊ%d�Ķ��߽�����ȡ�ļ�\n", GetCurrentThreadId());
	lock.Unlock();
	return 0;
}
//д���߳��������
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
//д���̺߳���
unsigned int __stdcall WriterThreadFun(PVOID pM)
{
	WriterPrintf("���Ϊ%dд���߳̽���ȴ���...\n", GetCurrentThreadId());
	//д������д�ļ�
	CWRLock lock(g_srwLock, false);
	//д�ļ�
	WriterPrintf("  ���Ϊ%dд�߿�ʼд�ļ�.....\n", GetCurrentThreadId());
	Sleep(3000+ rand() % 3000);
	WriterPrintf("  ���Ϊ%dд�߽���д�ļ�\n", GetCurrentThreadId());
	lock.Unlock();
	//���д�߽���д�ļ�
	return 0;
}
int main()
{
	printf("  ����д������ ��д��RWLock\n");
	//��ʼ����д���͹ؼ���
	InitializeCriticalSection(&g_cs);
	g_srwLock = GetWRLock();
	HANDLE hThread[READER_NUM + 1];
	int i;
	//���������������߳�
	for (i = 0; i <= 1; i++)
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, ReaderThreadFun, NULL, 0, NULL);
	Sleep(1000);
	//��������д���߳�
	for(i = 2; i <= 4; i++ )
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WriterThreadFun, NULL, 0, NULL);
	Sleep(1000);
	//��������������߽��
	for ( ; i <= READER_NUM; i++)
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, ReaderThreadFun, NULL, 0, NULL);
	WaitForMultipleObjects(READER_NUM + 1, hThread, TRUE, INFINITE);
	for (i = 0; i < READER_NUM + 1; i++)
		CloseHandle(hThread[i]);
	//���ٹؼ���
	DeleteCriticalSection(&g_cs);
	DestroyWRLock(g_srwLock);
	char c;
	std::cin >> c;
	return 0;
}

