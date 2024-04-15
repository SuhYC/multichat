#pragma once

#include <conio.h>
#include "SocketClass.h"
#include <Windows.h>

/*
*  To Do List
* 
* 1. 서버로부터 공개방 리스트를 가져와 로비에서 보여주기
* - 비밀방 공개방 여부까지 구현해야함.
* 
* 
*/

class ChatClient : public SocketClass
{
public:
	ChatClient() {}
	~ChatClient() {}
	
	// 초기화
	bool Start(const int port_, const char* ipAddress_);
	// 통신이 완전히 종료되어 프로그램을 끝내도 되는지 확인하는 함수.
	bool IsAbleToClose() { return mReadyToClose; }
	// 사용자가 통신을 종료시키는 함수
	void End();
private:

	void OnReceive(std::string msg_) override
	{
		mReceiveQueue += msg_;

		std::string str = PopFront();

		if (!str.empty())
		{
			ActOnReceive(str);
		}
	}

	void OnConnect() override
	{
		PrintScreen();
		mState = ClientState::SIGNINPAGE;
		mSubState = SubState::ID;
		mInputThread = std::thread([this]() {InputThread(); });
	}

	void OnDisConnect() override
	{
		ClearScreen();
		std::cout << "Disconnected from Server.\n";

		mSubState = SubState::QUIT;

		mIsRun = false;

		mReadyToClose = true;

		return;
	}


	// cmd를 초기화하여 이전까지 출력된 문자들을 지우는 함수이다.
	void ClearScreen();

	// 화면을 작성하는 함수이다.
	void PrintScreen();

	// 회원가입화면 작성
	void PrintSignUpPage() const;

	// 로그인화면 작성
	void PrintSignInPage() const;

	// cmd를 초기화, 화면 재작성 *Mutual Exclusion
	void RenewScreen();

	// 오버로딩. cmd를 초기화, 문구를 띄운 후 화면 재작성. *Mutual Exclusion
	void RenewScreen(std::string str_);

	// 채팅방에서 퇴장하는 함수
	void ExitRoom();

	// 입력을 받기위한 스레드
	void InputThread();

	// 회원가입이나 로그인의 파라미터에 대해 유효성 검사 (영문, 숫자만 가능)
	bool CheckValid(const std::string str_) const;

	// 수신이 완료된 메시지를 바탕으로 채팅방을 갱신.
	void ActOnReceive(const std::string msg_);

	// 서브스테이트를 변경. (QUIT스테이트는 변경불가)
	void ChangeSubState(SubState eSubState_);

	// 리시브큐에서 수신이 완료된 데이터를 빼서 가져옴. 수신이 완료되지 않았거나 오류가 발생하면 빈문자열을 리턴한다.
	std::string PopFront();


	std::string mLastInput;
	std::string mReceiveQueue;
	std::string mRoomCode;
	std::vector<std::string> mChatLog;
	ClientState mState = ClientState::SIGNINPAGE;
	SubState mSubState = SubState::ID;

	bool mIsRun = false;
	bool mReadyToClose = false;
	std::thread mInputThread;
	std::mutex mPrintLock;
};
