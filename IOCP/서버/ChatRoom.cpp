#include "ChatRoom.h"

ChatRoom::ChatRoom(UINT32 index_) : mRoomIndex(index_)
{
	mChatCode.clear();
}

void ChatRoom::RemoveUser(int userSocket)
{
	if (user.size() == 1) { // ������ �ο��� ��û�� ��� �ش� �� �ڵ带 ���ְ� ����ִ� ���·� �����.
		user.clear();
		this->mChatCode.clear();

		std::cout << "[ChatRoom" << mRoomIndex << "] : �� �� ä�ù��� û����...\n";
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