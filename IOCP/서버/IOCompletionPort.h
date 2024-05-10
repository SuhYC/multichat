#pragma once

#include <thread>
#include <vector>

#include "ClientInfo.h"

class IOCompletionPort
{
protected:
	IOCompletionPort(void);
	virtual ~IOCompletionPort(void);

	//������ �ʱ�ȭ�ϴ� �Լ�
	bool InitSocket();

	//������ �ּ������� ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� ������ ����ϴ� �Լ�
	bool BindandListen(int nBindPort);

	//���� ��û�� �����ϰ� �޼����� �޾Ƽ� ó���ϴ� �Լ�
	bool StartServer(const UINT32 maxClientCount);

	//�����Ǿ��ִ� �����带 �ı��Ѵ�.
	void DestroyThread();

	//�� Ŭ���̾�Ʈ�� �޽��� �۽� ��û�� �Ѵ�.
	bool SendMsg(const UINT32 sessionIndex_, const UINT32 dataSize_, std::shared_ptr<char>& pData);

	//�ش� ��ȣ�� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ.
	std::shared_ptr<ClientInfo> GetClientInfo(const UINT32 sessionIndex_);

private:
	// �ִ���� Ŭ���̾�Ʈ ����ŭ Ŭ���̾�Ʈ���� ��ü ����
	void CreateClient();

	// WaitingThread Queue���� ����� ��������� ����.
	bool CreateWorkerThread();

	// accept��û�� ó���ϴ� ������ ����.
	bool CreateAccepterThread();

	// Overlapped I/O�۾��� ���� �Ϸ� �뺸�� �޾� �׿� �ش��ϴ� ó���� �ϴ� �Լ�
	void WorkerThread();

	// ������� ������ �޴� ������
	void AccepterThread();
	
	// ���� ��ȯ �� Ŭ���̾�Ʈ ���� �ʱ�ȭ
	void CloseSocket(ClientInfo* pClientInfo_, bool bIsForce_ = false);

	//Ŭ���̾�Ʈ ���� ���� ����ü
	std::vector<std::shared_ptr<ClientInfo>> mClientInfos;

	//Ŭ���̾�Ʈ�� ������ �ޱ����� ���� ����
	SOCKET		mListenSocket = INVALID_SOCKET;

	// IO Worker ������
	std::vector<std::thread> mIOWorkerThreads;

	// Accept ������
	std::thread	mAccepterThread;

	//CompletionPort��ü �ڵ�
	HANDLE		mIOCPHandle = INVALID_HANDLE_VALUE;

	// �ִ� ���� Ŭ���̾�Ʈ
	UINT mMaxClientCount;

	// �۾� ������ ���� �÷���
	bool		mIsWorkerRun = true;
	// ���� ������ ���� �÷���
	bool		mIsAccepterRun = true;

	// ��Ʈ��ũ �̺�Ʈ ó��
	virtual void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData) = 0;

	virtual void OnConnect(const UINT32 clientIndex_) = 0;

	virtual void OnClose(const UINT32 clientIndex_) = 0;
};