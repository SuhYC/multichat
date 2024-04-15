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

	// 전송할 메시지 큐에 메시지를 적재하는 함수. *Mutual Exclusion
	void SendMsg(std::string msg_);
protected:
	// 소켓통신 준비과정. ConnectEx
	bool InitSocket(const int port_, const char* ipAddress_);

	// Connect
	bool InitSocket_Sync(const int port_, const char* ipAddress_);
	// 소켓통신 종료. 스레드 회수.
	bool Close();

private:
	// WorkerThread를 만드는 함수.
	void CreateWorkerThread();
	// 워커스레드
	void WorkerThread();
	//
	void SendIO();
	//
	void SendCompleted();

	bool BindRecv();

	// 메시지가 수신되었을 때 수신한 메시지를 파라미터로 호출되는 함수.
	virtual void OnReceive(std::string data_) = 0;
	// 서버에 연결되었을 때 호출되는 함수.
	virtual void OnConnect() = 0;
	// 연결이 종료되었을 때 호출되는 함수.
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