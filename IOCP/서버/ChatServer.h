#pragma once

#include "IOCompletionPort.h"
#include "Packet.h"
#include "RoomManager.h"
#include "DataBase.h"

#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <string>

class ChatServer : public IOCompletionPort
{
public:
	ChatServer() {};
	virtual ~ChatServer() {};

	// �����ʱ�ȭ �� ����
	void Init(int bindPort_);
	// ���� ���� ����. Init()�ʿ�
	void Run(const UINT32 maxClient_, const UINT32 maxRoom_);
	// ���� ���� ����
	void End();

private:
	void OnConnect(const UINT32 clientIndex_) override
	{
		std::cout << "[OnConnect] Socket(" << clientIndex_ << ")\n";
	}

	void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData) override
	{
		std::cout << "[OnReceive] Socket(" << clientIndex_ << ")\n";

		auto client = GetClientInfo(clientIndex_);

		// ���� �г����� ����
		if (client->GetNickname().empty())
		{
			// ȸ������ ������ ������ Ư�����ڸ� ������ 3���� ��Ʈ���� /�� �����Ͽ� ������ ����.
			// ex) ID/Password/Nickname
			// �α��� ������ ������ 2���� ��Ʈ���� /�� �����Ͽ� ������ ����.
			// ex) ID/Password

			std::string str(pData);

			std::string nickname = CallDB(str);

			// �α��� ����
			if (nickname.empty() || !nickname.compare("[INFAIL]"))
			{
				std::cout << "ReturnCode::SIGNIN_FAIL\n";
				// ReturnCode::SIGNIN_FAIL
				PushPacket(clientIndex_, "[103]");
			}
			// �̹� ������
			else if (!nickname.compare("[AL]"))
			{
				std::cout << "ReturnCode::SIGNUP_ALREADY\n";
				// ReturnCode::SIGNUP_ALREADY
				PushPacket(clientIndex_, "[202]"); 
			}
			// ���̵� ��ȿ
			else if (!nickname.compare("[ID]"))
			{
				std::cout << "ReturnCode::SIGNUP_INVALID_ID\n";
				// ReturnCode::SIGNUP_INVALID_ID
				PushPacket(clientIndex_, "[203]"); 
			}
			// ��й�ȣ ��ȿ
			else if (!nickname.compare("[PW]"))
			{
				std::cout << "ReturnCode::SIGNUP_INVALID_PW\n";
				// ReturnCode::SIGNUP_INVALID_PW
				PushPacket(clientIndex_, "[204]"); 
			}
			// �г��� ��ȿ
			else if (!nickname.compare("[NICK]"))
			{
				std::cout << "ReturnCode::SIGNUP_INVALID_NICK\n";
				// ReturnCode::SIGNUP_INVALID_NICK
				PushPacket(clientIndex_, "[205]"); 
			}
			// �г��� ���� ����
			else
			{
				std::cout << "ReturnCode::SUCCESS\n";
				client->SetNickname(nickname);

				PushPacket(clientIndex_, "[101]");
			}
		}

		// ���� ������ ä�ù��� ����.
		else if (client->GetChatRoom() != -1)
		{

			ChatRoom* chatRoom = mRoomManager.GetChatRoomByIndex(client->GetChatRoom());

			// ä�ù� ������ ��û
			if (!strcmp(pData, "/Q") || !strcmp(pData, "/q"))
			{
				chatRoom->RemoveUser(clientIndex_);

				client->SetChatRoom(-1);
			}
			// ���ŵ� ä���� ����.
			else
			{
				std::string str(pData);
				SendToAllUser(*chatRoom, client->GetNickname(), str);
			}
		}

		// ���� ������ ä�ù��� ����. (�κ�)
		else
		{
			// ä�ù� ���� ��û
			if (!strcmp(pData, "/N") || !strcmp(pData, "/n"))
			{
				ChatRoom* chatRoom = mRoomManager.GetEmptyRoom();

				if (chatRoom == nullptr)
				{
					// ReturnCode::ENTERROOM_NOROOM
					PushPacket(clientIndex_, "[302]");
					return;
				}

				chatRoom->AddUser(clientIndex_);
				client->SetChatRoom(chatRoom->GetIndex());

				std::cout << "client(" << clientIndex_ << ") : �� ä�ù� ���\n";
				std::string code = chatRoom->GetChatCode();

				// ä�ù� ������ �����Ͽ��ٸ� �ش� ä�ù��� �ڵ带 Ŭ���̾�Ʈ�� ����.
				PushPacket(clientIndex_, code);
				SendToAllUser(*chatRoom, client->GetNickname() + "���� �����ϼ̽��ϴ�.");
			}

			// �ڵ�� ���� �� ���� ��û
			else
			{
				std::string code(pData);

				ChatRoom* chatRoom = mRoomManager.GetChatRoomByCode(code);

				if (chatRoom == nullptr)
				{
					// ReturnCode::ENTERROOM_INCORRECTCODE
					PushPacket(clientIndex_, "[303]");
					return;
				}

				chatRoom->AddUser(clientIndex_);
				client->SetChatRoom(chatRoom->GetIndex());

				std::cout << "client(" << clientIndex_ << ") : ä�ù� ����\n";

				//ReturnCode::ENTERROOM_SUCCESS
				PushPacket(clientIndex_, code);

				SendToAllUser(*chatRoom, client->GetNickname() + "���� �����ϼ̽��ϴ�.");
			}
		}

		return;
	}

	void OnClose(const UINT32 clientIndex_) override
	{
		std::cout << "[OnClose] Socket(" << clientIndex_ << ")\n";

		auto client = GetClientInfo(clientIndex_);
		ChatRoom* chatRoom = mRoomManager.GetChatRoomByIndex(client->GetChatRoom());

		if (chatRoom != nullptr)
		{
			chatRoom->RemoveUser(clientIndex_);
		}

		client->SetChatRoom(-1);
		client->SetNickname(std::string());

		return;
	}


	// ���Ӱ� Ŭ���̾�Ʈ���� ������ �����͸� ť�� �����صξ��ٰ� ����
	void ProcessPacket();
	// ������ �����͸� �ϳ� ������
	PacketData* DequePacketData();
	// DBó���� �����͸� �ľ��ϰ� ��û�ϴ� �Լ�
	std::string CallDB(std::string str_);
	// const char*�� �����͸� Ŭ���̾�Ʈ���� ������ ��Ŷ���� ����� �����ϴ� �Լ�
	void PushPacket(UINT32 clientIndex_, const char* data_);
	// string�� �����͸� Ŭ���̾�Ʈ���� ������ ��Ŷ���� ����� �����ϴ� �Լ�
	void PushPacket(UINT32 clientIndex_, std::string data_);
	// �ش� ä�ù��� ����������� �޽��� ����. �ַ� �����޽���
	void SendToAllUser(ChatRoom& chatroom_, std::string data_);
	// �ش� ä�ù��� ����������� �޽��� ����. �ַ� ä��
	void SendToAllUser(ChatRoom& chatroom_, std::string nickname_, std::string data_);
	

	bool mIsRunProcessThread = false;

	std::thread mProcessThread;

	std::mutex mLock;

	std::deque<PacketData*> mPacketDataQueue;

	RoomManager mRoomManager;
	DataBase mDataBase;
};