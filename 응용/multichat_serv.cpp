#include "multichat_serv.h"

void* handle_clnt(void* arg);
void error_handling(const char* msg);
class ChatRoom;
ChatRoom* createRoom();


std::mutex clnt_sock_mu;
std::vector<ChatRoom*> ChatRoomList;

int main() {

	WORD		wVersionRequested;
	WSADATA		wsaData;
	SOCKADDR_IN	servAddr, cliAddr;
	int			err;

	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);

	if (err != 0) {
		cout << "WSAStartup error " << WSAGetLastError() << endl;
		WSACleanup();
		return false;
	}

	servAddr.sin_family = AF_INET; // address family Internet
	servAddr.sin_port = htons(SERVER_PORT); //Port to connect on
	inet_pton(AF_INET, IPAddress, &(servAddr.sin_addr.s_addr)); //servAddr.sin_addr.s_addr = inet_addr(IPAddress); //Target IP


	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Create socket
	if (s == INVALID_SOCKET)
	{
		cout << "Socket error " << WSAGetLastError() << endl;
		WSACleanup();
		return false; //Couldn't create the socket
	}

	int x = bind(s, reinterpret_cast<SOCKADDR*>(&servAddr), sizeof(servAddr));
	if (x == SOCKET_ERROR)
	{
		cout << "Binding failed. Error code: " << WSAGetLastError() << endl;
		WSACleanup();
		return false; //Couldn't connect
	}

	cout << "Waiting for client..." << endl;

	if (listen(s, 5) == -1) {
		error_handling("listen() error");
	}

	tcp_keepalive tcpKeepAliveVals;
	tcpKeepAliveVals.onoff = 1;
	tcpKeepAliveVals.keepalivetime = 5500;
	tcpKeepAliveVals.keepaliveinterval = 1000;

	DWORD dwreturned;

	while (true) {
		int xx = sizeof(cliAddr);

		SOCKET clnt_sock = accept(s, reinterpret_cast<SOCKADDR*>(&cliAddr), &xx);
		
		//하트비트 옵션 추가.
		WSAIoctl(clnt_sock, SIO_KEEPALIVE_VALS, &tcpKeepAliveVals, sizeof(tcpKeepAliveVals), nullptr, 0, &dwreturned, nullptr, nullptr);

		std::thread t = std::thread(handle_clnt, (void*)&clnt_sock);

		t.detach();
	}

	closesocket(s);
	WSACleanup();

	return 0;
}

class ChatRoom {
private:
	std::vector<int> user;
	std::string chatcode;
public:
	ChatRoom(std::string code) {
		chatcode = code;
	}
	~ChatRoom() {
		for (int i = 0; i < ChatRoomList.size(); i++) {
			if (!strcmp(ChatRoomList[i]->getChatCode().c_str(), this->chatcode.c_str())) {
				ChatRoomList.erase(ChatRoomList.begin() + i);
				break;
			}
		}
		cout << "current chatroom count : " << ChatRoomList.size() << endl;
	}
	void removeUser(int userSocket) {
		if (user.size() == 1) { // 마지막 인원이 요청할 경우 메모리를 해제하고 리턴합니다.
			delete this;
			return;
		}

		for (int i = 0; i < user.size(); i++) { // 
			if (user[i] == userSocket) {
				user.erase(user.begin() + i);
				break;
			}
		}
	}
	void addUser(int userSocket) {
		user.push_back(userSocket);
	}
	void sendMessage(char* msg, int len) {
		for (int i = 0; i < user.size(); i++) {
			send(user[i], msg, len, 0);
		}
	}
	std::string getChatCode() {
		return chatcode;
	}
};

void* handle_clnt(void* arg) {
	int clnt_sock = *((int*)arg);
	int str_len = 0;
	char msg[BUF_SIZE];

	ChatRoom* cr = nullptr;

	clock_t last_recv_time = clock();
	bool ping_msg_sended = false;

	while ((str_len = recv(clnt_sock, msg, sizeof(msg), 0)) != 0) {
		clnt_sock_mu.lock();
		if (cr == nullptr) {
			/* msg q||Q 를 처리하지 않는 이유는
			* clnt 측이 채팅방 없는 상태에서 q||Q 입력시
			* 메시지를 보내지 않고 소켓을 종료하기 때문입니다.
			*/ 

			if (!strcmp(msg, "n") || !strcmp(msg, "N")) { // 새 채팅방 만들기
				cr = createRoom();
				cr->addUser(clnt_sock);
				send(clnt_sock, cr->getChatCode().c_str(), 6, 0);
			}
			else if(str_len == 8){ // 채팅방 코드로 참여하기
				for (int i = 0; i < ChatRoomList.size(); i++) {
					if (!strcmp(ChatRoomList[i]->getChatCode().c_str(), msg)) {
						cr = ChatRoomList[i];
						cr->addUser(clnt_sock);
						send(clnt_sock, msg, str_len, 0); // 채팅방 코드 일치한다는 신호
						break;
					}
				}
				if (cr == nullptr) { // 코드 틀렸음
					send(clnt_sock, "fail", sizeof("fail"), 0);
				}
			}
			else if(str_len == -1){ // SOCKET_ERROR
				cout << "disconnected" << endl;
				clnt_sock_mu.unlock();
				break;
			}
		}
		else { // 채팅방 있음
			if (str_len == -1) { // SOCKET_ERROR. 이 경우는 채팅방이 있는 상태에서 콘솔을 종료했을 가능성이 있음.
				cout << "disconnected" << endl;
				clnt_sock_mu.unlock();
				break;
			}

			if (!strcmp(msg, "q") || !strcmp(msg,"Q")) { // 나가기
				cr->removeUser(clnt_sock);
				cr = nullptr;
			}
			else { // 채팅 전파
				cr->sendMessage(msg, str_len);
			}
		}
		clnt_sock_mu.unlock();
	}


	clnt_sock_mu.lock();

	if (cr != nullptr) { // 클라이언트가 종료한 상황이니 채팅방을 나가지 않고 종료한 경우 반영
		cr->removeUser(clnt_sock);
		cr = nullptr;
	}

	clnt_sock_mu.unlock();

	closesocket(clnt_sock);
	return NULL;
}

void error_handling(const char* msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

ChatRoom* createRoom() {
	std::string str;

	srand(time(NULL));

	for (int i = 0; i < 6; i++) { // 챗코드 작성
		str = str + (char)('A' + rand() % 26);
	}

	for (int i = 0; i < ChatRoomList.size(); i++) {
		// 챗코드가 일치하는 경우 다시 챗코드를 만든다.
		if (!strcmp(ChatRoomList[i]->getChatCode().c_str(), str.c_str())) {
			str.clear();
			for (int j = 0; j < 6; j++) { // 챗코드 재작성
				str = str + (char)('A' + rand() % 26);
			}
			i = -1; // 다시 0번부터 확인
			continue;
		}
	}

	ChatRoom* cr = new ChatRoom(str);
	cout << "new ChatRoom Code : " << cr->getChatCode() << endl;
	ChatRoomList.push_back(cr);
	cout << "current chatroom count : " << ChatRoomList.size() << endl;

	return cr;
}

