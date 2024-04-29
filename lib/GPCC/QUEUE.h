// QUEUE.h: (10/12/23, RD)

// references:
// - SpecC
// - SystemC
// - https://stackoverflow.com/questions/36762248/why-is-stdqueue-not-thread-safe

#ifndef QUEUE_H
#define QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class QUEUE_IN
{
    public:
	virtual ~QUEUE_IN(void){};
	virtual void POP(T&) = 0;
};

template <typename T>
class QUEUE_OUT
{
    public:
	virtual ~QUEUE_OUT(void){};
	virtual void PUSH(const T&) = 0;
	virtual void PUSH(T&&) = 0;
};

template <typename T>
class QUEUE: public QUEUE_IN<T>, public QUEUE_OUT<T>
{
    public:
	QUEUE(unsigned max_size = 0)	// 0 means unlimited
	{
	    max_size_ = max_size;
	}

	virtual ~QUEUE(void)
	{
	}

	void PUSH(const T &data)
	{
	    std::unique_lock<std::mutex> mlock(mutex_);
//	    printf("QUEUE[%p].PUSH: %lu items\n", this, queue_.size());
	    while (max_size_>0 && queue_.size()>=max_size_)
	    {
		cond_.wait(mlock);
	    }
	    queue_.push(data);
	    mlock.unlock();
	    cond_.notify_one();
	}

	void PUSH(T&& data)
	{
	    std::unique_lock<std::mutex> mlock(mutex_);
//	    printf("QUEUE[%p].PUSH: %lu items\n", this, queue_.size());
	    while (max_size_>0 && queue_.size()>=max_size_)
	    {
		cond_.wait(mlock);
	    }
	    queue_.push(std::move(data));
	    mlock.unlock();
	    cond_.notify_one();
	}

	void POP(T &data)
	{
	    std::unique_lock<std::mutex> mlock(mutex_);
//	    printf("QUEUE[%p].POP: %lu items\n", this, queue_.size());
	    while (queue_.empty())
	    {
		cond_.wait(mlock);
	    }
	    data = queue_.front();
	    queue_.pop();
	    mlock.unlock();
	    cond_.notify_one();
	}

    private:
	std::queue<T> queue_;
	unsigned max_size_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

#endif // QUEUE_H

/* ex: set softtabstop=4 shiftwidth=4: */
