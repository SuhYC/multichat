#include "ChatClient.h"

bool ChatClient::Start(const int port_, const char* ipAddress_)
{
	//bool bRet = InitSocket_Sync(port_, ipAddress_); // 동기 Connect 사용
	bool bRet = InitSocket(port_, ipAddress_); // 비동기 Connect 사용

	if (!bRet)
	{
		std::cerr << "[에러] ChatClient::Start InitSocket()\n";
		mReadyToClose = true;
		return false;
	}

	mIsRun = true;

	return true;
}

void ChatClient::ClearScreen()
{
	system("cls");

	return;
}

void ChatClient::ChangeSubState(SubState eSubState_)
{
	if (mSubState == SubState::QUIT)
	{
		return;
	}

	mSubState = eSubState_;

	return;
}

void ChatClient::PrintScreen()
{
	if (mSubState == SubState::QUIT)
	{
		std::cout << "종료.";
		return;
	}

	switch (mState)
	{
	case ClientState::SIGNINPAGE:
		PrintSignInPage();

		break;
	case ClientState::SIGNUPPAGE:
		PrintSignUpPage();

		break;
	case ClientState::LOBBY:
		// 공개방을 띄우는건 나중에 구현.
		std::cout << "새로운 채팅방을 만드시려면 /n을 입력하시고, 종료하시려면 /q를 입력하시고, \n기존 채팅방에 참여하시려면 채팅방의 코드를 입력해주세요.\n";

		break;
	case ClientState::CHATROOM:
		std::cout << "채팅방 코드 : " << mRoomCode << "\n";

		for (std::string str : mChatLog)
		{
			std::cout << str << "\n";
		}

		std::cout << "입력 : " << mLastInput;

		break;
	default:
		std::cerr << "[에러] ChatClient::PrintScreen ClientState 미정의\n";
		break;
	}
}

void ChatClient::PrintSignUpPage() const
{
	switch (mSubState)
	{
	case SubState::ID:
		std::cout << "영문대문자, 영문소문자, 숫자로 구성된 6~10자 ID를 입력해주세요.\n로그인하시려면 /L을 입력해주세요.\n";
		break;
	case SubState::PW:
		std::cout << "영문대문자, 영문소문자, 숫자로 구성된 6~10자 비밀번호를 입력해주세요.\n";
		break;
	case SubState::NICKNAME:
		std::cout << "영문대문자, 영문소문자, 숫자로 구성된 2~10자 닉네임을 입력해주세요.\n";
		break;
	case SubState::WAIT:
		//std::cout << "잠시만 기다려 주세요.\n";
		break;
	default:
		std::cerr << "[에러] ChatClient::PrintSignUpPage SubState 미정의\n";
		break;
	}

	return;
}

void ChatClient::PrintSignInPage() const
{
	switch (mSubState)
	{
	case SubState::ID:
		std::cout << "ID를 입력해주세요.\n회원가입하시려면 /N를 입력해주세요.\n";
		break;
	case SubState::PW:
		std::cout << "비밀번호를 입력해주세요.\n";
		break;
	case SubState::WAIT:
		//std::cout << "잠시만 기다려 주세요.\n";
		break;
	default:
		std::cerr << "[에러] ChatClient::PrintSignInPage SubState 미정의\n";
		break;
	}
	
	return;
}

void ChatClient::RenewScreen()
{
	std::lock_guard<std::mutex> guard(mPrintLock);

	ClearScreen();
	PrintScreen();

	return;
}

void ChatClient::RenewScreen(std::string str_)
{
	std::lock_guard<std::mutex> guard(mPrintLock);

	ClearScreen();
	std::cout << str_ << '\n';
	PrintScreen();

	return;
}

void ChatClient::ExitRoom()
{
	mRoomCode.clear();
	mChatLog.clear();

	mState = ClientState::LOBBY;
	RenewScreen();

	return;
}

