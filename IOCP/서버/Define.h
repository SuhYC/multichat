#pragma once
#include <winsock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib,"ws2_32.lib")

#define MAX_SOCKBUF 10000
#define MAX_WORKERTHREAD 12  //쓰레드 풀에 넣을 쓰레드 수
const UINT64 RE_USE_SESSION_WAIT_TIMESEC = 3;

enum class eIOOperation
{
	RECV,
	SEND,
	ACCEPT
};

//WSAOVERLAPPED구조체를 확장 시켜서 필요한 정보를 더 넣었다.
struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;		//Overlapped I/O구조체
	UINT		SessionIndex;			//클라이언트 소켓
	WSABUF		m_wsaBuf;				//Overlapped I/O작업 버퍼
	eIOOperation m_eOperation;			//작업 동작 종류
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
