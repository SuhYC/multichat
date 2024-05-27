#include "ClientInfo.h"

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

	// ���� ����
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

	MyAccept(mSocket, WSAID_ACCEPTEX);

	if (g_accept(listenSock_, mSocket, mAcceptBuf, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes, (LPWSAOVERLAPPED) & (mAcceptContext)) == FALSE)
	{
		int errcode = WSAGetLastError();

		if (errcode != WSA_IO_PENDING) // WSA_IO_PENDING�� ���������� overlapped operation�� ������, ���߿� �Ϸ�
		{
			std::cerr << "[����] AcceptEx : " << errcode << "\n";
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

	//Overlapped I/O�� ���� �� ������ ������ �ش�.
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

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[����] ClientInfo::BindRecv : WSARecv()�Լ� ����" <<  WSAGetLastError() << '\n';
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
		std::cerr << "[����] ClientInfo::BindIOCompletionPort : CreateIoCompletionPort()�Լ� ����" <<  GetLastError() << '\n';
		return false;
	}

	return true;
}

void ClientInfo::Close(bool bIsForce)
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER�� ����

	// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
	if (bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	mIsConnect = 0;
	mLatestClosedTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	//socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
	shutdown(mSocket, SD_BOTH);

	//���� �ɼ��� �����Ѵ�.
	setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//���� ������ ���� ��Ų��. 
	closesocket(mSocket);

	mSocket = INVALID_SOCKET;

	return;
}

bool ClientInfo::SendMsg(PacketData* packet_)
{
	packet_->SetOverlapped();

	std::lock_guard<std::mutex> guard(mSendLock); // 1-send

	mSendDataQueue.push(packet_);

	// ��� ���� �����Ͱ� ���δ�. (������ �������̴� �����Ͱ� ����)
	if (mSendDataQueue.size() == 1) 
	{
		SendIO();
	}

	return true;
}

void ClientInfo::SendCompleted(const UINT32 dataSize_)
{
	std::lock_guard<std::mutex> guard(mSendLock);

	delete mSendDataQueue.front();
	mSendDataQueue.pop();

	// �ش� Ŭ���̾�Ʈ�� �����۾��� �ϴ� �����尡 �̾ ��� �����Ѵ�.
	if (!mSendDataQueue.empty()) 
	{
		SendIO();
	}

	return;
}

bool ClientInfo::SendIO()
{
	PacketData* packet = mSendDataQueue.front();

	DWORD dwRecvNumBytes = 0;

	int nRet = WSASend(mSocket,
		&(packet->sendOverlapped.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED) & (packet->sendOverlapped),
		NULL);

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << "[����] WSASend()�Լ� ���� : " << WSAGetLastError() << "\n";
		return false;
	}

	return true;
}