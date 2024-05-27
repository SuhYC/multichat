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
		std::cerr << "[����] WSAStartup()�Լ� ���� : " << nRet << "\n";
		return false;
	}

	//TCP , Overlapped I/O ���� ����
	mListenSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (mListenSocket == INVALID_SOCKET)
	{
		std::cerr << "[����] socket()�Լ� ���� : " << WSAGetLastError() << "\n";
		return false;
	}

	std::cout << "���� �ʱ�ȭ ����\n";
	return true;
}

bool IOCompletionPort::BindandListen(int nBindPort)
{
	SOCKADDR_IN		stServerAddr;
	stServerAddr.sin_family = AF_INET;
	//���� ��Ʈ ����.
	stServerAddr.sin_port = htons(nBindPort); 
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//���ε�
	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (nRet != 0)
	{
		std::cerr << "[����] bind()�Լ� ���� : " << WSAGetLastError() << "\n";
		return false;
	}

	//����
	nRet = listen(mListenSocket, 5);
	if (nRet != 0)
	{
		std::cerr << "[����] listen()�Լ� ���� : " << WSAGetLastError() << "\n";
		return false;
	}

	//IOCP�ڵ� �߱�
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (mIOCPHandle == nullptr)
	{
		std::cerr << "[����] CreateIoCompletionPort()�Լ� ����: " << WSAGetLastError() << "\n";
		return false;
	}

	auto hIOCPHandle = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, (UINT32)0, 0);

	if (hIOCPHandle == nullptr)
	{
		std::cerr << "[����] listen socket IOCP bind ���� : " << WSAGetLastError() << "\n";
		return false;
	}

	std::cout << "���� ��� ����\n";
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

	std::cout << "���� ����\n";
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

	//Accepter ������ ����.
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
		std::cerr << "[����] IOCP::SendMsg : Ŭ���̾�Ʈ ���� Ȯ�� �Ұ�";
		return false;
	}

	return pClient->SendMsg(packet_);
}

ClientInfo* IOCompletionPort::GetClientInfo(const UINT32 sessionIndex_)
{
	if (sessionIndex_ >= mMaxClientCount)
	{
		std::cerr << "[����] IOCP::GetClientInfo : �����ڵ� ������\n";
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
	//WaitngThread Queue�� ��� ���·� ���� ������� ����. ����Ǵ� ���� : (cpu���� * 2) + 1 
	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
	}

	std::cout << "WorkerThread ����..\n";
	return true;
}

bool IOCompletionPort::CreateAccepterThread()
{
	mAccepterThread = std::thread([this]() { AccepterThread(); });

	std::cout << "AccepterThread ����..\n";
	return true;
}

void IOCompletionPort::WorkerThread()
{
	//CompletionKey
	ClientInfo* pClientInfo = nullptr;
	//�Լ� ȣ�� ���� ����
	BOOL bSuccess = TRUE;
	//���۵� ������ ũ��
	DWORD dwIoSize = 0;
	//Overlapped ����ü
	LPOVERLAPPED lpOverlapped = NULL;

	while (mIsWorkerRun)
	{
		// IOó���Ϸ� �뺸�� �ö����� WaitingThread Queue�� Blocked�� ���·� ���
		// �Ϸ�� Overlapped IO �۾��� �����´�.
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
			&dwIoSize,					// ������ ���۵� ����Ʈ
			(PULONG_PTR)&pClientInfo,	// CompletionKey
			&lpOverlapped,				// Overlapped IO ��ü
			INFINITE);					// ����� �ð�

		//����� ������ ���� �޼��� ó��
		if (bSuccess && dwIoSize == 0 && lpOverlapped == nullptr)
		{
			mIsWorkerRun = false;
			continue;
		}

		if (lpOverlapped == nullptr)
		{
			continue;
		}

		// Overlapped����ü�� Ȯ���ؼ� � IO��û�̾����� Ȯ��
		stOverlappedEx* pOverlappedEx = (stOverlappedEx*)lpOverlapped;
		pClientInfo = GetClientInfo(pOverlappedEx->SessionIndex);

		//client�� ������ ��������
		if (pOverlappedEx->m_eOperation != eIOOperation::ACCEPT && (!bSuccess || (dwIoSize == 0 && bSuccess)))
		{
			CloseSocket(pClientInfo);
			continue;
		}

		//Overlapped I/O Accept�۾� ��� ó��
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
		//Overlapped I/O Recv�۾� ��� ó��
		else if (pOverlappedEx->m_eOperation == eIOOperation::RECV)
		{
			pClientInfo->RecvBuffer()[dwIoSize] = NULL;
			std::cout << "[����] bytes : " << dwIoSize << ", msg : " << pClientInfo->RecvBuffer() << "\n";
			
			OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->RecvBuffer());
			pClientInfo->BindRecv();
		}
		//Overlapped I/O Send�۾� ��� ó��
		else if (pOverlappedEx->m_eOperation == eIOOperation::SEND)
		{
			pClientInfo->SendCompleted(dwIoSize);
		}
		else 
		{
			std::cerr << "[����] IOCompletionPort::WorkerThread : socket(" << (int)pClientInfo->GetSocket() << ")���� ���ܻ�Ȳ\n";
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