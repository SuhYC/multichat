#pragma once

#include <atomic>

// IO�۾� ����
enum class eIOOperation
{
	RECV,
	SEND,
	ACCEPT
};

//WSAOVERLAPPED����ü�� Ȯ�� ���Ѽ� �ʿ��� ������ �� �־���.
struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;		//Overlapped I/O����ü
	UINT		SessionIndex;			//Ŭ���̾�Ʈ ����
	WSABUF		m_wsaBuf;				//Overlapped I/O�۾� ����
	eIOOperation m_eOperation;			//�۾� ���� ����
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

	// ���ο� ��Ŷ�� ����.
	// �ش� �������� ����° �Ķ���ͷ� �ٸ� ��Ŷ�� pPacketData->pData�� ���� �ʵ��� ����.
	PacketData(UINT32 sessionIndex_, UINT32 dataSize_, char* pData_)
	{
		SessionIndex = sessionIndex_;
		DataSize = dataSize_;
		pPacketData = new Block(pData_);
	}

	// ���������.
	// �ٸ� ��Ŷ�� ���ڿ��� �����ϵ� ������ Ŭ���̾�Ʈ�� �ε����� ����
	PacketData(PacketData& other_, UINT32 sessionIndex_)
	{
		SessionIndex = sessionIndex_;
		DataSize = other_.DataSize;
		pPacketData = other_.pPacketData;
		pPacketData->PlusCount();
	}

	// ���������.
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

	// overlapped ����ü�� �ʱ�ȭ�Ͽ� �ٷ� �۽ſ� ����� �� �ֵ��� ��.
	void SetOverlapped()
	{
		ZeroMemory(&sendOverlapped, sizeof(stOverlappedEx));

		sendOverlapped.m_wsaBuf.len = DataSize;
		sendOverlapped.m_wsaBuf.buf = pPacketData->pData;

		sendOverlapped.m_eOperation = eIOOperation::SEND;
		sendOverlapped.SessionIndex = SessionIndex;
	}
};