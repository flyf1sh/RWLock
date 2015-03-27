#include "stdafx.h"
#include<windows.h>
#include <cassert>
#include "WRLock.h"

//xp
#define _WIN32_WINNT_VER 0x501
//win7, vista
//#define _WIN32_WINNT_VER 0x600

#if(_WIN32_WINNT_VER < 0x600)	
//xp
class CKSRWLock
{
public:
	void LockWrite()
	{
		// 写等待的计数变量
		assert(m_refWriteWaitCount >= 0);
		InterlockedIncrement(&m_refWriteWaitCount);
		// 临界区
		EnterCriticalSection(&m_csWrite);
		// 不可读
		ResetEvent(m_eventReadEnable);
		// 互斥
		InterlockedExchangeAdd(&m_refCount, -m_clMAX_READ);
		// 等待所有读完成
		while (m_refCount < 0)
			SwitchToThread();
	}
	void UnlockWrite()
	{
		// 互斥
		LONG init = InterlockedExchangeAdd(&m_refCount, m_clMAX_READ);

		// 写等待的计数变量
		LONG value = InterlockedDecrement(&m_refWriteWaitCount);
		assert(value >= 0);

		// 可读
		SetEvent(m_eventReadEnable);
		LeaveCriticalSection(&m_csWrite);
	}

	// 返回并发读的线程数
	LONG LockRead()
	{
		DWORD dwRes;
		while (true)
		{
			if (0 == m_refWriteWaitCount)
			{ // 互斥
				LONG iResult = InterlockedDecrement(&m_refCount);
				if (iResult > 0)
				{
					// Lock : n -> (n - 1)
					if (0 == m_refWriteWaitCount)
						return m_clMAX_READ - iResult;
				}
				InterlockedIncrement(&m_refCount);
			}
			// 进入内核等待
			dwRes = WaitForSingleObject(m_eventReadEnable, INFINITE);
			if (dwRes == WAIT_ABANDONED || dwRes == WAIT_FAILED)//失败
				return 0;
		}
	}

	void UnlockRead()
	{ // 互斥
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
	// 最大不会超过 m_clMAX_READ 个并发读
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
	CKSRWLock * wr = (CKSRWLock *) m_wrlock;
	wr->LockRead();
	m_iLocked = 1;
}
void CWRLock::WLock()
{
	CKSRWLock * wr = (CKSRWLock *) m_wrlock;
	wr->LockWrite();
	m_iLocked = -1;
}
void CWRLock::Unlock() throw()
{
	CKSRWLock * wr = (CKSRWLock *) m_wrlock;
	if(m_iLocked == 1)
		wr->UnlockRead();
	else if(m_iLocked == -1)
		wr->UnlockWrite();
	m_iLocked = 0;
}

HANDLE GetWRLock()
{
	return new CKSRWLock();
}
void DestroyWRLock(HANDLE lock)
{
	delete lock;
}
#else
//win7

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
	AcquireSRWLockShared((SRWLOCK*)m_wrlock);
	m_iLocked = 1;
}
void CWRLock::WLock()
{
	AcquireSRWLockExclusive((SRWLOCK*)m_wrlock);
	m_iLocked = -1;
}
void CWRLock::Unlock() throw()
{
	if(m_iLocked == 1)
		ReleaseSRWLockShared((SRWLOCK*)m_wrlock);
	else if(m_iLocked == -1)
		ReleaseSRWLockExclusive((SRWLOCK*)m_wrlock);
	m_iLocked = 0;
}

HANDLE GetWRLock()
{
	SRWLOCK * wr = new SRWLOCK();
	InitializeSRWLock(wr);
	return (PVOID)wr;
}
void DestroyWRLock(HANDLE lock)
{
	delete lock;
}
#endif
