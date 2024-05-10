#pragma once

#include <thread>
#include <vector>

#include "ClientInfo.h"

class IOCompletionPort
{
protected:
	IOCompletionPort(void);
	virtual ~IOCompletionPort(void);

	//소켓을 초기화하는 함수
	bool InitSocket();

	//서버의 주소정보를 소켓과 연결시키고 접속 요청을 받기 위해 소켓을 등록하는 함수
	bool BindandListen(int nBindPort);

	//접속 요청을 수락하고 메세지를 받아서 처리하는 함수
	bool StartServer(const UINT32 maxClientCount);

	//생성되어있는 쓰레드를 파괴한다.
	void DestroyThread();

	//각 클라이언트에 메시지 송신 요청을 한다.
	bool SendMsg(const UINT32 sessionIndex_, const UINT32 dataSize_, std::shared_ptr<char>& pData);

	//해당 번호의 클라이언트 정보 구조체를 반환.
	std::shared_ptr<ClientInfo> GetClientInfo(const UINT32 sessionIndex_);

private:
	// 최대수용 클라이언트 수만큼 클라이언트정보 객체 생성
	void CreateClient();

	// WaitingThread Queue에서 대기할 쓰레드들을 생성.
	bool CreateWorkerThread();

	// accept요청을 처리하는 쓰레드 생성.
	bool CreateAccepterThread();

	// Overlapped I/O작업에 대한 완료 통보를 받아 그에 해당하는 처리를 하는 함수
	void WorkerThread();

	// 사용자의 접속을 받는 쓰레드
	void AccepterThread();
	
	// 소켓 반환 및 클라이언트 정보 초기화
	void CloseSocket(ClientInfo* pClientInfo_, bool bIsForce_ = false);

	//클라이언트 정보 저장 구조체
	std::vector<std::shared_ptr<ClientInfo>> mClientInfos;

	//클라이언트의 접속을 받기위한 리슨 소켓
	SOCKET		mListenSocket = INVALID_SOCKET;

	// IO Worker 쓰레드
	std::vector<std::thread> mIOWorkerThreads;

	// Accept 쓰레드
	std::thread	mAccepterThread;

	//CompletionPort객체 핸들
	HANDLE		mIOCPHandle = INVALID_HANDLE_VALUE;

	// 최대 수용 클라이언트
	UINT mMaxClientCount;

	// 작업 쓰레드 동작 플래그
	bool		mIsWorkerRun = true;
	// 접속 쓰레드 동작 플래그
	bool		mIsAccepterRun = true;

	// 네트워크 이벤트 처리
	virtual void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData) = 0;

	virtual void OnConnect(const UINT32 clientIndex_) = 0;

	virtual void OnClose(const UINT32 clientIndex_) = 0;
};