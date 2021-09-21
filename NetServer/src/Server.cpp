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

class Server : public Networking::ServerInterface<CustomMessages>
{
public:
	Server(uint16_t port)
		: ServerInterface<CustomMessages>(port)
	{
		
	}

	~Server() override = default;

protected:
	bool OnClientConnected(std::shared_ptr<Networking::Connection<CustomMessages>> client) override
	{
		Networking::Message<CustomMessages> msg;
		msg.Header.Id = CustomMessages::ServerAccept;
		client->Send(msg);

		return true;
	}

	void OnClientDisconnected(std::shared_ptr<Networking::Connection<CustomMessages>> client) override
	{
		std::cout << "Removing client [" << client->GetId() << "]\n";
	}

	void OnMessage(std::shared_ptr<Networking::Connection<CustomMessages>> client, Networking::Message<CustomMessages>& msg) override
	{
		switch (msg.Header.Id)
		{
			case CustomMessages::ServerPing:
			{
				std::cout << '[' << client->GetId() << "]: Server Ping\n";
				client->Send(msg);
				break;
			}
			case CustomMessages::MessageAll:
			{
				std::cout << '[' << client->GetId() << "]: Message All\n";
				Networking::Message<CustomMessages> msg;
				msg.Header.Id = CustomMessages::ServerMessage;
				msg << client->GetId();
				MessageAllClients(msg, client);
				break;
			}
		}
	}
};

int main()
{
	Server server(60000);
	server.Start();

	for (;;)
	{
		server.Update(-1, true);
	}
}