#include "ChatClient.h"

bool ChatClient::Start(const int port_, const char* ipAddress_)
{
	//bool bRet = InitSocket_Sync(port_, ipAddress_); // ���� Connect ���
	bool bRet = InitSocket(port_, ipAddress_); // �񵿱� Connect ���

	if (!bRet)
	{
		std::cerr << "[����] ChatClient::Start InitSocket()\n";
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
		std::cout << "����.";
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
		// �������� ���°� ���߿� ����.
		std::cout << "���ο� ä�ù��� ����÷��� /n�� �Է��Ͻð�, �����Ͻ÷��� /q�� �Է��Ͻð�, \n���� ä�ù濡 �����Ͻ÷��� ä�ù��� �ڵ带 �Է����ּ���.\n";

		break;
	case ClientState::CHATROOM:
		std::cout << "ä�ù� �ڵ� : " << mRoomCode << "\n";

		for (std::string str : mChatLog)
		{
			std::cout << str << "\n";
		}

		std::cout << "�Է� : " << mLastInput;

		break;
	default:
		std::cerr << "[����] ChatClient::PrintScreen ClientState ������\n";
		break;
	}
}

void ChatClient::PrintSignUpPage() const
{
	switch (mSubState)
	{
	case SubState::ID:
		std::cout << "�����빮��, �����ҹ���, ���ڷ� ������ 6~10�� ID�� �Է����ּ���.\n�α����Ͻ÷��� /L�� �Է����ּ���.\n";
		break;
	case SubState::PW:
		std::cout << "�����빮��, �����ҹ���, ���ڷ� ������ 6~10�� ��й�ȣ�� �Է����ּ���.\n";
		break;
	case SubState::NICKNAME:
		std::cout << "�����빮��, �����ҹ���, ���ڷ� ������ 2~10�� �г����� �Է����ּ���.\n";
		break;
	case SubState::WAIT:
		//std::cout << "��ø� ��ٷ� �ּ���.\n";
		break;
	default:
		std::cerr << "[����] ChatClient::PrintSignUpPage SubState ������\n";
		break;
	}

	return;
}

void ChatClient::PrintSignInPage() const
{
	switch (mSubState)
	{
	case SubState::ID:
		std::cout << "ID�� �Է����ּ���.\nȸ�������Ͻ÷��� /N�� �Է����ּ���.\n";
		break;
	case SubState::PW:
		std::cout << "��й�ȣ�� �Է����ּ���.\n";
		break;
	case SubState::WAIT:
		//std::cout << "��ø� ��ٷ� �ּ���.\n";
		break;
	default:
		std::cerr << "[����] ChatClient::PrintSignInPage SubState ������\n";
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
		// �����û
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

			// _getch() : ���۸� ������� �ʱ� ������ ����Ű�� ������ �ʾƵ� �����Ѵ�.
			// �����κ��� �����͸� �޾� �׶��׶� ä���� �ݿ��ϱ� �����̴�.
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
				// ä�ù� ������
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
					RenewScreen("ID�� ��ȿ���� �ʽ��ϴ�.\n");
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
					RenewScreen("��й�ȣ�� ��ȿ���� �ʽ��ϴ�.\n");
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
					RenewScreen("ID�� ��ȿ���� �ʽ��ϴ�.\n");
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
					RenewScreen("��й�ȣ�� ��ȿ���� �ʽ��ϴ�.\n");
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
					RenewScreen("�г����� ��ȿ���� �ʽ��ϴ�.\n");
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

			// ä�����α׷� ���� ��û
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

		// ������ ���ڵ� �ƴϸ� ��ȿ.
		return false;
	}

	// ������ ���ڷθ� �̷���������� ��ȿ.
	return true;
}

