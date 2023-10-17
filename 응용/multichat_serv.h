#ifndef SERVER_MAIN_H
#define SERVER_MAIN_H

#include <WS2tcpip.h>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

#include <iostream>

using std::cout;
using std::endl;

#include <thread>
#include <mutex>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <mstcpip.h>

#define SERVER_PORT 11235
#define BUF_SIZE 4096
#define QUEUE_SIZE 10
#define IPAddress "127.0.0.1"


#endif // !SERVER_MAIN_H