void ChatClient::InputThread()
{
	char c;
	std::string msg;

	std::string id;
	std::string pw;
	std::string nickname;

	while (mIsRun)
	{
		// 종료요청
		if (mSubState == SubState::QUIT)
		{
			return;
		}

		if (mSubState == SubState::WAIT)
		{
			continue;
		}

		switch (mState)
		{
		case ClientState::CHATROOM:

			// _getch() : 버퍼를 사용하지 않기 때문에 엔터키를 누르지 않아도 저장한다.
			// 서버로부터 데이터를 받아 그때그때 채팅을 반영하기 위함이다.
			c = _getch();

			if (c == '\b')
			{
				// backspace
				if (!mLastInput.empty())
				{
					mLastInput.pop_back();
					RenewScreen();
				}
			}
			// != new line
			else if (c != '\r' && c != '\n')
			{
				mLastInput += c;
				RenewScreen();
			}
			// == new line
			else
			{
				// 채팅방 나가기
				if (!mLastInput.compare("/q") || !mLastInput.compare("/Q"))
				{
					SendMsg(mLastInput);
					mLastInput.clear();

					ExitRoom();
					break;
				}
				
				if (mLastInput.empty())
				{
					RenewScreen();
					break;
				}
				
				SendMsg(mLastInput);
				mLastInput.clear();
			}
			break;
		case ClientState::SIGNINPAGE:

			if (id.empty())
			{
				std::getline(std::cin, id);

				if (!id.compare("/n") || !id.compare("/N"))
				{
					mState = ClientState::SIGNUPPAGE;
					RenewScreen();
					id.clear();
				}
				else if (!CheckValid(id))
				{
					id.clear();
					RenewScreen("ID가 유효하지 않습니다.\n");
				}
				else
				{
					ChangeSubState(SubState::PW);
					RenewScreen();
				}
			}
			else if (pw.empty())
			{
				std::getline(std::cin, pw);

				if (!CheckValid(pw))
				{
					pw.clear();
					RenewScreen("비밀번호가 유효하지 않습니다.\n");
					break;
				}

				msg = id + '/' + pw;

				SendMsg(msg);
				ChangeSubState(SubState::WAIT);

				id.clear();
				pw.clear();
			}

			break;
		case ClientState::SIGNUPPAGE:

			if (id.empty())
			{
				std::getline(std::cin, id);

				if (!id.compare("/l") || !id.compare("/L"))
				{
					mState = ClientState::SIGNINPAGE;
					RenewScreen();
					id.clear();
				}
				else if (!CheckValid(id))
				{
					id.clear();
					RenewScreen("ID가 유효하지 않습니다.\n");
				}
				else
				{
					ChangeSubState(SubState::PW);
					RenewScreen();
				}
			}
			else if (pw.empty())
			{
				std::getline(std::cin, pw);

				if (!CheckValid(pw))
				{
					pw.clear();
					RenewScreen("비밀번호가 유효하지 않습니다.\n");
				}
				else
				{
					ChangeSubState(SubState::NICKNAME);
					RenewScreen();
				}
			}
			else if (nickname.empty())
			{
				std::getline(std::cin, nickname);

				if (!CheckValid(nickname))
				{
					nickname.clear();
					RenewScreen("닉네임이 유효하지 않습니다.\n");
					break;
				}

				msg = id + '/' + pw + '/' + nickname;

				SendMsg(msg);
				ChangeSubState(SubState::WAIT);

				id.clear();
				pw.clear();
				nickname.clear();
			}

			break;
		case ClientState::LOBBY:

			std::getline(std::cin, msg);

			// 채팅프로그램 종료 요청
			if (!msg.compare("/q"))
			{
				mReadyToClose = true;
				return;
			}

			SendMsg(msg);
			ChangeSubState(SubState::WAIT);

			break;
		default:

			break;
		}
	}

	mLastInput.clear();

	return;
}

bool ChatClient::CheckValid(const std::string str_) const
{
	for (int i = 0; i < str_.size(); i++)
	{
		if (str_[i] >= '0' && str_[i] <= '9')
		{
			continue;
		}

		if (str_[i] >= 'a' && str_[i] <= 'z')
		{
			continue;
		}

		if (str_[i] >= 'A' && str_[i] <= 'Z')
		{
			continue;
		}

		// 영문도 숫자도 아니면 무효.
		return false;
	}

	// 영문과 숫자로만 이루어져있으면 유효.
	return true;
}

