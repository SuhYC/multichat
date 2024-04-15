#include "ChatRoom.h"

ChatRoom::ChatRoom(UINT32 index_) : mRoomIndex(index_)
{
	mChatCode.clear();
}

void ChatRoom::RemoveUser(int userSocket)
{
	if (user.size() == 1) { // 마지막 인원이 요청할 경우 해당 방 코드를 없애고 비어있는 상태로 만든다.
		user.clear();
		this->mChatCode.clear();

		std::cout << "[ChatRoom" << mRoomIndex << "] : 다 쓴 채팅방을 청소중...\n";
		return;
	}

	for (int i = 0; i < user.size(); i++) { // 
		if (user[i] == userSocket) {
			user.erase(user.begin() + i);
			break;
		}
	}

	return;
}

void ChatRoom::AddUser(int userSocket)
{
	user.push_back(userSocket);

	return;
}