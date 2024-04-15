#include "ChatClient.h"

const UINT16 PORT = 11021;
const char* IPADDRESS = "127.0.0.1";

int main()
{
	ChatClient c;
	c.Start(PORT, IPADDRESS);
	
	while (!c.IsAbleToClose())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	c.End();

	return 0;
}