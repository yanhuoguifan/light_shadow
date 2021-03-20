#include"thread.h"
#include<pthread.h>
#include"rwlock.h"
#include<stdlib.h>
#include<cassert>
#include<syscall.h>
#include<unordered_map>

namespace thread_space
{

	struct _thread_data
	{
	       void* arg;
	       int state;
               pid_t tpid;
	       pthread_t ppid;
               pthread_rwlock_t lock;
               pthread_attr_t attr;	       
	       pthread_barrier_t* barrier;
	       std::weak_ptr<thread_pool> pool;
	       unsigned char thread_type;

	       _thread_data(unsigned char _thread_type,std::shared_ptr<thread_pool>&_pool,pthread_attr_t*_attr):pool(_pool),
          thread_type(_thread_type)
 		{
			if(pthread_rwlock_init(&lock,NULL)!=0)
			{
				goto lock_bad;	
			}
			if((barrier=(pthread_barrier_t*)malloc(sizeof(pthread_barrier_t)))==NULL)
			{
				goto barrier_bad;
			}
			if(pthread_barrier_init(barrier,NULL,2)!=0)
			{
				goto barrier_bad;
			}
			if(_attr==NULL)
			{
				if(pthread_attr_init(&attr)!=0)
				{
					goto attr_bad;
				}
			}
			else
			{
				attr=*_attr;
			}
			arg=NULL;
			state=THREAD_SUCCEED;
			return;
attr_bad:
			::pthread_barrier_destroy(barrier);
barrier_bad:
			::free(barrier);
			::pthread_rwlock_destroy(&lock);
lock_bad:
			state=THREAD_INIT_FAIL;
 		}
	};
        
	_thread::_thread(unsigned char _thread_type,std::shared_ptr<thread_pool>_pool,pthread_attr_t*_attr)
	{ 
		try
		{
    			data=new _thread_data(_thread_type,_pool,_attr);
		}
		catch(...)
		{
			data=NULL;
		}
		if(data->state!=THREAD_SUCCEED)
		{
			data=NULL;
		}
	}

	int _thread::get_state(int&state)const
	{
               using namespace rwlock_space;
	       if(!data)
	       {
		       return THREAD_INIT_FAIL;
	       }
	       rwlock lock(data->lock,RWLOCK_READ_LOCK);
	       if(!lock)
	       {
		      return THREAD_LOCK_FAIL;
	       }
               state=data->state;
	       return THREAD_SUCCEED;
	}

	int _thread::get_attr(pthread_attr_t&attr )const
	{
              using namespace rwlock_space;
	      assert(data);
	      rwlock lock(data->lock,RWLOCK_READ_LOCK);
	      if(!lock)
	      {
		      return THREAD_LOCK_FAIL;
	      }
	      attr=data->attr;
	      return THREAD_SUCCEED;
	}

	int _thread::get_tpid(pid_t&tpid)const
	{
              using namespace rwlock_space;
	      assert(data);
	      rwlock lock(data->lock,RWLOCK_READ_LOCK);
	      if(!lock)
	      {
		      return THREAD_LOCK_FAIL;
	      }
	      tpid=data->tpid;
	      return THREAD_SUCCEED;
	}

	int _thread::get_ppid(pthread_t& ppid)const
	{
              using namespace rwlock_space;
	      assert(data);
	      rwlock lock(data->lock,RWLOCK_READ_LOCK);
	      if(!lock)
	      {
		      return THREAD_LOCK_FAIL;
	      }
	      ppid=data->ppid;
	      return THREAD_SUCCEED;
	}

	int _thread::get_arg(void*arg)const
	{
              using namespace rwlock_space;
	      assert(data);
	      rwlock lock(data->lock,RWLOCK_READ_LOCK);
	      if(!lock)
	      {
		      return THREAD_LOCK_FAIL;
	      }
	      arg=data->arg;
	      return THREAD_SUCCEED;
	}

