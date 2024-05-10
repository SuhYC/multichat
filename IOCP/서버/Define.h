#pragma once
#include <winsock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib,"ws2_32.lib")

#define MAX_SOCKBUF 10000
#define MAX_WORKERTHREAD 12  //������ Ǯ�� ���� ������ ��
const UINT64 RE_USE_SESSION_WAIT_TIMESEC = 3;

enum class eIOOperation
{
	RECV,
	SEND,
	ACCEPT
};

//WSAOVERLAPPED����ü�� Ȯ�� ���Ѽ� �ʿ��� ������ �� �־���.
struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;		//Overlapped I/O����ü
	UINT		SessionIndex;			//Ŭ���̾�Ʈ ����
	WSABUF		m_wsaBuf;				//Overlapped I/O�۾� ����
	eIOOperation m_eOperation;			//�۾� ���� ����
};

enum class eReturnCode
{
	SIGNIN_SUCCESS = 101,
	SIGNIN_ALREADY = 102,
	SIGNIN_FAIL = 103,

	SIGNUP_SUCCESS = 201,
	SIGNUP_ALREADY = 202,
	SIGNUP_INVALID_ID = 203,
	SIGNUP_INVALID_PW = 204,
	SIGNUP_INVALID_NICK = 205,

	ENTERROOM_SUCCESS = 301,
	ENTERROOM_NOROOM = 302,
	ENTERROOM_INCORRECTCODE = 303
};
