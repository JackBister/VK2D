#pragma once

#include <concurrentqueue.h>
#include <experimental/variant.hpp>

#include "Core/Maybe.h"

/*
	Queue
	Wraps MoodyCamel's lockless MPMC queue with an (IMO) nicer API.
	GetReader() creates a consumer, GetWriter() creates a producer.
	Neither readers nor writers can be copied, only moved.
	The expectation is that a thread will only have one writer/consumer per queue
	and that writers/consumers won't be moved between threads or used by multiple threads.
	That means they should be kept in thread-local storage only.
*/

template<typename T>
struct Queue
{
	struct Reader
	{
		friend class Queue;
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
		friend class Queue;
		void Push(T val)
		{
			queue->enqueue(token, val);
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