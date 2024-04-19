#include "SocketClass.h"

void SocketClass::ConnectEx(SOCKET& socket, GUID guid)
{
	DWORD dwbyte{ 0 };
	WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid, sizeof(guid),
		&g_connect, sizeof(g_connect),
		&dwbyte, NULL, NULL);
}

bool SocketClass::InitSocket_Sync(const int port_, const char* ipAddress_)
{
	WORD		wVersionRequested;
	WSADATA		wsaData;
	SOCKADDR_IN server; //Socket address information
	int			nRet;

	wVersionRequested = MAKEWORD(2, 2);
	nRet = WSAStartup(wVersionRequested, &wsaData);

	if (nRet != 0) {
		std::cerr << "[에러] SocketClass::InitSocket_Sync : WSAStartup error" << WSAGetLastError() << "\n";
		WSACleanup();
		return false;
	}

	server.sin_family = AF_INET; // address family Internet
	server.sin_port = htons(port_); //Port to connect on
	inet_pton(AF_INET, ipAddress_, &(server.sin_addr.s_addr)); //target.sin_addr.s_addr = inet_addr(IPAddress)


	mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Create socket

	if (mSocket == INVALID_SOCKET)
	{
		std::cerr << "[에러] SocketClass::InitSocket_Sync : socket() error" << WSAGetLastError() << "\n";
		WSACleanup();
		return false; //Couldn't create the socket
	}

	if (connect(mSocket, reinterpret_cast<SOCKADDR*>(&server), sizeof(server)) == SOCKET_ERROR)
	{
		std::cerr << "[에러] SocketClass::InitSocket_Sync : connect() error" << WSAGetLastError() << "\n";
		WSACleanup();
		return false; //Couldn't connect
	}

	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_THREAD_NUM);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(mSocket), mIOCPHandle, 0, 0);

	mIsRun = true;
	CreateWorkerThread();

	BindRecv();

	OnConnect();

	return true;
}

bool SocketClass::InitSocket(const int port_, const char* ipAddress_)
{
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		std::cerr << "[에러] SocketClass::InitSocket : WSAStartup error\n";
		WSACleanup();
		return false;
	}

	SOCKADDR_IN server;
	ZeroMemory(&server, sizeof(server));
	server.sin_family = PF_INET;
	server.sin_port = 0;
	server.sin_addr.s_addr = htonl(INADDR_ANY);


	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (bind(mSocket, reinterpret_cast<sockaddr*>(&server),
		sizeof(server)) == SOCKET_ERROR) {
		std::cerr << "[에러] SocketClass::InitSocket : bind error" << GetLastError() << "\n";
		closesocket(mSocket);
		WSACleanup();
		return false;
	}

	//Create IOCP
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_THREAD_NUM);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(mSocket), mIOCPHandle, 0, 0);

	auto over_ex = new stOverlappedEx;
	ZeroMemory(over_ex, sizeof(stOverlappedEx));
	over_ex->m_eOperation = eIOOperation::CONNECT;
	server.sin_port = htons(port_);
	inet_pton(AF_INET, ipAddress_, &(server.sin_addr.s_addr));

	// 함수포인터
	ConnectEx(mSocket, WSAID_CONNECTEX);
	if (g_connect(mSocket, reinterpret_cast<SOCKADDR*>(&server),
		sizeof(server), NULL, NULL, NULL,
		reinterpret_cast<LPOVERLAPPED>(over_ex)) == FALSE) {

		auto error = GetLastError();
		if (error != WSA_IO_PENDING) {
			std::cerr << "[에러] SocketClass::InitSocket : ConnectEx Error" << error << "\n";
			closesocket(mSocket);
			WSACleanup();
			return false;
		}
	}
	
	int nRet = setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
	if (nRet != 0)
	{
		std::cerr << "[에러] SocketClass::InitSocket : setsockopt Error" << WSAGetLastError() << "\n";
		return false;
	}

	mIsRun = true;

	CreateWorkerThread();

	return true;
}

bool SocketClass::Close()
{
	mIsRun = false;

	CloseHandle(mIOCPHandle);

	for (auto& i : mWorkerThread)
	{
		if (i.joinable())
		{
			i.join();
		}
	}

	closesocket(mSocket);
	WSACleanup();

	return true;
}

