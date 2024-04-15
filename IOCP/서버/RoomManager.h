#pragma once
#include "ChatRoom.h"
#include <mutex>

class RoomManager
{
private:
	std::vector<ChatRoom*> ChatRoomList;
	// 현재 있는 채팅방들과 중복된 코드를 갖지 않도록 상호배제
	std::mutex mRoomLock;
	UINT32 mIndexCnt;
	INT32 mMaxRoom = 0;

public:
	RoomManager() { mIndexCnt = 0; }

	// 초기화. 최대 방 갯수를 설정하고 CreateRoom을 호출한다.
	void Init(const UINT maxRoom_);
	// 채팅방 생성
	void CreateRoom();
	// 채팅방 메모리 회수
	void DestroyRoom();

	ChatRoom* GetChatRoomByIndex(const INT32 chatRoomIndex_);
	ChatRoom* GetChatRoomByCode(const std::string chatRoomCode_);

	// 빈 방을 가져와 기존 채팅방과 다른 코드를 적용 후 반환. Mutual Exclusion.
	ChatRoom* GetEmptyRoom();

	// 새로운 6글자 채팅방코드를 생성.
	std::string CreateNewCode();
	
};