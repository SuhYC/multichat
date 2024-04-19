#include "ClientInfo.h"

void ClientInfo::Init(const UINT32 index, HANDLE iocpHandle_)
{
	mIndex = index;
	mIOCPHandle = iocpHandle_;
	mRecvOverlappedEx.SessionIndex = mIndex;
}

void ClientInfo::MyAccept(SOCKET& socket_, GUID guid_)
{
	DWORD dwbyte{ 0 };
	WSAIoctl(socket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid_, sizeof(guid_),
		&g_accept, sizeof(g_accept),
		&dwbyte, NULL, NULL);
}

bool ClientInfo::PostAccept(SOCKET listenSock_, const UINT64 curTimeSec_)
{
	mLatestClosedTimeSec = curTimeSec_;

	// 소켓 생성
	mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (mSocket == INVALID_SOCKET)
	{
		return false;
	}

	ZeroMemory(&mAcceptContext, sizeof(stOverlappedEx));

	DWORD bytes = 0;
	DWORD flags = 0;
	mAcceptContext.m_wsaBuf.len = 0;
	mAcceptContext.m_wsaBuf.buf = nullptr;
	mAcceptContext.m_eOperation = eIOOperation::ACCEPT;
	mAcceptContext.SessionIndex = mIndex;
	
	// 함수포인터
	MyAccept(mSocket, WSAID_ACCEPTEX);

	if (g_accept(listenSock_, mSocket, mAcceptBuf, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes, (LPWSAOVERLAPPED) & (mAcceptContext)) == FALSE)
	{
		int errcode = WSAGetLastError();

		if (errcode != WSA_IO_PENDING) // WSA_IO_PENDING은 성공적으로 overlapped operation을 시작함, 나중에 완료
		{
			std::cerr << "[에러] AcceptEx : " << errcode << "\n";
			return false;
		}
	}

	setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<char*>(&listenSock_), sizeof(listenSock_));

	return true;
}

bool ClientInfo::AcceptCompletion()
{
	std::cout << "AcceptCompletion : SessionIndex(" << mIndex << ")\n";

	if (!OnConnect(mIOCPHandle, mSocket))
	{
		return false;
	}

	SOCKADDR_IN stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);
	char clientIP[32] = { 0, };
	inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);

	std::cout << "Connected : IP(" << clientIP << ") SOCKET(" << (int)mSocket << ")\n";

	return true;
}

bool ClientInfo::BindRecv()
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	mRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
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
		std::cerr << "[에러] ClientInfo::BindRecv : WSARecv()함수 실패" <<  WSAGetLastError() << '\n';
		return false;
	}

	return true;
}

bool ClientInfo::OnConnect(HANDLE iocpHandle_, SOCKET socket_)
{
	mSocket = socket_;
	mIsConnect = 1;

	if (!BindIOCompletionPort(iocpHandle_))
	{
		return false;
	}
	return BindRecv();
}

bool ClientInfo::BindIOCompletionPort(HANDLE iocpHandle_)
{
	auto hIOCP = CreateIoCompletionPort(reinterpret_cast<HANDLE>(mSocket)
		, iocpHandle_
		, (ULONG_PTR)(this), 0);

	if (hIOCP == INVALID_HANDLE_VALUE)
	{
		std::cerr << "[에러] ClientInfo::BindIOCompletionPort : CreateIoCompletionPort()함수 실패" <<  GetLastError() << '\n';
		return false;
	}

	return true;
}

void ClientInfo::Close(bool bIsForce)
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER로 설정

	// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음 
	if (bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	mIsConnect = 0;
	mLatestClosedTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	//socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
	shutdown(mSocket, SD_BOTH);

	//소켓 옵션을 설정한다.
	setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//소켓 연결을 종료 시킨다. 
	closesocket(mSocket);

	mSocket = INVALID_SOCKET;

	return;
}

bool ClientInfo::SendMsg(const UINT32 dataSize_, char* pMsg_)
{
	auto sendOverlappedEx = new stOverlappedEx;
	ZeroMemory(sendOverlappedEx, sizeof(stOverlappedEx));

	sendOverlappedEx->m_wsaBuf.len = dataSize_;
	sendOverlappedEx->m_wsaBuf.buf = new char[dataSize_];
	CopyMemory(sendOverlappedEx->m_wsaBuf.buf, pMsg_, dataSize_);
	sendOverlappedEx->m_eOperation = eIOOperation::SEND;
	sendOverlappedEx->SessionIndex = mIndex;

	std::lock_guard<std::mutex> guard(mSendLock); // 1-send

	mSendDataQueue.push_back(sendOverlappedEx);

	// 1Send
	if (mSendDataQueue.size() == 1) 
	{
		SendIO();
	}

	return true;
}

void ClientInfo::SendCompleted(const UINT32 dataSize_)
{
	std::lock_guard<std::mutex> guard(mSendLock);

	delete[] mSendDataQueue.front()->m_wsaBuf.buf;
	delete mSendDataQueue.front();

	mSendDataQueue.pop_front();

	// 해당 클라이언트에 전송작업을 하던 스레드가 이어서 계속 전송한다.
	if (!mSendDataQueue.empty()) 
	{
		SendIO();
	}

	return;
}

bool ClientInfo::SendIO()
{
	auto sendOverlappedEx = mSendDataQueue.front();

	DWORD dwRecvNumBytes = 0;

	int nRet = WSASend(mSocket,
		&(sendOverlappedEx->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)sendOverlappedEx,
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[에러] WSASend()함수 실패 : " << WSAGetLastError() << "\n";
		return false;
	}

	return true;
}

bool ClientInfo::SetSocketOption()
{
	int opt = 1;
	if (setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)) == SOCKET_ERROR)
	{
		std::cerr << "[DEBUG] SO_RCVBUF change error: " << GetLastError() << '\n';
		return false;
	}

	opt = 0;
	if (setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)) == SOCKET_ERROR)
	{
		std::cerr << "[DEBUG] SO_RCVBUF change error: " << GetLastError() << "\n";

		return false;
	}

	return true;
}