	int _thread::get_thread_type(unsigned char& thread_type)const
	{
              using namespace rwlock_space;
	      assert(data);
	      rwlock lock(data->lock,RWLOCK_READ_LOCK);
	      if(!lock)
	      {
		      return THREAD_LOCK_FAIL;
	      }
	      thread_type=data->thread_type;
	      return THREAD_SUCCEED;
	}
        int _thread::set_arg(void*arg)
	{
              using namespace rwlock_space;
	      assert(data);
	      rwlock lock(data->lock,RWLOCK_WRITE_LOCK);
	      if(!lock)
	      {
		      return THREAD_LOCK_FAIL;
	      }
	      data->arg=arg;
	      return THREAD_SUCCEED;
	}

        int _thread::set_thread_pool(std::shared_ptr<thread_pool>&_pool)
	{
              using namespace rwlock_space;
	      assert(data);
	      rwlock lock(data->lock,RWLOCK_WRITE_LOCK);
	      if(!lock)
	      {
		      return THREAD_LOCK_FAIL;
	      }
	      data->pool=_pool;
	      return THREAD_SUCCEED;
	}
        int _thread::set_end()
	{
              using namespace rwlock_space;
	      assert(data);
	      rwlock lock(data->lock,RWLOCK_WRITE_LOCK);
	      if(!lock)
	      {
		      return THREAD_LOCK_FAIL;
	      }
	      data->state|=THREAD_END;
	      return THREAD_SUCCEED;
	}

        int _thread::set_ppid(pthread_t&ppid)
	{
              using namespace rwlock_space;
	      assert(data);
	      rwlock lock(data->lock,RWLOCK_WRITE_LOCK);
	      if(!lock)
	      {
		      return THREAD_LOCK_FAIL;
	      }
	      data->ppid=ppid;
	      return THREAD_SUCCEED;
	}

	int _thread::stop()
	{
		return THREAD_SUCCEED;
	}

	_thread::~_thread()
	{
		if(data)
		{
	                auto pool=data->pool.lock();
		        if(pool)
			{
				pool->remove(this);
			}	
			::pthread_attr_destroy(&(data->attr));
			if(data->barrier)
			{
				::pthread_barrier_destroy(data->barrier);
				::free(data->barrier);
			}
			::pthread_rwlock_destroy(&(data->lock));
		}
		free(data);
	}

	int _thread::set_tpid(pid_t&tpid)
	{
              using namespace rwlock_space;
	      assert(data);
	      rwlock lock(data->lock,RWLOCK_WRITE_LOCK);
	      if(!lock)
	      {
		      return THREAD_LOCK_FAIL;
	      }
	      data->tpid=tpid;
	      return THREAD_SUCCEED;
	}
        
	int _thread::start_wait(int thread_type)
	{
	        assert(data);
		int ret;
		ret=::pthread_barrier_wait(data->barrier);
		if(ret==0||ret==PTHREAD_BARRIER_SERIAL_THREAD)
		{
			if(thread_type==THREAD_MAIN)
			{
		                ret=pthread_barrier_destroy(data->barrier);
                                free(data->barrier);
		           	data->barrier=NULL;
                                return ret;
			}
			return 0;
		}
		return ret;
	}

	struct thread_pool_data
	{
		pthread_rwlock_t io_thread_lock;
		pthread_rwlock_t calculate_thread_lock;
		pthread_rwlock_t other_lock;
		std::unordered_map<pthread_t,pid_t>io_pthread_to_pid;
		std::unordered_map<pthread_t,pid_t>caculate_pthread_to_pid;
		std::unordered_map<pid_t,std::weak_ptr<_thread>>io_thread;
		std::unordered_map<pid_t,std::weak_ptr<_thread>>caculate_thread;
		unsigned int io_thread_num;
		unsigned int caculate_thread_num;
		unsigned int surplus_io_thread_num;
		unsigned int surplus_caculate_thread_num;
		int state;

