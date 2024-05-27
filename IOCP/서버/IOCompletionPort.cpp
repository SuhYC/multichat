#include "IOCompletionPort.h"

IOCompletionPort::IOCompletionPort()
{
	mMaxClientCount = 0;
}

IOCompletionPort::~IOCompletionPort()
{
	WSACleanup();
}

bool IOCompletionPort::InitSocket()
{
	WSADATA wsaData;

	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRet != 0)
	{
		std::cerr << "[에러] WSAStartup()함수 실패 : " << nRet << "\n";
		return false;
	}

	//TCP , Overlapped I/O 소켓 생성
	mListenSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (mListenSocket == INVALID_SOCKET)
	{
		std::cerr << "[에러] socket()함수 실패 : " << WSAGetLastError() << "\n";
		return false;
	}

	std::cout << "소켓 초기화 성공\n";
	return true;
}

bool IOCompletionPort::BindandListen(int nBindPort)
{
	SOCKADDR_IN		stServerAddr;
	stServerAddr.sin_family = AF_INET;
	//서버 포트 설정.
	stServerAddr.sin_port = htons(nBindPort); 
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//바인드
	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (nRet != 0)
	{
		std::cerr << "[에러] bind()함수 실패 : " << WSAGetLastError() << "\n";
		return false;
	}

	//리슨
	nRet = listen(mListenSocket, 5);
	if (nRet != 0)
	{
		std::cerr << "[에러] listen()함수 실패 : " << WSAGetLastError() << "\n";
		return false;
	}

	//IOCP핸들 발급
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (mIOCPHandle == nullptr)
	{
		std::cerr << "[에러] CreateIoCompletionPort()함수 실패: " << WSAGetLastError() << "\n";
		return false;
	}

	auto hIOCPHandle = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, (UINT32)0, 0);

	if (hIOCPHandle == nullptr)
	{
		std::cerr << "[에러] listen socket IOCP bind 실패 : " << WSAGetLastError() << "\n";
		return false;
	}

	std::cout << "서버 등록 성공\n";
	return true;
}

bool IOCompletionPort::StartServer(const UINT32 maxClientCount)
{
	mMaxClientCount = maxClientCount;
	CreateClient();

	bool bRet = CreateWorkerThread();
	if (!bRet)
	{
		return false;
	}

	bRet = CreateAccepterThread();
	if (!bRet)
	{
		return false;
	}

	std::cout << "서버 시작\n";
	return true;
}

void IOCompletionPort::DestroyThread()
{
	mIsWorkerRun = false;
	CloseHandle(mIOCPHandle);

	for (auto& th : mIOWorkerThreads)
	{
		if (th.joinable())
		{
			th.join();
		}
	}

	for (auto* clientInfo : mClientInfos)
	{
		delete clientInfo;
	}

	//Accepter 쓰레드 종료.
	mIsAccepterRun = false;
	closesocket(mListenSocket);

	if (mAccepterThread.joinable())
	{
		mAccepterThread.join();
	}

	return;
}

bool IOCompletionPort::SendMsg(PacketData* packet_)
{
	ClientInfo* pClient = GetClientInfo(packet_->SessionIndex);

	if (pClient == nullptr)
	{
		std::cerr << "[에러] IOCP::SendMsg : 클라이언트 정보 확인 불가";
		return false;
	}

	return pClient->SendMsg(packet_);
}

ClientInfo* IOCompletionPort::GetClientInfo(const UINT32 sessionIndex_)
{
	if (sessionIndex_ >= mMaxClientCount)
	{
		std::cerr << "[에러] IOCP::GetClientInfo : 세션코드 부적합\n";
		return nullptr;
	}

	return mClientInfos[sessionIndex_];
}

void IOCompletionPort::CreateClient()
{
	for (UINT32 i = 0; i < mMaxClientCount; ++i)
	{
		mClientInfos.push_back(new ClientInfo(i, mIOCPHandle));
	}

	return;
}

