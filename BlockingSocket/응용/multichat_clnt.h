#ifndef CLIENT_MAIN_H
#define CLIENT_MAIN_H

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include <iostream>
using std::cout;
using std::endl;

#include <thread>
#include <string>
#include <stdlib.h>
#include <mutex>

#define SERVER_PORT 11235
#define BUF_SIZE 4096
#define QUEUE_SIZE 10
#define IPAddress "127.0.0.1"

#endif // !CLIENT_MAIN_H
