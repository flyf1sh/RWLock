#ifndef WRLOCK_H
#define WRLOCK_H

//typedef void *HANDLE;

HANDLE GetWRLock();
void DestroyWRLock(HANDLE wrlock);

class CWRLock
{
public:
	CWRLock(
		 _Inout_ HANDLE wrlock,
		 _In_ bool bInitialLock=true);//��ʼ��ʱ����Դ��true��������flase��д��
	~CWRLock() throw();
	void RLock();
	void WLock();
	void Unlock() throw();
private:
	HANDLE m_wrlock;
	int m_iLocked;
	CWRLock(_In_ const CWRLock&) throw();
	CWRLock& operator=(_In_ const CWRLock&) throw();
};

#endif
