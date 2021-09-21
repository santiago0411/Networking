#pragma once

#include "NetCommon.h"
#include "NetConnection.h"
#include "NetMessage.h"
#include "NetTsQueue.h"

namespace Networking
{
	template<typename T>
	class ServerInterface
	{
	public:
		ServerInterface(uint16_t port)
			: m_Acceptor(m_AsioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
		{

		}

		virtual ~ServerInterface()
		{
			Stop();
		}

		bool Start()
		{
			try
			{
				AwaitClientConnection();

				m_ThrContext = std::thread([this]() { m_AsioContext.run(); });

				std::cout << "[SERVER] Started!\n";
				return true;
			}
			catch (std::exception& e)
			{
				std::cerr << "[SERVER] Exception: " << e.what() << '\n';
				return false;
			}
		}

		void Stop()
		{
			m_AsioContext.stop();
			if (m_ThrContext.joinable())
				m_ThrContext.join();

			std::cout << "[SERVER] Stopped!\n";
		}

		void AwaitClientConnection()
		{
			m_Acceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket)
			{
				if (!ec)
				{
					std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << '\n';

					auto newConn = std::make_shared<Connection<T>>(Connection<T>::Owner::Server, m_AsioContext, std::move(socket), m_InMessages);

					if (OnClientConnected(newConn))
					{
						m_Connections.push_back(std::move(newConn));
						m_Connections.back()->ConnectToClient(m_IdCounter++, NET_BIND_FN(ServerInterface::OnClientValidated));

						std::cout << '[' << m_Connections.back()->GetId() << "] Connection Approved\n";
					}
					else
					{
						std::cout << "[-----] Connection Denied\n";
					}
				}
				else
				{
					std::cerr << "[SERVER] New Connection Error: " << ec.message() << '\n';
				}

				AwaitClientConnection();
			});
		}

		void MessageClient(std::shared_ptr<Connection<T>> client, const Message<T>& msg)
		{
			if (client && client->IsConnected())
			{
				client->Send(msg);
			}
			else
			{
				OnClientDisconnected(client);
				client.reset();
				m_Connections.erase(std::remove(m_Connections.begin(), m_Connections.end(), client), m_Connections.end());
			}
		}

		void MessageAllClients(const Message<T>& msg, std::shared_ptr<Connection<T>> ignoreClient = nullptr)
		{
			bool invalidClientsExist = false;

			for (auto& client : m_Connections)
			{
				if (client && client->IsConnected())
				{
					if (client != ignoreClient)
						client->Send(msg);
				}
				else
				{
					OnClientDisconnected(client);
					client.reset();
					invalidClientsExist = true;
				}
			}

			if (invalidClientsExist)
				m_Connections.erase(std::remove(m_Connections.begin(), m_Connections.end(), nullptr), m_Connections.end());
		}

		void Update(size_t maxMessages = -1, bool wait = false)
		{
			if (wait)
				m_InMessages.Wait();

			size_t messageCount = 0;
			while (messageCount < maxMessages && !m_InMessages.IsEmpty())
			{
				auto msg = m_InMessages.PopFront();
				OnMessage(msg.Remote, msg.Message);
				messageCount++;
			}
		}


	protected:
		// Called when a client connects, returning false will discard the connection.
		virtual bool OnClientConnected(std::shared_ptr<Connection<T>> client) { return true; }
		// Called once the client has successfully passed the handshake
		virtual void OnClientValidated(std::shared_ptr<Connection<T>> client) {}
		// Called when a client disconnects.
		virtual void OnClientDisconnected(std::shared_ptr<Connection<T>> client) {}
		// Called when a message arrives
		virtual void OnMessage(std::shared_ptr<Connection<T>> client, Message<T>& msg) = 0;

	private:
		asio::io_context m_AsioContext;
		std::thread m_ThrContext;
		asio::ip::tcp::acceptor m_Acceptor;

		uint32_t m_IdCounter = 10000;

		TsQueue<OwnedMessage<T>> m_InMessages;
		std::deque<std::shared_ptr<Connection<T>>> m_Connections;
	};
}
