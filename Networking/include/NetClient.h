#pragma once

#include "NetCommon.h"
#include "NetConnection.h"
#include "NetMessage.h"
#include "NetTsQueue.h"

namespace Networking
{
	template<typename T>
	class ClientInterface
	{
	public:
		ClientInterface()
			: m_Socket(m_AsioContext)
		{
			
		}

		virtual ~ClientInterface()
		{
			Disconnect();
		}

		bool Connect(const std::string& host, const uint16_t port)
		{
			try
			{
				asio::ip::tcp::resolver resolver(m_AsioContext);
				auto endpoints = resolver.resolve(host, std::to_string(port));

				m_Connection = std::make_unique<Connection<T>>(Connection<T>::Owner::Client, m_AsioContext, asio::ip::tcp::socket(m_AsioContext), m_InMessages);
				m_Connection->ConnectToServer(endpoints);

				m_ThrContext = std::thread([this]() { m_AsioContext.run(); });

				return true;
			}
			catch (std::exception& e)
			{
				std::cerr << "Client Exception: " << e.what() << '\n';
				return false;
			}
		}

		void Disconnect()
		{
			if (IsConnected())
				m_Connection->Disconnect();

			m_AsioContext.stop();
			if (m_ThrContext.joinable())
				m_ThrContext.join();

			m_Connection.release();
		}

		bool IsConnected()
		{
			return m_Connection ? m_Connection->IsConnected() : false;
		}

		void Send(const Message<T>& msg)
		{
			if (IsConnected())
				m_Connection->Send(msg);
		}

		TsQueue<OwnedMessage<T>>& Incoming()
		{
			return m_InMessages;
		}

	private:
		asio::io_context m_AsioContext;
		std::thread m_ThrContext;
		asio::ip::tcp::socket m_Socket;
		std::unique_ptr<Connection<T>> m_Connection;

		TsQueue<OwnedMessage<T>> m_InMessages;
	};
}
