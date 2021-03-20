#ifndef THREAD
#define THREAD

#include<pthread.h>
#include<unistd.h>
#include<memory>
#include<stdlib.h>
#include<cassert>
#include<syscall.h>
#include"rwlock.h"

#define gettid() syscall(__NR_gettid)

namespace thread_space
{
	const int THREAD_SUCCEED=0;

	const int THREAD_INIT_FAIL=-1;

	const int THREAD_LOCK_FAIL=3;

        const int THREAD_START=1;

	const int THREAD_STOP=2;

        const int THREAD_END=4;

	const int THREAD_MAIN=0;

	const int THREAD_SECONDARY=1;

	const unsigned char  THREAD_IO_TYPE=1;
	
	const unsigned char  THREAD_CALCULATE_TYPE=2;

	const unsigned char  THREAD_ALL_TYPE=THREAD_IO_TYPE|THREAD_CALCULATE_TYPE;
	
	const int THREAD_POOL_SUCCEED=0;

	const int THREAD_POOL_INIT_FAIL=-1;

	const int THREAD_POOL_GET_THREAD_NULL=-2;

	const int THREAD_POOL_NO_SURPLUS_NUM=-3;

	const int THREAD_POOL_LOCK_FAIL=-4;

	const int THREAD_POOL_INSERT_FAIL=-5;

	const int THREAD_REMOVE_THREAD_NULL=-6;

	const unsigned char THREAD_POOL_CALCULATE_TYPE=THREAD_CALCULATE_TYPE;

	const unsigned char THREAD_POOL_IO_TYPE=THREAD_IO_TYPE;

	const unsigned char THREAD_POOL_ALL_TYPE=THREAD_ALL_TYPE;

	const int THREAD_POOL_BAD_TYPE=4;

	template<typename THREAD_FUN>
		void* thread_main(void*);

	struct _thread_data;

	class thread_pool;

      	class _thread:public std::enable_shared_from_this<_thread>
	{
		private:
			_thread_data*data;
		public:
			_thread(unsigned char thread_type=THREAD_ALL_TYPE,std::shared_ptr<thread_pool>pool=NULL,pthread_attr_t*attr=NULL);
      
			int get_state(int&state)const;
		
			int get_attr(pthread_attr_t&attr )const;

			int get_tpid(pid_t&tpid)const;

			int get_ppid(pthread_t& ppid)const;

			int get_arg(void*arg)const;

			int get_thread_type(unsigned char&thread_type)const;

			int set_arg(void*arg);

			int set_ppid(pthread_t&ppid);

			int set_thread_pool(std::shared_ptr<thread_pool>&);

			int set_tpid(pid_t&tpid);

			int set_end();
        
			int start_wait(int thread_type);

			virtual int stop();

			virtual ~_thread();
		
			virtual int start()=0;
	};
        
	template<typename THREAD_FUN>
	class thread:public _thread
	{
		private:
			THREAD_FUN fun;
		public:
			thread(THREAD_FUN  _fun,unsigned char thread_type=THREAD_ALL_TYPE,std::shared_ptr<thread_pool> pool=NULL,pthread_attr_t*attr=NULL):_thread(thread_type,pool,attr),fun(_fun)
		        {
		        }
	
			int start()
			{
	    			using namespace rwlock_space;
                                #ifndef NDEBUG
	    			int state=THREAD_SUCCEED;
                                #endif
	    			assert(get_state(state)!=THREAD_LOCK_FAIL);
	    			assert(state!=THREAD_INIT_FAIL);
	    			pthread_attr_t attr;
	    			if(get_attr(attr)!=THREAD_SUCCEED)
	    			{
		  			return THREAD_LOCK_FAIL;
	    			}
	    			pthread_t ppid;
	    			int ret= pthread_create(&ppid,&attr,thread_main<THREAD_FUN>,this);
	    			set_ppid(ppid);
	    			return ret;
			}
        
			friend void* thread_main<THREAD_FUN>(void*);
	};
	template<typename THREAD_FUN>
		void* thread_main(void*thread_this)
		{
			using namespace std;
			thread<THREAD_FUN> *thread_ptr_=(thread<THREAD_FUN>*)thread_this;
		        shared_ptr<thread<THREAD_FUN>>thread_ptr=dynamic_pointer_cast<thread<THREAD_FUN>>(thread_ptr_->shared_from_this());
                        pid_t tpid=::gettid();
			thread_ptr->set_tpid(tpid);
			thread_ptr->start_wait(THREAD_SECONDARY);
			void*ret;
			try
			{
				ret= thread_ptr->fun(THREAD_START,thread_ptr->shared_from_this());
			}
			catch(...)
			{
                                ret=NULL;
			}
			while(thread_ptr_->set_end()!=THREAD_SUCCEED)
			{
			}
			return ret;
		}

	struct thread_pool_data;

	class thread_pool:public std::enable_shared_from_this<thread_pool>
	{
		private:
			thread_pool_data*data;
		public:
			thread_pool(unsigned int  io_thread_num,unsigned int calculate_thread_num);
			~thread_pool();
			int set_thread(std::shared_ptr<_thread>&thread_);
			int get_thread(pthread_t,std::weak_ptr<_thread>&thread_,unsigned char thread_type);
			int get_thread(pid_t,std::weak_ptr<_thread>&thread_,unsigned char thread_type);
	                int remove(_thread*);
		       	int get_io_thread_num(unsigned int &)const;
	                int get_calculate_thread_num(unsigned int &)const;
	                int get_surplus_io_thread_num(unsigned int &)const;
	                int get_surplus_calculate_thread_num(unsigned int &)const;
                        int get_state(int &state);
	};
}

#endif
