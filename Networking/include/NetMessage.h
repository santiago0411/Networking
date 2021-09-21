#pragma once
#include "NetCommon.h"

namespace Networking
{
	template<typename T>
	struct MessageHeader
	{
		T Id{};
		uint32_t Size = 0;
	};

	template<typename T>
	struct Message
	{
		MessageHeader<T> Header{};
		std::vector<uint8_t> Body;

		size_t Size() const
		{
			return Body.size();
		}

		friend std::ostream& operator<<(std::ostream&os, const Message<T>& msg)
		{
			os << "Id: " << static_cast<int32_t>(msg.Header.Id) << " Size: " << msg.Header.Size;
			return os;
		}

		template<typename DataType>
		friend Message<T>& operator<<(Message<T>& msg, const DataType& data)
		{
			static_assert(std::is_standard_layout_v<DataType>, "Data is too complex to be pushed into vector");

			const size_t bodySize = msg.Body.size();
			msg.Body.resize(bodySize + sizeof(DataType));
			std::memcpy(msg.Body.data() + bodySize, &data, sizeof(DataType));
			msg.Header.Size = (uint32_t)msg.Size();

			return msg;
		}

		template<typename DataType>
		friend Message<T>& operator>>(Message<T>& msg, DataType& data)
		{
			static_assert(std::is_standard_layout_v<DataType>, "Data is too complex to be pulled out of vector");

			const size_t dataSize = msg.Body.size() - sizeof(DataType);
			std::memcpy(&data, msg.Body.data() + dataSize, sizeof(DataType));
			msg.Body.resize(dataSize);
			msg.Header.Size = (uint32_t)msg.Size();

			return msg;
		}
	};

	template<typename T>
	class Connection;

	template<typename T>
	struct OwnedMessage
	{
		std::shared_ptr<Connection<T>> Remote = nullptr;
		Message<T> Message;

		friend std::ostream& operator<<(std::ostream& os, const OwnedMessage& msg)
		{
			os << msg.Message;
			return os;
		}
	};
}