		thread_pool_data(unsigned int _io_thread_num,unsigned int _calculate_thread_num):
			io_pthread_to_pid(_io_thread_num),
			caculate_pthread_to_pid(_calculate_thread_num),
			io_thread(_io_thread_num),
			caculate_thread(_calculate_thread_num),
			io_thread_num(_io_thread_num),
			caculate_thread_num(_calculate_thread_num),
			surplus_io_thread_num(_io_thread_num),
			surplus_caculate_thread_num(_calculate_thread_num)
		{
			if(pthread_rwlock_init(&io_thread_lock,NULL)!=0)
			{
				goto io_bad;
			}
			if(pthread_rwlock_init(&calculate_thread_lock,NULL)!=0)
			{
				goto calculate_bad;
			}
			if(pthread_rwlock_init(&other_lock,NULL)!=0)
			{
				goto other_bad;
			}
                        state=THREAD_POOL_SUCCEED;
	 	        return;
other_bad:
			::pthread_rwlock_destroy(&calculate_thread_lock);	
calculate_bad:
			::pthread_rwlock_destroy(&io_thread_lock);
io_bad:
			state=THREAD_POOL_INIT_FAIL;
		}
		~thread_pool_data()
		{
			if(state!=THREAD_POOL_INSERT_FAIL)
			{
				::pthread_rwlock_destroy(&io_thread_lock);
				::pthread_rwlock_destroy(&calculate_thread_lock);
				::pthread_rwlock_destroy(&other_lock);
			}
		}
	};

	thread_pool::thread_pool(unsigned int io_thread_num,unsigned int calculate_thread_num):data(new thread_pool_data(io_thread_num,calculate_thread_num))
	{
		if(data->state==THREAD_POOL_INIT_FAIL)
		{
			delete data;
			data=NULL;
		}
	}
        
	thread_pool::~thread_pool()
	{
		delete data;
	}

        int thread_pool::get_state(int&state)
	{
                if(data==NULL)
		{
			return THREAD_POOL_INIT_FAIL;
		}
		using namespace rwlock_space;
		rwlock lock(data->other_lock,RWLOCK_READ_LOCK);
		if(!lock)
		{
			return THREAD_POOL_LOCK_FAIL;
		}
		state=data->state;
		return THREAD_POOL_SUCCEED;
	}

	int thread_pool::get_thread(pthread_t ppid,std::weak_ptr<_thread>&thread_,unsigned char thread_type)
	{
		using namespace rwlock_space;
		pid_t tpid;
		if(thread_type&THREAD_POOL_IO_TYPE)
		{
			rwlock lock(data->io_thread_lock,RWLOCK_READ_LOCK);
			if(!lock)
			{
				return THREAD_POOL_LOCK_FAIL;
			}
			auto iterator=data->io_pthread_to_pid.find(ppid);
			if(iterator!=data->io_pthread_to_pid.end())
			{
				tpid=iterator->second;
			        lock.unlock();
				return get_thread(tpid,thread_,THREAD_POOL_IO_TYPE);
			}
		}
		if(thread_type&THREAD_POOL_CALCULATE_TYPE)
		{
			rwlock lock(data->calculate_thread_lock,RWLOCK_READ_LOCK);
			if(!lock)
			{
				return THREAD_POOL_LOCK_FAIL;
			}
			auto iterator=data->caculate_pthread_to_pid.find(ppid);
			if(iterator!=data->caculate_pthread_to_pid.end())
			{
				tpid=iterator->second;
			        lock.unlock();
				return get_thread(tpid,thread_,THREAD_POOL_CALCULATE_TYPE);
			}
		}
		return THREAD_POOL_GET_THREAD_NULL;
	}

	int thread_pool::get_thread(pid_t tpid,std::weak_ptr<_thread>&thread_,unsigned char thread_type)
	{
               using namespace rwlock_space;
	       if(thread_type&THREAD_POOL_IO_TYPE)
	       {
                  rwlock lock(data->io_thread_lock,RWLOCK_READ_LOCK);
		  if(!lock)
		  {
			  return THREAD_POOL_LOCK_FAIL;
		  }
		  auto iterator=data->io_thread.find(tpid);
		  if(iterator!=data->io_thread.end())
		  {
                       thread_=iterator->second;
		       return THREAD_POOL_SUCCEED;
		  }
	       }
	       if(thread_type&THREAD_POOL_CALCULATE_TYPE)
	       {
                  rwlock lock(data->calculate_thread_lock,RWLOCK_READ_LOCK);
		  if(!lock)
		  {
			  return THREAD_POOL_LOCK_FAIL;
		  }
		  auto iterator=data->caculate_thread.find(tpid);
		  if(iterator!=data->caculate_thread.end())
		  {
                       thread_=iterator->second;
		       return THREAD_POOL_SUCCEED;
		  }
	       }
	       return THREAD_POOL_GET_THREAD_NULL;
	}
	
