#pragma once
#include <vector>
#include <iostream>
#include <WinSock2.h>

class ChatRoom
{
private:
	// 채팅방 고유의 코드. 해당 코드로 접속할 수 있고 다른 채팅방과 중복되지 않음.
	std::string mChatCode;
	// 채팅방 번호
	INT32 mRoomIndex;
	// 해당 채팅방에 존재하는 유저의 정보가 저장된 벡터.
	std::vector<int> user;

public:
	ChatRoom(UINT index_);
	~ChatRoom() {}

	std::string GetChatCode() { return mChatCode; }
	INT32 GetIndex() { return mRoomIndex; }
	void SetCode(std::string str_) { mChatCode = str_; }
	const std::vector<int>& GetUserList() { return user; }

	// 해당 채팅방의 유저 목록에서 특정 유저를 제거
	void RemoveUser(int userSocket);
	// 해당 채팅방의 유저 목록에 특정 유저를 추가
	void AddUser(int userSocket);
};

