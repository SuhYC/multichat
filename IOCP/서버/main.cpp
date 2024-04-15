#include "ChatServer.h"

const UINT16 SERVER_PORT = 11021;
const UINT16 MAX_CLIENT = 100;		//�� �����Ҽ� �ִ� Ŭ���̾�Ʈ ��
const UINT32 MAX_ROOM = 100;		//�ִ� ä�ù��� ����

int main()
{
	ChatServer server;

	server.Init(SERVER_PORT);
	server.Run(MAX_CLIENT, MAX_ROOM);

	std::cout << "�ƹ� Ű�� ���� ������ ����մϴ�\n";
	getchar();

	server.End();
	return 0;
}
