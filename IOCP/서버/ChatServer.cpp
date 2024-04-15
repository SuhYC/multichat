#define _CRT_SECURE_NO_WARNINGS
#include "ChatServer.h"

void ChatServer::Init(int bindPort_)
{
	InitSocket();
	BindandListen(bindPort_);

	return;
}

void ChatServer::Run(const UINT32 maxClient_, const UINT32 maxRoom_)
{
	mRoomManager.Init(maxRoom_);
	mDataBase.Initialize();

	mIsRunProcessThread = true;
	mProcessThread = std::thread([this]() {ProcessPacket(); });

	StartServer(maxClient_);

	return;
}

void ChatServer::End()
{
	mIsRunProcessThread = false;

	if (mProcessThread.joinable())
	{
		mProcessThread.join();
	}

	DestroyThread();

	mDataBase.End();

	mRoomManager.DestroyRoom();

	return;
}

void ChatServer::ProcessPacket()
{
	while (mIsRunProcessThread)
	{
		auto packetData = DequePacketData();

		if (packetData != nullptr)
		{
			SendMsg(packetData->SessionIndex, packetData->DataSize, packetData->pPacketData);
			delete packetData;
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	return;
}

PacketData* ChatServer::DequePacketData()
{
	std::lock_guard<std::mutex> guard(mLock);

	if (mPacketDataQueue.empty())
	{
		return nullptr;
	}

	PacketData* packet = mPacketDataQueue.front();
	mPacketDataQueue.pop_front();

	return packet;
}

std::string ChatServer::CallDB(std::string str_)
{
	std::string ID, PW, Nickname;

	for (int i = 0; i < str_.size(); i++)
	{
		if (str_[i] == '/')
		{
			ID = str_.substr(0, i);
			if (str_.size() > i + 1)
			{
				str_ = str_.substr(i + 1);
			}
			else
			{
				str_.clear();
			}

			break;
		}
	}

	for (size_t i = 0; i < str_.size(); i++)
	{
		if (str_[i] == '/')
		{
			PW = str_.substr(0, i);
			if (str_.size() > i + 1)
			{
				str_ = str_.substr(i + 1);
			}
			else
			{
				str_.clear();
			}

			break;
		}
	}

	if (PW.empty())
	{
		PW = str_;
		Nickname.clear();
	}
	else
	{
		for (int i = 0; i < str_.size(); i++)
		{
			if (str_[i] == '/')
			{
				// 더 이상 슬래시가 나오면 안됨
				return std::string();
			}
		}

		Nickname = str_;
	}

	// 요청에 Nickname이 없다 -> SignIn (로그인)
	if (Nickname.empty())
	{
		Nickname = mDataBase.SignIn(ID, PW);

		// 로그인 실패시 empty string 전송, 성공시 닉네임 전송
		return Nickname;
	}
	// 요청에 Nickname이 있다 -> SignUp (회원가입)
	else
	{
		eReturnCode ret = mDataBase.SignUp(ID, PW, Nickname);

		// 회원가입 성공
		if (ret == eReturnCode::SIGNUP_SUCCESS)
		{
			return Nickname;
		}
		// 이미 가입함
		else if (ret == eReturnCode::SIGNUP_ALREADY)
		{
			return std::string("[AL]");
		}
		// 아이디 무효
		else if (ret == eReturnCode::SIGNUP_INVALID_ID)
		{
			return std::string("[ID]");
		}
		// 비밀번호 무효
		else if (ret == eReturnCode::SIGNUP_INVALID_PW)
		{
			return std::string("[PW]");
		}
		// 닉네임 무효
		else if (ret == eReturnCode::SIGNUP_INVALID_NICK)
		{
			return std::string("[NICK]");
		}
		// ???
		else
		{
			std::cerr << "[에러] ChatServer::CallDB 정의되지 않은 반환값\n";
			return std::string();
		}
	}
}

void ChatServer::PushPacket(UINT32 clientIndex_, const char* data_)
{
	std::string str(data_);

	PushPacket(clientIndex_, str);

	return;
}

void ChatServer::PushPacket(UINT32 clientIndex_, std::string data_)
{
	std::string str = std::to_string((int)data_.size()) + data_;

	int len = (int)strlen(str.c_str());
	char* newData;
	newData = strcpy(new char[len + 1], str.c_str());

	PacketData* packet = new PacketData();
	packet->Set(clientIndex_, len, newData);

	std::lock_guard<std::mutex> guard(mLock);
	mPacketDataQueue.push_back(packet);

	return;
}

void ChatServer::SendToAllUser(ChatRoom& chatroom_, std::string nickname_, std::string data_)
{
	std::string str = nickname_ + " : " + data_;

	for (auto otherUserIndex : chatroom_.GetUserList())
	{
		PushPacket(otherUserIndex, str);
	}

	return;
}

void ChatServer::SendToAllUser(ChatRoom& chatroom_, std::string data_)
{
	for (auto otherUserIndex : chatroom_.GetUserList())
	{
		PushPacket(otherUserIndex, data_);
	}

	return;
}