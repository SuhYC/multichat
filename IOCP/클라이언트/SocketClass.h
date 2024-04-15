#pragma once

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <MSWSock.h>

#include <iostream>

#include <thread>
#include <string>
#include <queue>
#include <mutex>

#include "Define.h"

#pragma comment(lib, "ws2_32.lib")

const int BUF_SIZE = 4096;
const int QUEUE_SIZE = 10;
const int MAX_THREAD_NUM = 4;

class SocketClass
{
public:
	virtual ~SocketClass() {}

	// ������ �޽��� ť�� �޽����� �����ϴ� �Լ�. *Mutual Exclusion
	void SendMsg(std::string msg_);
protected:
	// ������� �غ����. ConnectEx
	bool InitSocket(const int port_, const char* ipAddress_);

	// Connect
	bool InitSocket_Sync(const int port_, const char* ipAddress_);
	// ������� ����. ������ ȸ��.
	bool Close();

private:
	// WorkerThread�� ����� �Լ�.
	void CreateWorkerThread();
	// ��Ŀ������
	void WorkerThread();
	//
	void SendIO();
	//
	void SendCompleted();

	bool BindRecv();

	// �޽����� ���ŵǾ��� �� ������ �޽����� �Ķ���ͷ� ȣ��Ǵ� �Լ�.
	virtual void OnReceive(std::string data_) = 0;
	// ������ ����Ǿ��� �� ȣ��Ǵ� �Լ�.
	virtual void OnConnect() = 0;
	// ������ ����Ǿ��� �� ȣ��Ǵ� �Լ�.
	virtual void OnDisConnect() = 0;

	std::vector<std::thread> mWorkerThread;

	SOCKET mSocket;

	std::queue<std::string> mSendQueue;
	std::queue<stOverlappedEx*> mSendDataQueue;
	stOverlappedEx mRecvOverlappedEx;
	char mRecvBuf[MAX_SOCK_BUF];
	std::mutex mLock;

	bool mIsRun = false;

	void ConnectEx(SOCKET& socket, GUID guid);
	LPFN_CONNECTEX g_connect;
	HANDLE mIOCPHandle = NULL;
};