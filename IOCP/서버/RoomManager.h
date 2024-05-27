#pragma once
#include "ChatRoom.h"
#include <mutex>

class RoomManager
{
private:
	std::vector<ChatRoom*> ChatRoomList;
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

	ChatRoom* GetChatRoomByIndex(const INT32 chatRoomIndex_);
	ChatRoom* GetChatRoomByCode(const std::string chatRoomCode_);

	// �� ���� ������ ���� ä�ù�� �ٸ� �ڵ带 ���� �� ��ȯ. Mutual Exclusion.
	ChatRoom* GetEmptyRoom();

	// ���ο� 6���� ä�ù��ڵ带 ����.
	std::string CreateNewCode();
	
};