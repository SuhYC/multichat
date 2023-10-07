#include "multichat_clnt.h"

void* send_msg(void* arg);
void* recv_msg(void* arg);

std::string name;
std::string chatcode = "";
char msg[BUF_SIZE];

std::mutex mu;

int main() {

    WORD		wVersionRequested;
    WSADATA		wsaData;
    SOCKADDR_IN target; //Socket address information
    SOCKET      s;
    int			err;
    char        buf[BUF_SIZE];

    wVersionRequested = MAKEWORD(1, 1);
    err = WSAStartup(wVersionRequested, &wsaData);

    if (err != 0) {
        printf("WSAStartup error %ld", WSAGetLastError());
        WSACleanup();
        return 0;
    }

    target.sin_family = AF_INET; // address family Internet
    target.sin_port = htons(SERVER_PORT); //Port to connect on
    inet_pton(AF_INET, IPAddress, &(target.sin_addr.s_addr)); //target.sin_addr.s_addr = inet_addr(IPAddress)


    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Create socket
    if (s == INVALID_SOCKET)
    {
        cout << "socket() error : " << WSAGetLastError() << endl;
        WSACleanup();
        return 0; //Couldn't create the socket
    }


    if (connect(s, reinterpret_cast<SOCKADDR*>(&target), sizeof(target)) == SOCKET_ERROR)
    {
        cout << "connect() error : " << WSAGetLastError() << endl;
        cout << "���� ����." << endl;
        WSACleanup();
        return 0; //Couldn't connect
    }
    
    std::string str = "12345678901";
    while (str.size() > 10) {
        cout << "�̸��� �Է����ּ��� (10�� �̸�)" << endl;
        std::cin >> str;
    }
    
    std::cin.ignore();
    name = "[" + str + "]";

    std::thread recv_t(recv_msg, (void*)&s);
    std::thread send_t(send_msg, (void*)&s);

    recv_t.join();
    send_t.join();

    closesocket(s);
    WSACleanup();
    
    return 0;
}

void* recv_msg(void* s) {
    SOCKET sock = *((int*)s);
    char buf[400];
    std::string msg;
    int str_len;

    while (true) {
        str_len = recv(sock, buf, 350, 0);
        if (str_len == -1) {
            return NULL;
        }

        buf[str_len] = 0;
        msg = std::string(buf);

        mu.lock();

        if (chatcode.empty()) { // �������� ä�ù��� ���� ���
            if (msg.size() == 6) { // ä�ù� �ڵ带 �����κ��� ���� == ���������� ä�ù濡 ������
                chatcode = msg;
                system("cls");
                cout << "�� �г��� : " << name  << endl << "���� �������� ä�ù� �ڵ� : " << chatcode << "\n" << endl;
            }
            else { // ä�ù� �ڵ� Ʋ��
                cout << msg << endl;
                cout << "�ڵ尡 ��ġ�ϴ� ä�ù��� �����ϴ�." << endl;
            }
        }
        else {
            cout << msg << endl;
        }

        mu.unlock();

    }

    return NULL;
}

void* send_msg(void* s) {
    SOCKET sock = *((int*)s);

    std::string msg;

    while (true) {
        std::getline(std::cin, msg);
        if (!strcmp(msg.c_str(), "q") || !strcmp(msg.c_str(), "Q")) {
            mu.lock();
            if (chatcode.empty()) {
                mu.unlock();
                closesocket(sock);
                exit(0);
            }
            else {
                chatcode = "";
                system("cls");
                send(sock, "q", sizeof("q"), 0);
                cout << "�������� ä�ù��� �����ϴ�. ���ο� ���� ����ðų� ä�ù� �ڵ带 �Է��Ͽ� �����Ͻʽÿ�." << endl;
                mu.unlock();
                continue;
            }
        }

        if (chatcode.empty()) { // �� ä�ù� ����� Ȥ�� �����ϱ�
            send(sock, msg.c_str(), sizeof(msg.c_str()), 0);
            continue;
        }

        // ������ʹ� ä�� ���

        msg = name + " : " + msg;
        if (msg.size() > 300) {
            cout << "�ʹ� ��." << endl;
            continue;
        }
        send(sock, msg.c_str(), sizeof(msg), 0);
    }
}