void SocketClass::CreateWorkerThread()
{
	for (int i = 0; i < MAX_THREAD_NUM; ++i)
	{
		mWorkerThread.emplace_back([this]() {WorkerThread(); });
	}

	return;
}

void SocketClass::WorkerThread()
{
	PULONG_PTR completionKey = nullptr;
	BOOL bSuccess = TRUE;
	DWORD dwIoSize = 0;
	LPOVERLAPPED lpOverlapped = NULL;

	while (mIsRun)
	{
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
			&dwIoSize,
			(PULONG_PTR)&completionKey,
			&lpOverlapped,
			INFINITE);

		if (bSuccess && dwIoSize == 0 && lpOverlapped == nullptr)
		{
			mIsRun = false;
			return;
		}

		if (lpOverlapped == nullptr)
		{
			continue;
		}

		stOverlappedEx* pOverlappedEx = (stOverlappedEx*)lpOverlapped;

		if (pOverlappedEx->m_eOperation != eIOOperation::CONNECT &&
			(!bSuccess || (dwIoSize == 0 && bSuccess)))
		{
			OnDisConnect();
			return;
		}

		if (pOverlappedEx->m_eOperation == eIOOperation::CONNECT)
		{
			delete pOverlappedEx;
			// 수신 가능으로 설정
			BindRecv();

			// 연결완료 네트워크함수 호출
			OnConnect();
		}
		else if (pOverlappedEx->m_eOperation == eIOOperation::RECV)
		{
			pOverlappedEx->m_wsaBuf.buf[dwIoSize] = NULL;

			std::string str(pOverlappedEx->m_wsaBuf.buf);

			// 수신완료 네트워크함수 호출
			OnReceive(str);

			BindRecv();
		}
		else if (pOverlappedEx->m_eOperation == eIOOperation::SEND)
		{
			// 송신완료 -> 데이터가 남아있다면 더 송신
			SendCompleted();
		}
	}

	return;
}

void SocketClass::SendMsg(std::string msg_)
{
	auto sendOverlappedEx = new stOverlappedEx;
	ZeroMemory(sendOverlappedEx, sizeof(stOverlappedEx));

	const char* msg = msg_.c_str();
	size_t dataSize = strlen(msg);

	sendOverlappedEx->m_wsaBuf.len = dataSize;
	sendOverlappedEx->m_wsaBuf.buf = new char[dataSize];
	CopyMemory(sendOverlappedEx->m_wsaBuf.buf, msg, dataSize);
	sendOverlappedEx->m_eOperation = eIOOperation::SEND;

	std::lock_guard<std::mutex> guard(mLock);

	mSendDataQueue.push(sendOverlappedEx);

	// 1Send
	if (mSendDataQueue.size() == 1)
	{
		SendIO();
	}

	return;
}

void SocketClass::SendIO()
{
	stOverlappedEx* sendOverlappedEx = mSendDataQueue.front();

	DWORD dwRecvNumBytes = 0;

	int nRet = WSASend(mSocket,
		&(sendOverlappedEx->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)sendOverlappedEx,
		NULL);

	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSASend() 함수 실패 : " << WSAGetLastError() << "\n";
	}

	return;
}

void SocketClass::SendCompleted()
{
	std::lock_guard<std::mutex> guard(mLock);

	delete[] mSendDataQueue.front()->m_wsaBuf.buf;
	delete mSendDataQueue.front();

	mSendDataQueue.pop();

	if (!mSendDataQueue.empty())
	{
		SendIO();
	}

	return;
}

bool SocketClass::BindRecv()
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O을 위해 각 정보를 세팅해 준다.
	mRecvOverlappedEx.m_wsaBuf.len = MAX_SOCK_BUF;
	mRecvOverlappedEx.m_wsaBuf.buf = mRecvBuf;
	mRecvOverlappedEx.m_eOperation = eIOOperation::RECV;

	int nRet = WSARecv(mSocket,
		&(mRecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) & (mRecvOverlappedEx),
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] SocketClass::BindRecv : WSARecv() error" << WSAGetLastError() << "\n";
		return false;
	}

	return true;
}