bool IOCompletionPort::CreateWorkerThread()
{
	//WaitngThread Queue에 대기 상태로 넣을 쓰레드들 생성. 권장되는 개수 : (cpu개수 * 2) + 1 
	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
	}

	std::cout << "WorkerThread 시작..\n";
	return true;
}

bool IOCompletionPort::CreateAccepterThread()
{
	mAccepterThread = std::thread([this]() { AccepterThread(); });

	std::cout << "AccepterThread 시작..\n";
	return true;
}

void IOCompletionPort::WorkerThread()
{
	//CompletionKey
	ClientInfo* pClientInfo = nullptr;
	//함수 호출 성공 여부
	BOOL bSuccess = TRUE;
	//전송된 데이터 크기
	DWORD dwIoSize = 0;
	//Overlapped 구조체
	LPOVERLAPPED lpOverlapped = NULL;

	while (mIsWorkerRun)
	{
		// IO처리완료 통보가 올때까지 WaitingThread Queue에 Blocked된 상태로 대기
		// 완료된 Overlapped IO 작업을 가져온다.
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
			&dwIoSize,					// 실제로 전송된 바이트
			(PULONG_PTR)&pClientInfo,	// CompletionKey
			&lpOverlapped,				// Overlapped IO 객체
			INFINITE);					// 대기할 시간

		//사용자 쓰레드 종료 메세지 처리
		if (bSuccess && dwIoSize == 0 && lpOverlapped == nullptr)
		{
			mIsWorkerRun = false;
			continue;
		}

		if (lpOverlapped == nullptr)
		{
			continue;
		}

		// Overlapped구조체를 확장해서 어떤 IO요청이었는지 확인
		stOverlappedEx* pOverlappedEx = (stOverlappedEx*)lpOverlapped;
		pClientInfo = GetClientInfo(pOverlappedEx->SessionIndex);

		//client가 접속을 끊었을때
		if (pOverlappedEx->m_eOperation != eIOOperation::ACCEPT && (!bSuccess || (dwIoSize == 0 && bSuccess)))
		{
			CloseSocket(pClientInfo);
			continue;
		}

		//Overlapped I/O Accept작업 결과 처리
		if (pOverlappedEx->m_eOperation == eIOOperation::ACCEPT)
		{
			if (pClientInfo->AcceptCompletion())
			{
				OnConnect(pClientInfo->GetIndex());
			}
			else
			{
				CloseSocket(pClientInfo, true);
			}
		}
		//Overlapped I/O Recv작업 결과 처리
		else if (pOverlappedEx->m_eOperation == eIOOperation::RECV)
		{
			pClientInfo->RecvBuffer()[dwIoSize] = NULL;
			std::cout << "[수신] bytes : " << dwIoSize << ", msg : " << pClientInfo->RecvBuffer() << "\n";
			
			OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->RecvBuffer());
			pClientInfo->BindRecv();
		}
		//Overlapped I/O Send작업 결과 처리
		else if (pOverlappedEx->m_eOperation == eIOOperation::SEND)
		{
			pClientInfo->SendCompleted(dwIoSize);
		}
		else 
		{
			std::cerr << "[에러] IOCompletionPort::WorkerThread : socket(" << (int)pClientInfo->GetSocket() << ")에서 예외상황\n";
		}
	}
}

void IOCompletionPort::AccepterThread()
{
	while (mIsAccepterRun)
	{
		auto curTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		for (auto client : mClientInfos)
		{
			if (client->IsConnected())
			{
				continue;
			}

			if ((UINT64)curTimeSec < client->GetLatestClosedTimeSec())
			{
				continue;
			}

			auto diff = curTimeSec - client->GetLatestClosedTimeSec();
			if (diff <= RE_USE_SESSION_WAIT_TIMESEC)
			{
				continue;
			}

			client->PostAccept(mListenSocket, curTimeSec);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(32));
	}
}

void IOCompletionPort::CloseSocket(ClientInfo* pClientInfo_, bool bIsForce)
{
	auto clientIndex = pClientInfo_->GetIndex();

	pClientInfo_->Close(bIsForce);

	OnClose(clientIndex);

	return;
}