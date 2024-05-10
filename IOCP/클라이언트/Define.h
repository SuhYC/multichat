#pragma once

const int MAX_SOCK_BUF = 1024;

// 현재 유저의 상태를 표시. 이를 바탕으로 화면을 출력.
enum class ClientState
{
	SIGNINPAGE,
	SIGNUPPAGE,
	LOBBY,
	CHATROOM
};

// 현재 입력받아야하는 문자열이 무엇인지 표기. 송신후 대기중일경우 WAIT, 종료요청이 들어온경우 QUIT
enum class SubState
{
	ID,
	PW,
	NICKNAME,
	WAIT, // 입력불가능상태. (서버응답대기)
	QUIT // 통신 종료상태. (종료요청)
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
