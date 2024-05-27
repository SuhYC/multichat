#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#ifdef _DEBUG
	#ifndef DBG_NEW
		#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
		#define new DBG_NEW
	#endif
#endif // _DEBUG

#include "ChatServer.h"

const UINT16 SERVER_PORT = 11021;
const UINT16 MAX_CLIENT = 100;		//총 접속할수 있는 클라이언트 수
const UINT32 MAX_ROOM = 100;		//최대 채팅방의 갯수

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	ChatServer server;

	server.Init(SERVER_PORT);
	server.Run(MAX_CLIENT, MAX_ROOM);

	std::cout << "아무 키나 누를 때까지 대기합니다\n";
	getchar();

	server.End();
	return 0;
}
