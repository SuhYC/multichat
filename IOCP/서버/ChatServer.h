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

	// 소켓초기화 및 연결
	void Init(int bindPort_);
	// 서버 가동 시작. Init()필요
	void Run(const UINT32 maxClient_, const UINT32 maxRoom_);
	// 서버 가동 중지
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

		// 아직 닉네임이 없음
		if (client->GetNickname().empty())
		{
			// 회원가입 데이터 포맷은 특수문자를 제외한 3개의 스트링을 /로 구분하여 전송할 예정.
			// ex) ID/Password/Nickname
			// 로그인 데이터 포맷은 2개의 스트링을 /로 구분하여 전송할 예정.
			// ex) ID/Password

			std::string str(pData);

			std::string nickname = CallDB(str);

			// 로그인 실패
			if (nickname.empty() || !nickname.compare("[INFAIL]"))
			{
				std::cout << "ReturnCode::SIGNIN_FAIL\n";
				// ReturnCode::SIGNIN_FAIL
				PushPacket(clientIndex_, "[103]");
			}
			// 이미 가입함
			else if (!nickname.compare("[AL]"))
			{
				std::cout << "ReturnCode::SIGNUP_ALREADY\n";
				// ReturnCode::SIGNUP_ALREADY
				PushPacket(clientIndex_, "[202]"); 
			}
			// 아이디 무효
			else if (!nickname.compare("[ID]"))
			{
				std::cout << "ReturnCode::SIGNUP_INVALID_ID\n";
				// ReturnCode::SIGNUP_INVALID_ID
				PushPacket(clientIndex_, "[203]"); 
			}
			// 비밀번호 무효
			else if (!nickname.compare("[PW]"))
			{
				std::cout << "ReturnCode::SIGNUP_INVALID_PW\n";
				// ReturnCode::SIGNUP_INVALID_PW
				PushPacket(clientIndex_, "[204]"); 
			}
			// 닉네임 무효
			else if (!nickname.compare("[NICK]"))
			{
				std::cout << "ReturnCode::SIGNUP_INVALID_NICK\n";
				// ReturnCode::SIGNUP_INVALID_NICK
				PushPacket(clientIndex_, "[205]"); 
			}
			// 닉네임 설정 성공
			else
			{
				std::cout << "ReturnCode::SUCCESS\n";
				client->SetNickname(nickname);

				PushPacket(clientIndex_, "[101]");
			}
		}

		// 현재 접속한 채팅방이 있음.
		else if (client->GetChatRoom() != -1)
		{

			ChatRoom* chatRoom = mRoomManager.GetChatRoomByIndex(client->GetChatRoom());

			// 채팅방 나가기 요청
			if (!strcmp(pData, "/Q") || !strcmp(pData, "/q"))
			{
				chatRoom->RemoveUser(clientIndex_);

				client->SetChatRoom(-1);
			}
			// 수신된 채팅을 전파.
			else
			{
				std::string str(pData);
				SendToAllUser(*chatRoom, client->GetNickname(), str);
			}
		}

		// 현재 접속한 채팅방이 없음. (로비)
		else
		{
			// 채팅방 개설 요청
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

				std::cout << "client(" << clientIndex_ << ") : 새 채팅방 사용\n";
				std::string code = chatRoom->GetChatCode();

				// 채팅방 참가에 성공하였다면 해당 채팅방의 코드를 클라이언트로 전송.
				PushPacket(clientIndex_, code);
				SendToAllUser(*chatRoom, client->GetNickname() + "님이 입장하셨습니다.");
			}

			// 코드로 기존 방 참가 요청
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

				std::cout << "client(" << clientIndex_ << ") : 채팅방 참가\n";

				//ReturnCode::ENTERROOM_SUCCESS
				PushPacket(clientIndex_, code);

				SendToAllUser(*chatRoom, client->GetNickname() + "님이 입장하셨습니다.");
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


	// 새롭게 클라이언트에게 전송할 데이터를 큐에 적재해두었다가 전송
	void ProcessPacket();
	// 전송할 데이터를 하나 가져옴
	PacketData* DequePacketData();
	// DB처리할 데이터를 파악하고 요청하는 함수
	std::string CallDB(std::string str_);
	// const char*형 데이터를 클라이언트에게 전달할 패킷으로 만들어 적재하는 함수
	void PushPacket(UINT32 clientIndex_, const char* data_);
	// string형 데이터를 클라이언트에게 전달할 패킷으로 만들어 적재하는 함수
	void PushPacket(UINT32 clientIndex_, std::string data_);
	// 해당 채팅방의 모든유저에게 메시지 전달. 주로 공지메시지
	void SendToAllUser(ChatRoom& chatroom_, std::string data_);
	// 해당 채팅방의 모든유저에게 메시지 전달. 주로 채팅
	void SendToAllUser(ChatRoom& chatroom_, std::string nickname_, std::string data_);
	

	bool mIsRunProcessThread = false;

	std::thread mProcessThread;

	std::mutex mLock;

	std::deque<PacketData*> mPacketDataQueue;

	RoomManager mRoomManager;
	DataBase mDataBase;
};