#include "multichat_serv.h"

#define MAX_CLNT 256

void* handle_clnt(void* arg);
void send_msg(char* msg, int len);
void error_handling(const char* msg);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
std::mutex clnt_sock_mu;

int main() {

	WORD		wVersionRequested;
	WSADATA		wsaData;
	SOCKADDR_IN	servAddr, cliAddr;
	int			err;
	char		buf[BUF_SIZE]; // recv buf

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

	while (true) {
		int xx = sizeof(cliAddr);
		SOCKET clnt_sock = accept(s, reinterpret_cast<SOCKADDR*>(&cliAddr), &xx);

		clnt_sock_mu.lock();
		clnt_socks[clnt_cnt++] = clnt_sock;
		clnt_sock_mu.unlock();

		std::thread t = std::thread(handle_clnt, (void*)&clnt_sock);

		t.detach();
	}

	closesocket(s);
	WSACleanup();

	return 0;
}


void* handle_clnt(void* arg) {
	int clnt_sock = *((int*)arg);
	int str_len = 0;
	char msg[BUF_SIZE];

	while ((str_len = recv(clnt_sock, msg, sizeof(msg), 0)) != 0) {
		send_msg(msg, str_len);
	}

	clnt_sock_mu.lock();
	for (int i = 0; i < clnt_cnt; i++) {
		if (clnt_sock == clnt_socks[i]) {
			while (i++ < clnt_cnt - 1) {
				clnt_socks[i] = clnt_socks[i + 1];
			}
			break;
		}
	}
	clnt_cnt--;
	clnt_sock_mu.unlock();

	closesocket(clnt_sock);
	return NULL;
}

void send_msg(char* msg, int len) {
	clnt_sock_mu.lock();

	for (int i = 0; i < clnt_cnt; i++) {
		send(clnt_socks[i], msg, len, 0);
	}
	clnt_sock_mu.unlock();

	return;
}

void error_handling(const char* msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}