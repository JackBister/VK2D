#pragma once

#include <concurrentqueue.h>
#if _MSC_VER && !__INTEL_COMPILER
#include <variant>
#else
#include <experimental/variant.hpp>
using std::variant = std::experimental::variant;
#endif

#include "Core/Maybe.h"
#include "Core/Semaphore.h"

/*
	Queue
	Wraps MoodyCamel's lockless MPMC queue with an (IMO) nicer API.
	GetReader() creates a consumer, GetWriter() creates a producer.
	Neither readers nor writers can be copied, only moved.
	The expectation is that a thread will only have one writer/consumer per queue
	and that writers/consumers won't be moved between threads or used by multiple threads.
*/

template<typename T, bool UseSemaphore = true>
struct Queue
{
};

template<typename T>
struct Queue<T, false>
{
	struct Reader
	{
		friend struct Queue;
		Maybe<T> Pop()
		{
			T ret;
			if (queue->try_dequeue(token, ret)) {
				return ret;
			}
			return None{};
		}

		Reader(Reader&& rhs) : queue(rhs.queue), token(std::move(rhs.token)) {}
	private:
		Reader(moodycamel::ConcurrentQueue<T> * q) : queue(q), token(*q) {}
		moodycamel::ConcurrentQueue<T> * queue;
		moodycamel::ConsumerToken token;
	};

	struct Writer
	{
		friend struct Queue;
		void Push(T&& val)
		{
			queue->enqueue(token, std::move(val));
		}

		Writer(Writer&& rhs) : queue(rhs.queue), token(std::move(rhs.token)) {}
	private:
		Writer(moodycamel::ConcurrentQueue<T> * q) : queue(q), token(*q) {}
		moodycamel::ConcurrentQueue<T> * queue;
		moodycamel::ProducerToken token;
	};

	Reader GetReader()
	{
		return Reader(&queue);
	}

	Writer GetWriter()
	{
		return Writer(&queue);
	}
private:
	moodycamel::ConcurrentQueue<T> queue;
};


template<typename T>
struct Queue<T, true>
{
	struct Reader
	{
		friend struct Queue;
		Maybe<T> Pop()
		{
			T ret;
			if (queue->try_dequeue(token, ret)) {
				sem->Wait();
				return ret;
			}
			return None{};
		}

		T Wait()
		{
			T ret;
			sem->Wait();
			if (!queue->try_dequeue(token, ret)) {
				assert(false);
			}
			return ret;
		}

		Reader(Reader&& rhs) : queue(rhs.queue), token(std::move(rhs.token)), sem(rhs.sem) {}
	private:
		Reader(moodycamel::ConcurrentQueue<T> * q, Semaphore * sem) : queue(q), token(*q), sem(sem) {}
		moodycamel::ConcurrentQueue<T> * queue;
		moodycamel::ConsumerToken token;

		Semaphore * sem;
	};

	struct Writer
	{
		friend struct Queue;
		void Push(T&& val)
		{
			queue->enqueue(token, std::move(val));
			sem->Signal();
		}

		Writer(Writer&& rhs) : queue(rhs.queue), token(std::move(rhs.token)), sem(rhs.sem) {}
	private:
		Writer(moodycamel::ConcurrentQueue<T> * q, Semaphore * sem) : queue(q), token(*q), sem(sem) {}
		moodycamel::ConcurrentQueue<T> * queue;
		moodycamel::ProducerToken token;

		Semaphore * sem;
	};

	Reader GetReader()
	{
		return Reader(&queue, &sem);
	}

	Writer GetWriter()
	{
		return Writer(&queue, &sem);
	}
private:
	moodycamel::ConcurrentQueue<T> queue;

	Semaphore sem;
};
