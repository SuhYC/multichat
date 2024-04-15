#pragma once
#include <vector>
#include <iostream>
#include <WinSock2.h>

class ChatRoom
{
private:
	// ä�ù� ������ �ڵ�. �ش� �ڵ�� ������ �� �ְ� �ٸ� ä�ù�� �ߺ����� ����.
	std::string mChatCode;
	// ä�ù� ��ȣ
	INT32 mRoomIndex;
	// �ش� ä�ù濡 �����ϴ� ������ ������ ����� ����.
	std::vector<int> user;

public:
	ChatRoom(UINT index_);
	~ChatRoom() {}

	std::string GetChatCode() { return mChatCode; }
	INT32 GetIndex() { return mRoomIndex; }
	void SetCode(std::string str_) { mChatCode = str_; }
	const std::vector<int>& GetUserList() { return user; }

	// �ش� ä�ù��� ���� ��Ͽ��� Ư�� ������ ����
	void RemoveUser(int userSocket);
	// �ش� ä�ù��� ���� ��Ͽ� Ư�� ������ �߰�
	void AddUser(int userSocket);
};

