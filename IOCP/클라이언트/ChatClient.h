#pragma once

#include <conio.h>
#include "SocketClass.h"
#include <Windows.h>

/*
*  To Do List
* 
* 1. �����κ��� ������ ����Ʈ�� ������ �κ񿡼� �����ֱ�
* - ��й� ������ ���α��� �����ؾ���.
* 
* 
*/

class ChatClient : public SocketClass
{
public:
	ChatClient() {}
	~ChatClient() {}
	
	// �ʱ�ȭ
	bool Start(const int port_, const char* ipAddress_);
	// ����� ������ ����Ǿ� ���α׷��� ������ �Ǵ��� Ȯ���ϴ� �Լ�.
	bool IsAbleToClose() { return mReadyToClose; }
	// ����ڰ� ����� �����Ű�� �Լ�
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


	// cmd�� �ʱ�ȭ�Ͽ� �������� ��µ� ���ڵ��� ����� �Լ��̴�.
	void ClearScreen();

	// ȭ���� �ۼ��ϴ� �Լ��̴�.
	void PrintScreen();

	// ȸ������ȭ�� �ۼ�
	void PrintSignUpPage() const;

	// �α���ȭ�� �ۼ�
	void PrintSignInPage() const;

	// cmd�� �ʱ�ȭ, ȭ�� ���ۼ� *Mutual Exclusion
	void RenewScreen();

	// �����ε�. cmd�� �ʱ�ȭ, ������ ��� �� ȭ�� ���ۼ�. *Mutual Exclusion
	void RenewScreen(std::string str_);

	// ä�ù濡�� �����ϴ� �Լ�
	void ExitRoom();

	// �Է��� �ޱ����� ������
	void InputThread();

	// ȸ�������̳� �α����� �Ķ���Ϳ� ���� ��ȿ�� �˻� (����, ���ڸ� ����)
	bool CheckValid(const std::string str_) const;

	// ������ �Ϸ�� �޽����� �������� ä�ù��� ����.
	void ActOnReceive(const std::string msg_);

	// ���꽺����Ʈ�� ����. (QUIT������Ʈ�� ����Ұ�)
	void ChangeSubState(SubState eSubState_);

	// ���ú�ť���� ������ �Ϸ�� �����͸� ���� ������. ������ �Ϸ���� �ʾҰų� ������ �߻��ϸ� ���ڿ��� �����Ѵ�.
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