void ChatClient::ActOnReceive(const std::string msg_)
{
	if (mState == ClientState::SIGNINPAGE)
	{
		// if 로그인 성공 ~ else 로그인 실패 !! 이상한 데이터 오면 거르기

		// 로그인 성공 코드
		if (!msg_.compare("[101]"))
		{
			ChangeSubState(SubState::ID);
			mState = ClientState::LOBBY;

			RenewScreen();
		}
		// 로그인 실패 코드
		else if (!msg_.compare("[103]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("아이디 혹은 비밀번호를 잘못 입력하셨습니다.\n");
		}
		// 이미 로그인 됨 코드
		else if (!msg_.compare("[102]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("이미 로그인된 아이디입니다.\n");
		}
		// 잘못된 응답 코드
		else
		{
			return;
		}
	}
	else if (mState == ClientState::SIGNUPPAGE)
	{
		// if 회원가입 성공 ~ else 회원가입 실패 !! 이상한 데이터 오면 거르기

		// 회원가입 성공 코드
		if (!msg_.compare("[201]") || !msg_.compare("[101]"))
		{
			ChangeSubState(SubState::ID);
			mState = ClientState::LOBBY;
			RenewScreen();
		}
		// 아이디 중복 코드
		else if (!msg_.compare("[202]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("이미 생성된 ID입니다.\n");
		}
		// 아이디 조건 불만족 코드
		else if (!msg_.compare("[203]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("ID 조건이 맞지 않습니다.\n");
		}
		// 비밀번호 조건 불만족 코드
		else if (!msg_.compare("[204]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("비밀번호 조건이 맞지 않습니다.\n");
		}
		// 닉네임 조건 불만족 코드
		else if (!msg_.compare("[205]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("닉네임 조건이 맞지 않습니다.\n");
		}
		// 잘못된 응답 코드
		else
		{
			return;
		}
	}
	else if (mState == ClientState::LOBBY)
	{
		// if 방 참여 성공 ~ else 방 참여 실패 !! 이상한 데이터 오면 거르기

		// 방 입장 성공
		if (msg_.size() == 6)
		{
			mRoomCode = msg_;
			mState = ClientState::CHATROOM;
			ChangeSubState(SubState::ID);
		}
		// 방 생성 실패
		else if (!msg_.compare("[302]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("방이 너무 많아 생성할 수 없습니다.\n");
		}
		// 방 참가 실패
		else if (!msg_.compare("[303]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("코드가 잘못되었습니다.\n");
		}
		// 잘못된 응답 코드
		else
		{
			//return;
		}
	}
	else if (mState == ClientState::CHATROOM)
	{
		// 유저별 닉네임은 서버에서 붙여서 오도록 하자!
		mChatLog.push_back(msg_);
		RenewScreen();
	}

	return;
}

std::string ChatClient::PopFront()
{
	// 현재 들어있는 데이터가 없음
	if (mReceiveQueue.empty())
	{
		return std::string();
	}

	// 헤더부분이 작성되지 않은 데이터
	if (mReceiveQueue[0] > '9' || mReceiveQueue[0] < '0')
	{
		std::cerr << "[에러] ChatClient::CheckReceiveQueue 메시지큐 데이터 오류\n";
		return std::string();
	}

	// 실제 데이터의 크기
	int packetsize = 0;
	// packetsize의 자릿수
	int header = 0;

	for (int i = 0; i < mReceiveQueue.size(); i++)
	{
		// 숫자데이터가 끝날때까지 읽음
		if (mReceiveQueue[i] <= '9' && mReceiveQueue[i] >= '0')
		{
			header++;
			packetsize *= 10;
			packetsize += mReceiveQueue[i] - '0';
			continue;
		}
		break;
	}

	// 데이터의 길이가 0 이하일리는 없음
	if (packetsize <= 0)
	{
		std::cerr << "[에러] ChatClient::PopFront 문자열 사이즈 에러\n";
		return std::string();
	}

	// 아직 수신이 완료되지 않음
	if (header + packetsize > mReceiveQueue.size())
	{
		return std::string();
	}

	std::string str = mReceiveQueue.substr(header, packetsize);
	mReceiveQueue = mReceiveQueue.substr(header + packetsize);

	return str;
}

void ChatClient::End()
{

	ClearScreen();
	std::cout << "Closing.\n";

	if (mSubState != SubState::WAIT)
	{
		if (mState != ClientState::CHATROOM)
		{
			std::cout << "엔터키를 입력해주세요.\n";
		}
		else
		{
			std::cout << "아무키나 입력해주세요.\n";
		}
	}
	
	mSubState = SubState::QUIT;
	mIsRun = false;

	if(mInputThread.joinable())
	{
		mInputThread.join();
	}

	Close();

	return;
}