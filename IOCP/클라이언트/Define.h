#pragma once

const int MAX_SOCK_BUF = 1024;

// ���� ������ ���¸� ǥ��. �̸� �������� ȭ���� ���.
enum class ClientState
{
	SIGNINPAGE,
	SIGNUPPAGE,
	LOBBY,
	CHATROOM
};

// ���� �Է¹޾ƾ��ϴ� ���ڿ��� �������� ǥ��. �۽��� ������ϰ�� WAIT, �����û�� ���°�� QUIT
enum class SubState
{
	ID,
	PW,
	NICKNAME,
	WAIT, // �ԷºҰ��ɻ���. (����������)
	QUIT // ��� �������. (�����û)
};

enum class eIOOperation
{
	CONNECT,
	RECV,
	SEND
};

struct stOverlappedEx
{
	WSAOVERLAPPED overlapped;
	WSABUF m_wsaBuf;
	eIOOperation m_eOperation;
};
