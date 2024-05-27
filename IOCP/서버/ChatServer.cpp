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
		PacketData* packetData = DequePacket();

		if (packetData != nullptr)
		{
			SendMsg(packetData);
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	return;
}

PacketData* ChatServer::DequePacket()
{
	std::lock_guard<std::mutex> guard(mLock);

	if (mPacketDataQueue.empty())
	{
		return nullptr;
	}

	PacketData* ret = mPacketDataQueue.front();
	mPacketDataQueue.pop();

	return ret;
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
				// �� �̻� �����ð� ������ �ȵ�
				return std::string();
			}
		}

		Nickname = str_;
	}

	// ��û�� Nickname�� ���� -> SignIn (�α���)
	if (Nickname.empty())
	{
		Nickname = mDataBase.SignIn(ID, PW);

		// �α��� ���н� empty string ����, ������ �г��� ����
		return Nickname;
	}
	// ��û�� Nickname�� �ִ� -> SignUp (ȸ������)
	else
	{
		eReturnCode ret = mDataBase.SignUp(ID, PW, Nickname);

		// ȸ������ ����
		if (ret == eReturnCode::SIGNUP_SUCCESS)
		{
			return Nickname;
		}
		// �̹� ������
		else if (ret == eReturnCode::SIGNUP_ALREADY)
		{
			return std::string("[AL]");
		}
		// ���̵� ��ȿ
		else if (ret == eReturnCode::SIGNUP_INVALID_ID)
		{
			return std::string("[ID]");
		}
		// ��й�ȣ ��ȿ
		else if (ret == eReturnCode::SIGNUP_INVALID_PW)
		{
			return std::string("[PW]");
		}
		// �г��� ��ȿ
		else if (ret == eReturnCode::SIGNUP_INVALID_NICK)
		{
			return std::string("[NICK]");
		}
		// ???
		else
		{
			std::cerr << "[����] ChatServer::CallDB ���ǵ��� ���� ��ȯ��\n";
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

	char* msg = strcpy(new char[len + 1], str.c_str());
	msg[len] = NULL;

	PacketData* packet = new PacketData(clientIndex_, len, msg);

	std::lock_guard<std::mutex> guard(mLock);
	mPacketDataQueue.push(packet);

	return;
}

void ChatServer::PushPacket(UINT32 clientIndex_, PacketData* packet_)
{
	PacketData* tmp = new PacketData(*packet_, clientIndex_);

	std::lock_guard<std::mutex> guard(mLock);
	mPacketDataQueue.push(tmp);

	return;
}

void ChatServer::SendToAllUser(ChatRoom& chatroom_, std::string nickname_, std::string data_)
{
	std::string str = nickname_ + " : " + data_;
	str = std::to_string(str.size()) + str;
	UINT32 len = strlen(str.c_str());

	char* msg = strcpy(new char[len + 1], str.c_str());
	msg[len] = NULL;

	PacketData* tmp = new PacketData(0, len, msg);

	for (auto otherUserIndex : chatroom_.GetUserList())
	{
		PushPacket(otherUserIndex, tmp);
	}

	delete tmp;

	return;
}

void ChatServer::SendToAllUser(ChatRoom& chatroom_, std::string data_)
{
	std::string str = std::to_string(data_.size()) + data_;
	UINT32 len = strlen(str.c_str());

	char* msg = strcpy(new char[len + 1], str.c_str());
	msg[len] = NULL;

	PacketData* tmp = new PacketData(0, len, msg);

	for (auto otherUserIndex : chatroom_.GetUserList())
	{
		PushPacket(otherUserIndex, tmp);
	}

	delete tmp;

	return;
}