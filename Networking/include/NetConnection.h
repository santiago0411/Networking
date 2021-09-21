#pragma once

#include "NetCommon.h"
#include "NetMessage.h"
#include "NetTsQueue.h"

namespace Networking
{
	template<typename T>
	class Connection : public std::enable_shared_from_this<Connection<T>>
	{
		using OnClientValidated = std::function<void(std::shared_ptr<Connection<T>>)>;

	public:
		enum class Owner
		{
			Server,
			Client
		};

	public:
		Connection(Owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, TsQueue<OwnedMessage<T>>& qIn)
			: m_AsioContext(asioContext), m_Socket(std::move(socket)), m_InMessages(qIn)
		{
			m_OwnerType = parent;

			if (m_OwnerType == Owner::Server)
			{
				m_HandshakeOut = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
				m_HandshakeCheck = Scramble(m_HandshakeOut);
			}
		}

		virtual ~Connection() = default;

		uint32_t GetId() const { return m_Id; }

		void ConnectToClient(const uint32_t id, OnClientValidated clientValidatedCb)
		{
			if (m_OwnerType == Owner::Server)
			{
				if (m_Socket.is_open())
				{
					m_Id = id;
					WriteValidation();
					ReadValidation(clientValidatedCb);
				}
			}
		}

		void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
		{
			if (m_OwnerType == Owner::Client)
			{
				asio::async_connect(m_Socket, endpoints,
					[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
					{
						if (!ec)
							ReadValidation();
					});
			}
		}

		void Disconnect()
		{
			if (IsConnected())
				asio::post(m_AsioContext, [this]() { m_Socket.close(); });
		}

		bool IsConnected() const
		{
			return m_Socket.is_open();
		}

		void Send(const Message<T>& msg)
		{
			asio::post(m_AsioContext, [this, msg]()
			{
				const bool writingMessage = !m_OutMessages.IsEmpty();
				m_OutMessages.PushBack(msg);
				if (!writingMessage)
					WriteHeader();
			});
		}

	private:
		void ReadHeader()
		{
			asio::async_read(m_Socket, asio::buffer(&m_TmpInMsg.Header, sizeof(MessageHeader<T>)), 
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_TmpInMsg.Header.Size > 0)
						{
							m_TmpInMsg.Body.resize(m_TmpInMsg.Header.Size);
							ReadBody();
						}
						else
						{
							AddToIncomingMessageQueue();
						}
					}
					else
					{
						std::cerr << '[' << m_Id << "] Read Header Failed.\n";
						m_Socket.close();
					}
				});
		}

		void ReadBody()
		{
			asio::async_read(m_Socket, asio::buffer(m_TmpInMsg.Body.data(), m_TmpInMsg.Body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						AddToIncomingMessageQueue();
					}
					else
					{
						std::cerr << '[' << m_Id << "] Read Body Failed.\n";
						m_Socket.close();
					}
				});
		}

		void WriteHeader()
		{
			asio::async_write(m_Socket, asio::buffer(&m_OutMessages.Front().Header, sizeof(MessageHeader<T>)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_OutMessages.Front().Body.size() > 0)
						{
							WriteBody();
						}
						else
						{
							m_OutMessages.PopFront();

							if (!m_OutMessages.IsEmpty())
								WriteHeader();
						}
					}
					else
					{
						std::cerr << '[' << m_Id << "] Write Header Failed.\n";
						m_Socket.close();
					}
				});
		}

		void WriteBody()
		{
			asio::async_write(m_Socket, asio::buffer(m_OutMessages.Front().Body.data(), m_OutMessages.Front().Body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						m_OutMessages.PopFront();

						if (!m_OutMessages.IsEmpty())
							WriteHeader();
					}
					else
					{
						std::cerr << '[' << m_Id << "] Write Body Failed.\n";
						m_Socket.close();
					}
				});
		}

		void AddToIncomingMessageQueue()
		{
			if (m_OwnerType == Owner::Server)
				m_InMessages.PushBack({ this->shared_from_this(), m_TmpInMsg });
			else
				m_InMessages.PushBack({ nullptr, m_TmpInMsg });

			ReadHeader();
		}

		uint64_t Scramble(uint64_t input)
		{
			uint64_t out = input ^ 0xDEADBEEFC0DECAFE;
			out = (out & 0xF0F0F0F0F0F0) >> 6 | (out & 0x0F13EAC6F35) << 2;
			return ~(out ^ 0x500913261A);
		}

		void WriteValidation()
		{
			asio::async_write(m_Socket, asio::buffer(&m_HandshakeOut, sizeof(uint64_t)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_OwnerType == Owner::Client)
							ReadHeader();
					}
					else
					{
						m_Socket.close();
					}
				});
		}

		void ReadValidation(OnClientValidated clientValidatedCb = nullptr)
		{
			asio::async_read(m_Socket, asio::buffer(&m_HandshakeIn, sizeof(uint64_t)),
				[this, clientValidatedCb](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_OwnerType == Owner::Server)
						{
							if (m_HandshakeIn == m_HandshakeCheck)
							{
								std::cout << "Client Validated\n";
								clientValidatedCb(this->shared_from_this());

								ReadHeader();
							}
							else
							{
								std::cout << "Client Disconnected (Failed Validation)\n";
								m_Socket.close();
							}
						}
						else
						{
							m_HandshakeOut = Scramble(m_HandshakeIn);
							WriteValidation();
						}
					}
					else
					{
						std::cout << "Client Disconnected (Read Validation)\n";
						m_Socket.close();
					}
				});
		}

	private:
		asio::io_context& m_AsioContext;
		asio::ip::tcp::socket m_Socket;

		TsQueue<Message<T>> m_OutMessages;
		TsQueue<OwnedMessage<T>>& m_InMessages;
		Message<T> m_TmpInMsg;

		Owner m_OwnerType;
		uint32_t m_Id = 0;

		uint64_t m_HandshakeOut = 0;
		uint64_t m_HandshakeIn = 0;
		uint64_t m_HandshakeCheck = 0;
	};
}
