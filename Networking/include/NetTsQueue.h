#pragma once

#include "NetCommon.h"

namespace Networking
{
	template<typename T>
	class TsQueue
	{
	public:
		TsQueue() = default;
		TsQueue(const TsQueue<T>&) = delete;
		virtual ~TsQueue() { Clear(); }

		const T& Front()
		{
			std::scoped_lock lock(m_MuxQueue);
			return m_Deque.front();
		}

		const T& Back()
		{
			std::scoped_lock lock(m_MuxQueue);
			return m_Deque.back();
		}


		T PopFront()
		{
			std::scoped_lock lock(m_MuxQueue);
			auto t = std::move(m_Deque.front());
			m_Deque.pop_front();
			return t;
		}

		T PopBack()
		{
			std::scoped_lock lock(m_MuxQueue);
			auto t = std::move(m_Deque.back());
			m_Deque.pop_back();
			return t;
		}

		void PushFront(const T& item)
		{
			std::scoped_lock lock(m_MuxQueue);
			m_Deque.emplace_front(std::move(item));

			std::unique_lock ul(m_MuxBlocking);
			m_CvBlocking.notify_one();
		}

		void PushBack(const T& item)
		{
			std::scoped_lock lock(m_MuxQueue);
			m_Deque.emplace_back(std::move(item));

			std::unique_lock ul(m_MuxBlocking);
			m_CvBlocking.notify_one();
		}

		bool IsEmpty()
		{
			std::scoped_lock lock(m_MuxQueue);
			return m_Deque.empty();
		}

		size_t Count()
		{
			std::scoped_lock lock(m_MuxQueue);
			return m_Deque.size();
		}

		void Clear()
		{
			std::scoped_lock lock(m_MuxQueue);
			m_Deque.clear();
		}

		void Wait()
		{
			while (IsEmpty())
			{
				std::unique_lock ul(m_MuxBlocking);
				m_CvBlocking.wait(ul);
			}
		}

	private:
		std::mutex m_MuxQueue;
		std::deque<T> m_Deque;
		std::condition_variable m_CvBlocking;
		std::mutex m_MuxBlocking;
	};
}