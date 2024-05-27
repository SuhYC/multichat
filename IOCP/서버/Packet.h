#pragma once

#include <atomic>

// IO작업 종류
enum class eIOOperation
{
	RECV,
	SEND,
	ACCEPT
};

//WSAOVERLAPPED구조체를 확장 시켜서 필요한 정보를 더 넣었다.
struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;		//Overlapped I/O구조체
	UINT		SessionIndex;			//클라이언트 소켓
	WSABUF		m_wsaBuf;				//Overlapped I/O작업 버퍼
	eIOOperation m_eOperation;			//작업 동작 종류
};

class PacketData
{
public:
	class Block
	{
	public:
		Block(char* pData_)
		{
			refCount.store(1);
			pData = pData_;
		}
		~Block()
		{
			if (pData != nullptr)
			{
				delete[] pData;
			}
		}

		void PlusCount()
		{
			refCount++;
		}

		void MinusCount()
		{
			refCount--;
		}
		
		std::atomic_int refCount;
		char* pData;
	};
	UINT32 SessionIndex = 0;
	UINT32 DataSize = 0;
	Block* pPacketData;
	stOverlappedEx sendOverlapped;

	// 새로운 패킷을 생성.
	// 해당 생성자의 세번째 파라미터로 다른 패킷의 pPacketData->pData를 넣지 않도록 주의.
	PacketData(UINT32 sessionIndex_, UINT32 dataSize_, char* pData_)
	{
		SessionIndex = sessionIndex_;
		DataSize = dataSize_;
		pPacketData = new Block(pData_);
	}

	// 복사생성자.
	// 다른 패킷의 문자열은 복사하되 수신할 클라이언트의 인덱스만 수정
	PacketData(PacketData& other_, UINT32 sessionIndex_)
	{
		SessionIndex = sessionIndex_;
		DataSize = other_.DataSize;
		pPacketData = other_.pPacketData;
		pPacketData->PlusCount();
	}

	// 복사생성자.
	PacketData(PacketData& other_)
	{
		SessionIndex = other_.SessionIndex;
		DataSize = other_.DataSize;
		pPacketData = other_.pPacketData;
		pPacketData->PlusCount();
	}

	~PacketData()
	{
		if (pPacketData->refCount.load() == 1)
		{
			delete pPacketData;
		}
		else
		{
			pPacketData->MinusCount();
		}
	}

	// overlapped 구조체를 초기화하여 바로 송신에 사용할 수 있도록 함.
	void SetOverlapped()
	{
		ZeroMemory(&sendOverlapped, sizeof(stOverlappedEx));

		sendOverlapped.m_wsaBuf.len = DataSize;
		sendOverlapped.m_wsaBuf.buf = pPacketData->pData;

		sendOverlapped.m_eOperation = eIOOperation::SEND;
		sendOverlapped.SessionIndex = SessionIndex;
	}
};