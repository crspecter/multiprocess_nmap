#ifndef __THREAD_POOL__
#define __THREAD_POOL__


#include <list>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>



struct tasks {
	void* (*do_it)(void *arg);
	void *arg;
};


class thread_pool
{
public:
typedef  thread_pool* ptr_type;
typedef  void* (*task)(void*arg);

public:	

	thread_pool(uint16_t cout=10):thread_count_(cout), task_container_(0){}
	
	void create_thread_pool();
	
	void thread_pool_add_task(task do_it, void *arg);

	void thread_pool_destroy();

public:	
	
	uint16_t 		thread_count_;
	std::list<tasks> task_container_;
	pthread_t*		threads_;
	pthread_mutex_t mutex_lock_;
	pthread_cond_t 	cond_;
};


#endif
