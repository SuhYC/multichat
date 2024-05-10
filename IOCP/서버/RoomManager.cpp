#include "RoomManager.h"

void RoomManager::Init(const UINT maxRoom_)
{
	mMaxRoom = maxRoom_;
	CreateRoom();
}

void RoomManager::CreateRoom()
{
	for (INT32 i = 0; i < mMaxRoom; i++)
	{
		std::shared_ptr<ChatRoom> cr = std::make_shared<ChatRoom>(mIndexCnt++);
		ChatRoomList.emplace_back(std::move(cr));
	}

	return;
}

void RoomManager::DestroyRoom()
{
	for (INT32 i = mMaxRoom - 1; i >= 0; i--)
	{
		ChatRoomList[i].reset();
	}

	return;
}

std::shared_ptr<ChatRoom> RoomManager::GetChatRoomByIndex(const INT32 chatRoomIndex_)
{ 
	if (chatRoomIndex_ == -1 || chatRoomIndex_ >= mMaxRoom)
	{
		std::shared_ptr<ChatRoom> tmp = nullptr;

		return tmp;
	}

	return std::shared_ptr<ChatRoom>(ChatRoomList[chatRoomIndex_]);
}

std::shared_ptr<ChatRoom> RoomManager::GetChatRoomByCode(const std::string chatRoomCode_)
{
	std::lock_guard<std::mutex> guard(mRoomLock);

	for (std::shared_ptr<ChatRoom> chatRoom : ChatRoomList)
	{
		if (!chatRoom->GetChatCode().compare(chatRoomCode_))
		{
			return std::shared_ptr<ChatRoom>(chatRoom);
		}
	}

	return std::shared_ptr<ChatRoom>(nullptr);
}

std::shared_ptr<ChatRoom> RoomManager::GetEmptyRoom()
{
	std::lock_guard<std::mutex> guard(mRoomLock);

	std::string code = CreateNewCode();

	for (std::shared_ptr<ChatRoom> chatRoom : ChatRoomList)
	{
		if (chatRoom->GetChatCode().empty())
		{
			chatRoom->SetCode(code); // 새 코드 부여
			return std::shared_ptr<ChatRoom>(chatRoom);
		}
	}

	std::cerr << "[에러] ChatRoom::GetEmptyRoom : 채팅방 부족\n";

	return std::shared_ptr<ChatRoom>(nullptr);
}

std::string RoomManager::CreateNewCode()
{
	std::string str;
	bool bIsValid;
	srand(time(NULL));

	while (true)
	{
		bIsValid = true;

		for (int i = 0; i < 6; i++)
		{
			str = str + (char)('A' + rand() % 26);
		}

		for (auto cr : ChatRoomList)
		{
			if (!cr->GetChatCode().compare(str))
			{
				str.clear();
				bIsValid = false;
				break;
			}
		}

		if (bIsValid)
		{
			break;
		}
	}

	return str;
}