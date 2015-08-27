#include "thread_pool.h"
#include <unistd.h>


static void cleanup(pthread_mutex_t* lock) {
	pthread_mutex_unlock(lock);
}


static void* work(thread_pool::ptr_type pool)
{
	struct tasks t;
	for(;;)
	{
		pthread_mutex_lock(&pool->mutex_lock_);
		pthread_cleanup_push((void(*)(void*))cleanup, &pool->mutex_lock_);
		while( pool->task_container_.empty())
		{
			pthread_cond_wait(&pool->cond_, &pool->mutex_lock_);
		}
		t = pool->task_container_.front();
		pool->task_container_.pop_front();
		pthread_cleanup_pop(0);
		pthread_mutex_unlock(&pool->mutex_lock_);
		t.do_it(t.arg);
		usleep(100);
	}
	return NULL;
}


void thread_pool::create_thread_pool()
{

	pthread_mutex_init(&mutex_lock_, NULL);
	pthread_cond_init(&cond_, NULL);
	threads_ = new pthread_t[thread_count_];
	for (uint16_t i = 0; i < thread_count_; i ++)
	{
		pthread_create(threads_ + i, NULL, (void*(*)(void*))work, this);
	}	
}

	
void thread_pool::thread_pool_add_task(task do_it, void *arg)
{
	pthread_mutex_lock(&mutex_lock_);
	struct tasks tk;
	tk.do_it = do_it;
	tk.arg = arg;
	task_container_.push_back(tk);
	pthread_cond_signal(&cond_);
	pthread_mutex_unlock(&mutex_lock_);	
}


void thread_pool::thread_pool_destroy()
{
	uint16_t i;
	for (i = 0; i < thread_count_; i++)
	{
		pthread_cancel(threads_[i]);
	}
	for (i = 0; i < thread_count_; i++)
	{
		pthread_join(threads_[i], NULL);
	}	
	pthread_mutex_destroy(&mutex_lock_);
	pthread_cond_destroy(&cond_);
	delete [] threads_;
}
