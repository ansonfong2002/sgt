// THREAD.h: (10/18/23, RD)

// references:
// - SpecC
// - SystemC
// - https://en.cppreference.com/w/cpp/thread/thread
// - https://livebook.manning.com/book/c-plus-plus-concurrency-in-action/chapter-2/77

#ifndef THREAD_H
#define THREAD_H

#include "MODULE.h"

#include <list>
#include <thread>

class THREAD
{
    public:
	THREAD(void)
	{
//	    printf("THREAD::THREAD: %u concurrent threads supported.\n",
//		    std::thread::hardware_concurrency());
	}

	~THREAD(void)
	{
	}

	static void CREATE(MODULE *m)
	{
	    joinable_threads_.push_back(m);
	}

	static void DETACH(MODULE *m)
	{
	    detached_threads_.push_back(m);
	}

	static void START_ALL(void)
	{
	    for(std::list<MODULE*>::iterator m=detached_threads_.begin();
		    m!=detached_threads_.end(); m++)
	    {
		std::thread(&MODULE::main, *m).detach();
	    }
	    for(std::list<MODULE*>::iterator m=joinable_threads_.begin();
		    m!=joinable_threads_.end(); m++)
	    {
		// ensure that thread object is not destroyed before thread is joined
//		threads_.push_back(std::move(std::thread(&MODULE::main, *m)));
//		threads_.push_back(std::thread(&MODULE::main, *m));
		threads_.emplace_back(std::thread(&MODULE::main, *m));
	    }
	}

	static void JOIN_ALL(void)
	{
	    for(std::list<std::thread>::iterator t=threads_.begin(); t!=threads_.end(); t++)
	    {
		t->join();
	    }
	}

	static void QUICK_EXIT(int status)
	{
	    std::quick_exit(status);	// skip static destructors which may create havoc with detached threads
	}

    private:
	static std::list<MODULE*> joinable_threads_;
	static std::list<MODULE*> detached_threads_;
	static std::list<std::thread> threads_;
};

std::list<MODULE*> THREAD::joinable_threads_;
std::list<MODULE*> THREAD::detached_threads_;
std::list<std::thread> THREAD::threads_;

#endif // THREAD_H

/* ex: set softtabstop=4 shiftwidth=4: */