	int thread_pool::set_thread(std::shared_ptr<_thread>&thread_)
	{
		using namespace rwlock_space;
		unsigned char thread_type;
		if(thread_->get_thread_type(thread_type)!=THREAD_SUCCEED)
		{
			return THREAD_POOL_LOCK_FAIL;
		}
		if(thread_type&THREAD_POOL_IO_TYPE)
		{
			{
				rwlock lock(data->other_lock,RWLOCK_READ_LOCK);
                                if(!lock)
				{
					return THREAD_POOL_LOCK_FAIL;
				}
				if(data->surplus_io_thread_num<=0)
				{
					if(thread_type&THREAD_POOL_CALCULATE_TYPE)
					{
       						goto calculate;
					}
					return THREAD_POOL_NO_SURPLUS_NUM;
				}
			}
			pid_t tpid;
			if(thread_->get_tpid(tpid)!=THREAD_SUCCEED)
			{
				return THREAD_POOL_LOCK_FAIL;
			}
			pthread_t ppid;
			if(thread_->get_ppid(ppid)!=THREAD_SUCCEED)
			{
				return THREAD_POOL_LOCK_FAIL;
			}
                        rwlock lock(data->io_thread_lock,RWLOCK_WRITE_LOCK);
	                {
				auto pr=data->io_thread.insert(std::make_pair(tpid,thread_));
				if(!pr.second)
				{
					return THREAD_POOL_INSERT_FAIL;
				}
                        }
		        auto pr=data->io_pthread_to_pid.insert(std::make_pair(ppid,tpid));
                        if(!pr.second)
			{
				assert("thread_pool atypism\n");
				data->io_thread.erase(tpid);
				return THREAD_POOL_INSERT_FAIL;
			}
			rwlock numlock(data->other_lock,RWLOCK_WRITE_LOCK);
			if(!numlock)
			{
				data->io_thread.erase(tpid);
                                data->io_pthread_to_pid.erase(ppid);
				return THREAD_POOL_LOCK_FAIL;
			}
			--(data->surplus_io_thread_num);
			return THREAD_POOL_SUCCEED;
		}
		else if(thread_type==THREAD_POOL_CALCULATE_TYPE)
		{
calculate:
			{
				rwlock lock(data->other_lock,RWLOCK_READ_LOCK);
                                if(!lock)
				{
					return THREAD_POOL_LOCK_FAIL;
				}
				if(data->surplus_caculate_thread_num<=0)
				{
					return THREAD_POOL_NO_SURPLUS_NUM;
				}
			}
			pid_t tpid;
			if(thread_->get_tpid(tpid)!=THREAD_SUCCEED)
			{
				return THREAD_POOL_LOCK_FAIL;
			}
			pthread_t ppid;
			if(thread_->get_ppid(ppid)!=THREAD_SUCCEED)
			{
				return THREAD_POOL_LOCK_FAIL;
			}
                        rwlock lock(data->calculate_thread_lock,RWLOCK_WRITE_LOCK);
	                {
				auto pr=data->caculate_thread.insert(std::make_pair(tpid,thread_));
				if(!pr.second)
				{
					return THREAD_POOL_INSERT_FAIL;
				}
                        }
		        auto pr=data->caculate_pthread_to_pid.insert(std::make_pair(ppid,tpid));
                        if(!pr.second)
			{
				assert("thread_pool atypism\n");
				data->caculate_thread.erase(tpid);
				return THREAD_POOL_INSERT_FAIL;
			}
			rwlock numlock(data->other_lock,RWLOCK_WRITE_LOCK);
			if(!numlock)
			{
				data->caculate_thread.erase(tpid);
                                data->caculate_pthread_to_pid.erase(ppid);
				return THREAD_POOL_LOCK_FAIL;
			}
			--(data->surplus_caculate_thread_num);
			return THREAD_POOL_SUCCEED;
		}
		return THREAD_POOL_BAD_TYPE;
	}
        int thread_pool::remove(_thread*thread_)
	{
		using namespace rwlock_space;
		unsigned char thread_type;
		if(thread_->get_thread_type(thread_type)!=THREAD_SUCCEED)
		{
			return THREAD_POOL_LOCK_FAIL;
		}
	        pthread_t ppid;
		if(thread_->get_ppid(ppid)!=THREAD_SUCCEED)
		{
			return THREAD_POOL_LOCK_FAIL;
		}
		pid_t tpid;
		if(thread_->get_tpid(tpid)!=THREAD_SUCCEED)
		{
			return THREAD_POOL_LOCK_FAIL;
		}
		if(thread_type&THREAD_POOL_IO_TYPE)
		{
			rwlock iolock(data->io_thread_lock,RWLOCK_WRITE_LOCK);
			if(!iolock)
			{
				return THREAD_POOL_LOCK_FAIL;
			}
			if(!data->io_thread.erase(tpid))
			{
				if(thread_type&THREAD_POOL_CALCULATE_TYPE)
				{
					goto calculate;
				}
				else
				{
					return THREAD_POOL_GET_THREAD_NULL;
				}
			}
			if(!data->io_pthread_to_pid.erase(ppid))
			{
				assert("thread_pool atypism\n");
			}
			else
			{
				rwlock lock(data->other_lock,RWLOCK_WRITE_LOCK);
				if(!lock)
				{
					return THREAD_POOL_LOCK_FAIL;
				}
				++data->surplus_io_thread_num;
				assert(data->surplus_io_thread_num<=data->io_thread_num);
			}
		}
		else if(thread_type&THREAD_POOL_CALCULATE_TYPE)
		{
calculate:
			rwlock calculatelock(data->calculate_thread_lock,RWLOCK_WRITE_LOCK);
			if(!calculatelock)
			{
				return THREAD_POOL_LOCK_FAIL;
			}
			if(!data->caculate_thread.erase(tpid))
			{
				return THREAD_POOL_GET_THREAD_NULL;
			}
			if(!data->caculate_pthread_to_pid.erase(ppid))
			{
				assert("thread_pool atypism\n");
			}
			else
			{
				rwlock lock(data->other_lock,RWLOCK_WRITE_LOCK);
				if(!lock)
				{
					return THREAD_POOL_LOCK_FAIL;
				}
				++data->surplus_caculate_thread_num;
				assert(data->surplus_caculate_thread_num<=data->caculate_thread_num);
			}
		}
		else
		{
			return THREAD_POOL_BAD_TYPE;
		}
		return THREAD_POOL_SUCCEED;
	}
	int thread_pool::get_calculate_thread_num(unsigned int&num)const
	{
             using namespace rwlock_space;
	     rwlock lock(data->other_lock,RWLOCK_READ_LOCK);
	     if(!lock)
	     {
		     return THREAD_POOL_LOCK_FAIL;
	     }
             num=data->caculate_thread_num;
	     return THREAD_POOL_SUCCEED;
	}

	int thread_pool::get_surplus_calculate_thread_num(unsigned int&num)const
	{
             using namespace rwlock_space;
	     rwlock lock(data->other_lock,RWLOCK_READ_LOCK);
	     if(!lock)
	     {
		     return THREAD_POOL_LOCK_FAIL;
	     }
             num=data->surplus_caculate_thread_num;
	     return THREAD_POOL_SUCCEED;
	}

	int thread_pool::get_io_thread_num(unsigned int&num)const
	{
             using namespace rwlock_space;
	     rwlock lock(data->other_lock,RWLOCK_READ_LOCK);
	     if(!lock)
	     {
		     return THREAD_POOL_LOCK_FAIL;
	     }
             num=data->io_thread_num;
	     return THREAD_POOL_SUCCEED;
	}

	int thread_pool::get_surplus_io_thread_num(unsigned int&num)const
	{
             using namespace rwlock_space;
	     rwlock lock(data->other_lock,RWLOCK_READ_LOCK);
	     if(!lock)
	     {
		     return THREAD_POOL_LOCK_FAIL;
	     }
             num=data->surplus_io_thread_num;
	     return THREAD_POOL_SUCCEED;
	}
}
