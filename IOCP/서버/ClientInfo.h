#pragma once
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "mswsock.lib") // AcceptEx()

#include "Define.h"
#include <iostream>
#include <mutex>
#include <queue>


class ClientInfo
{
public:
	UINT32 GetIndex() const { return mIndex; }
	SOCKET GetSocket() const { return mSocket; }
	char* RecvBuffer() { return mRecvBuf; }
	INT32 GetChatRoom() const { return mChatRoomIndex; }
	bool IsConnected() const { return mIsConnect == 1; }
	UINT64 GetLatestClosedTimeSec() const { return mLatestClosedTimeSec; }
	std::string GetNickname() const { return mNickname; }
	void SetChatRoom(INT32 index_) { mChatRoomIndex = index_; }
	void SetNickname(std::string nickname_) { mNickname = nickname_; }

	ClientInfo()
	{
		mIndex = -1;
	}

	ClientInfo(const UINT32 index_, HANDLE mIOCPHandle_)
	{
		ZeroMemory(&mAcceptBuf, 64);
		ZeroMemory(&mRecvBuf, MAX_SOCKBUF);
		ZeroMemory(&mAcceptContext, sizeof(stOverlappedEx));
		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		mIOCPHandle = INVALID_HANDLE_VALUE;
		mSocket = INVALID_SOCKET;
		mChatRoomIndex = -1;

		mIndex = index_;
		mIOCPHandle = mIOCPHandle_;
		mRecvOverlappedEx.SessionIndex = mIndex;
	}

	//WSARecv Overlapped I/O 작업을 시킨다.
	bool PostAccept(SOCKET listenSock_, const UINT64 curTimeSec_);
	bool AcceptCompletion();
	bool BindRecv();
	bool OnConnect(HANDLE IOCPHandle_, SOCKET socket_);
	bool BindIOCompletionPort(HANDLE iocpHandle_);
	void Close(bool bIsForce = false);
	bool SendMsg(PacketData* packet_);
	void SendCompleted(const UINT32 dataSize_);

private:
	bool SendIO();

	UINT32 mIndex = -1;
	SOCKET			mSocket;			//Client와 연결되는 소켓
	stOverlappedEx	mRecvOverlappedEx;	//RECV Overlapped I/O작업을 위한 변수
	stOverlappedEx	mAcceptContext;	// Accpet Overlapped I/O작업을 위한 변수
	HANDLE mIOCPHandle;
	INT64 mIsConnect = 0; // 얘 왜 타입이..?
	UINT64 mLatestClosedTimeSec = 0;

	char		mAcceptBuf[64];			//데이터 버퍼
	char		mRecvBuf[MAX_SOCKBUF];	//데이터 버퍼

	INT32 mChatRoomIndex = -1;
	std::string mNickname;

	std::mutex mSendLock;
	std::queue<PacketData*> mSendDataQueue;

	void MyAccept(SOCKET& socket_, GUID guid_);
	LPFN_ACCEPTEX g_accept;
};