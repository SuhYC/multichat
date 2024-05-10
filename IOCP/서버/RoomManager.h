#pragma once
#include "ChatRoom.h"
#include <mutex>

class RoomManager
{
private:
	std::vector<std::shared_ptr<ChatRoom>> ChatRoomList;
	// ���� �ִ� ä�ù��� �ߺ��� �ڵ带 ���� �ʵ��� ��ȣ����
	std::mutex mRoomLock;
	UINT32 mIndexCnt;
	INT32 mMaxRoom = 0;

public:
	RoomManager() { mIndexCnt = 0; }

	// �ʱ�ȭ. �ִ� �� ������ �����ϰ� CreateRoom�� ȣ���Ѵ�.
	void Init(const UINT maxRoom_);
	// ä�ù� ����
	void CreateRoom();
	// ä�ù� �޸� ȸ��
	void DestroyRoom();

	std::shared_ptr<ChatRoom> GetChatRoomByIndex(const INT32 chatRoomIndex_);
	std::shared_ptr<ChatRoom> GetChatRoomByCode(const std::string chatRoomCode_);

	// �� ���� ������ ���� ä�ù�� �ٸ� �ڵ带 ���� �� ��ȯ. Mutual Exclusion.
	std::shared_ptr<ChatRoom> GetEmptyRoom();

	// ���ο� 6���� ä�ù��ڵ带 ����.
	std::string CreateNewCode();
	
};