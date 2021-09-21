#include <iostream>
#include "Networking.h"

enum class CustomMessages : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage
};

class Client : public Networking::ClientInterface<CustomMessages>
{
public:
	~Client() override = default;

	void PingServer()
	{
		Networking::Message<CustomMessages> msg;
		msg.Header.Id = CustomMessages::ServerPing;

		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
		msg << timeNow;
		Send(msg);
	}

	void MessageAll()
	{
		Networking::Message<CustomMessages> msg;
		msg.Header.Id = CustomMessages::MessageAll;
		Send(msg);
	}
};

int main()
{
	Client c;
	c.Connect("127.0.0.1", 60000);

	bool key[3] = { false, false, false };
	bool oldKey[3] = { false, false, false };

	bool quit = false;
	while (!quit)
	{
		if (GetForegroundWindow() == GetConsoleWindow())
		{
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
		}

		if (key[0] && !oldKey[0])
			c.PingServer();

		if (key[1] && !oldKey[1])
			c.MessageAll();

		if (key[2] && !oldKey[2])
			quit = true;

		for (int i = 0; i < 3; i++)
			oldKey[i] = key[i];

		if (c.IsConnected())
		{
			if (!c.Incoming().IsEmpty())
			{
				auto msg = c.Incoming().PopFront().Message;

				switch (msg.Header.Id)
				{
					case CustomMessages::ServerAccept:
					{	
						std::cout << "Server Accepted Connection\n";
						break;
					}
					case CustomMessages::ServerPing:
					{
						std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
						std::chrono::system_clock::time_point timeThen;
						msg >> timeThen;
						std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
						break;
					}
					case CustomMessages::ServerMessage:
					{
						uint32_t clientId;
						msg >> clientId;
						std::cout << "Hello from [" << clientId << "]\n";
						break;
					}
				}
			}
		}
		else
		{
			std::cout << "Server Down\n";
			quit = true;
		}
	}
		
	return 0;
}