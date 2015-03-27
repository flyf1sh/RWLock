#include "stdafx.h"
#include<windows.h>
#include <cassert>
#include "WRLock.h"

//0:xp 1:win7 2:win8 -1: <xp 
inline int _GetWinVersion()
{
	OSVERSIONINFO  osversioninfo = { sizeof(OSVERSIONINFO) };
	GetVersionEx(&osversioninfo);
	DWORD major = osversioninfo.dwMajorVersion;
	DWORD minor = osversioninfo.dwMinorVersion;
	if(major < 5 || (major == 5 && minor == 0))
		return -1;
	if(major == 5 && minor == 1)
		return 0;
	if(major == 6 && minor == 0)	//vista
		return 1;
	if(major == 6 && minor == 1)	//win7
		return 1;
	return 2;
}

inline int _GetRunningSystem()
{
	int runver = _GetWinVersion();
	if(runver < 1) return runver;
	//return 0;
#if(_WIN32_WINNT >= 0x600)	
	return 1;	//win7
#else
	return 0;
#endif
}

//����������е�ϵͳ��xp��Ϊ0��ֻ�������ұ����ϵͳΪwin7ʱΪ1
int _running_ver = _GetRunningSystem();


class CKSRWLock
{
public:
	void LockWrite()
	{
		// д�ȴ��ļ�������
		assert(m_refWriteWaitCount >= 0);
		InterlockedIncrement(&m_refWriteWaitCount);
		// �ٽ���
		EnterCriticalSection(&m_csWrite);
		// ���ɶ�
		ResetEvent(m_eventReadEnable);
		// ����
		InterlockedExchangeAdd(&m_refCount, -m_clMAX_READ);
		// �ȴ����ж����
		while (m_refCount < 0)
			SwitchToThread();
	}
	void UnlockWrite()
	{
		// ����
		LONG init = InterlockedExchangeAdd(&m_refCount, m_clMAX_READ);

		// д�ȴ��ļ�������
		LONG value = InterlockedDecrement(&m_refWriteWaitCount);
		assert(value >= 0);

		// �ɶ�
		SetEvent(m_eventReadEnable);
		// �ٽ���
		LeaveCriticalSection(&m_csWrite);
	}

	// ���ز��������߳���
	LONG LockRead()
	{
		DWORD dwRes;
		// ��������
		while (true)
		{
			if (0 == m_refWriteWaitCount)
			{
				// ����
				LONG iResult = InterlockedDecrement(&m_refCount);
				if (iResult > 0)
				{
					// Lock : n -> (n - 1)
					if (0 == m_refWriteWaitCount)
						return m_clMAX_READ - iResult;
				}
				InterlockedIncrement(&m_refCount);
			}
			// �����ں˵ȴ�
			dwRes = WaitForSingleObject(m_eventReadEnable, INFINITE);
			if (dwRes == WAIT_ABANDONED || dwRes == WAIT_FAILED)
			{//ʧ��
				return 0;
			}
		}
	}

	void UnlockRead()
	{
		// ����
		InterlockedIncrement(&m_refCount);
	}

public:
	CKSRWLock(void):m_refCount(m_clMAX_READ),m_refWriteWaitCount(0)
	{
		InitializeCriticalSection(&m_csWrite);
		m_eventReadEnable = ::CreateEvent(NULL, TRUE, TRUE, NULL);
		SetEvent(m_eventReadEnable);
	}
	~CKSRWLock(void)
	{
		try
		{
			DeleteCriticalSection(&m_csWrite);
			CloseHandle(m_eventReadEnable);
		}
		catch(...)
		{
		}
	}

protected:
	// ��󲻻ᳬ�� m_clMAX_READ ��������
	static const LONG	m_clMAX_READ = 1000000;
	// m_refCount: using interlocked funtions to access
	// 100000000		- free
	// 0				- one thread is writing
	// 100000000 - n	- n threads are reading
	volatile LONG		m_refCount;
	// n	- n threads are waiting to wirte
	volatile LONG		m_refWriteWaitCount;	
	// once a thread to write
	CRITICAL_SECTION	m_csWrite;
	// wait for writing complete
	HANDLE				m_eventReadEnable;
};



CWRLock::CWRLock(
		 _Inout_ HANDLE wrlock,
		 _In_ bool bInitialLock):
	m_wrlock(wrlock),
	m_iLocked(0)
{
	bInitialLock ? RLock() : WLock();
}
CWRLock::~CWRLock() throw()
{
	if(m_iLocked != 0)
		Unlock();
}
void CWRLock::RLock()
{
	if(_running_ver <= 0) //xp
	{
		CKSRWLock * wr = (CKSRWLock *) m_wrlock;
		wr->LockRead();
	}
	else{
#if(_WIN32_WINNT >= 0x600)	
		AcquireSRWLockShared((SRWLOCK*)m_wrlock);
#endif
	}
	m_iLocked = 1;
}
void CWRLock::WLock()
{
	if(_running_ver <= 0) //xp
	{
		CKSRWLock * wr = (CKSRWLock *) m_wrlock;
		wr->LockWrite();
	}
	else{
#if(_WIN32_WINNT >= 0x600)	
		AcquireSRWLockExclusive((SRWLOCK*)m_wrlock);
#endif
	}
	m_iLocked = -1;
}
void CWRLock::Unlock() throw()
{
	if(_running_ver <= 0) //xp
	{
		CKSRWLock * wr = (CKSRWLock *) m_wrlock;
		if(m_iLocked == 1)
			wr->UnlockRead();
		else if(m_iLocked == -1)
			wr->UnlockWrite();
	}
	else{
#if(_WIN32_WINNT >= 0x600)	
		if(m_iLocked == 1)
			ReleaseSRWLockShared((SRWLOCK*)m_wrlock);
		else if(m_iLocked == -1)
			ReleaseSRWLockExclusive((SRWLOCK*)m_wrlock);
#endif
	}
	m_iLocked = 0;
}

HANDLE GetWRLock()
{
	if(_running_ver <= 0) //xp
	{
		return new CKSRWLock();
	}
	else
	{
#if(_WIN32_WINNT >= 0x600)	
		SRWLOCK * wr = new SRWLOCK();
		InitializeSRWLock(wr);
		return (PVOID)wr;
#endif
	}
	assert(0);	//nerver be here
	return NULL;
}
void DestroyWRLock(HANDLE lock)
{
	delete lock;
}