void ChatClient::ActOnReceive(const std::string msg_)
{
	if (mState == ClientState::SIGNINPAGE)
	{
		// if �α��� ���� ~ else �α��� ���� !! �̻��� ������ ���� �Ÿ���

		// �α��� ���� �ڵ�
		if (!msg_.compare("[101]"))
		{
			ChangeSubState(SubState::ID);
			mState = ClientState::LOBBY;

			RenewScreen();
		}
		// �α��� ���� �ڵ�
		else if (!msg_.compare("[103]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("���̵� Ȥ�� ��й�ȣ�� �߸� �Է��ϼ̽��ϴ�.\n");
		}
		// �̹� �α��� �� �ڵ�
		else if (!msg_.compare("[102]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("�̹� �α��ε� ���̵��Դϴ�.\n");
		}
		// �߸��� ���� �ڵ�
		else
		{
			return;
		}
	}
	else if (mState == ClientState::SIGNUPPAGE)
	{
		// if ȸ������ ���� ~ else ȸ������ ���� !! �̻��� ������ ���� �Ÿ���

		// ȸ������ ���� �ڵ�
		if (!msg_.compare("[201]") || !msg_.compare("[101]"))
		{
			ChangeSubState(SubState::ID);
			mState = ClientState::LOBBY;
			RenewScreen();
		}
		// ���̵� �ߺ� �ڵ�
		else if (!msg_.compare("[202]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("�̹� ������ ID�Դϴ�.\n");
		}
		// ���̵� ���� �Ҹ��� �ڵ�
		else if (!msg_.compare("[203]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("ID ������ ���� �ʽ��ϴ�.\n");
		}
		// ��й�ȣ ���� �Ҹ��� �ڵ�
		else if (!msg_.compare("[204]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("��й�ȣ ������ ���� �ʽ��ϴ�.\n");
		}
		// �г��� ���� �Ҹ��� �ڵ�
		else if (!msg_.compare("[205]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("�г��� ������ ���� �ʽ��ϴ�.\n");
		}
		// �߸��� ���� �ڵ�
		else
		{
			return;
		}
	}
	else if (mState == ClientState::LOBBY)
	{
		// if �� ���� ���� ~ else �� ���� ���� !! �̻��� ������ ���� �Ÿ���

		// �� ���� ����
		if (msg_.size() == 6)
		{
			mRoomCode = msg_;
			mState = ClientState::CHATROOM;
			ChangeSubState(SubState::ID);
		}
		// �� ���� ����
		else if (!msg_.compare("[302]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("���� �ʹ� ���� ������ �� �����ϴ�.\n");
		}
		// �� ���� ����
		else if (!msg_.compare("[303]"))
		{
			ChangeSubState(SubState::ID);
			RenewScreen("�ڵ尡 �߸��Ǿ����ϴ�.\n");
		}
		// �߸��� ���� �ڵ�
		else
		{
			//return;
		}
	}
	else if (mState == ClientState::CHATROOM)
	{
		// ������ �г����� �������� �ٿ��� ������ ����!
		mChatLog.push_back(msg_);
		RenewScreen();
	}

	return;
}

std::string ChatClient::PopFront()
{
	// ���� ����ִ� �����Ͱ� ����
	if (mReceiveQueue.empty())
	{
		return std::string();
	}

	// ����κ��� �ۼ����� ���� ������
	if (mReceiveQueue[0] > '9' || mReceiveQueue[0] < '0')
	{
		std::cerr << "[����] ChatClient::CheckReceiveQueue �޽���ť ������ ����\n";
		return std::string();
	}

	// ���� �������� ũ��
	int packetsize = 0;
	// packetsize�� �ڸ���
	int header = 0;

	for (int i = 0; i < mReceiveQueue.size(); i++)
	{
		// ���ڵ����Ͱ� ���������� ����
		if (mReceiveQueue[i] <= '9' && mReceiveQueue[i] >= '0')
		{
			header++;
			packetsize *= 10;
			packetsize += mReceiveQueue[i] - '0';
			continue;
		}
		break;
	}

	// �������� ���̰� 0 �����ϸ��� ����
	if (packetsize <= 0)
	{
		std::cerr << "[����] ChatClient::PopFront ���ڿ� ������ ����\n";
		return std::string();
	}

	// ���� ������ �Ϸ���� ����
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
			std::cout << "����Ű�� �Է����ּ���.\n";
		}
		else
		{
			std::cout << "�ƹ�Ű�� �Է����ּ���.\n";
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