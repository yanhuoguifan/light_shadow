#ifndef MUTEX_LOCK
#define MUTEX_LOCK

#include<pthread.h>

namespace mutex_lock_space
{
	const int MUTEX_LOCK_SUCCEED=0;

	const int MUTEX_LOCKED=1;
	
	const int MUTEX_LOCK_FAIL=2;	
	
	const int MUTEX_LOCK_UNLOCK=4;

	const int MUTEX_LOCK_TRY=8;

	const int MUTEX_LOCK_NOT_TRY=16;

	class mutex_lock//考虑到锁可能是性能瓶颈，所以没有进行接口与数据分离
	{
		private:
			pthread_mutex_t &lock;
			int state;
		public:
			inline mutex_lock(pthread_mutex_t&,unsigned char lock_type=MUTEX_LOCK_NOT_TRY);//构造函数直接锁，默认写锁
			inline int get_state()const;//
		        inline int unlock();//主动解锁
			inline int locked(int lock_type=MUTEX_LOCK_NOT_TRY);
			inline operator bool()const;//返回与fail&后和succeed是否相等
			inline ~mutex_lock();//释放锁
	};

	mutex_lock::mutex_lock(pthread_mutex_t &_lock,unsigned char lock_type):lock(_lock)
	{
		 if(lock_type&MUTEX_LOCK_TRY)
		 {
			 if(pthread_mutex_trylock(&lock)==0)
			 {
				 state=MUTEX_LOCKED|MUTEX_LOCK_SUCCEED;
			 }
			 else
			 {
				 state=MUTEX_LOCK_FAIL|MUTEX_LOCK_UNLOCK;
			 }
		 }
		 else
		 {
			 if(pthread_mutex_lock(&lock)==0)
			 {
				 state=MUTEX_LOCKED|MUTEX_LOCK_SUCCEED;
			 }
			 else
			 {
				 state=MUTEX_LOCK_FAIL|MUTEX_LOCK_UNLOCK;
			 }
		 }
	}

	int mutex_lock::get_state()const
	{
            return state;
	}
        int mutex_lock::locked(int lock_type)
	{
		    if(state&MUTEX_LOCKED)
		    {
			    return MUTEX_LOCK_SUCCEED;
		    }
   		    if(lock_type&MUTEX_LOCK_TRY)
   		    {   
			    if(pthread_mutex_trylock(&lock)==0)
   			    {
   				    state=MUTEX_LOCKED|MUTEX_LOCK_SUCCEED;
				    return MUTEX_LOCK_SUCCEED;
   			    }
   			    else
   			    {
   				    state=MUTEX_LOCK_FAIL|MUTEX_LOCK_UNLOCK;
   			    }
   		    }
   		    else
   		    {
   			    if(pthread_mutex_lock(&lock)==0)
   			    {
   				    state=MUTEX_LOCKED|MUTEX_LOCK_SUCCEED;
				    return MUTEX_LOCK_SUCCEED;
	   		    }
   			    else
   			    {
   				    state=MUTEX_LOCK_FAIL|MUTEX_LOCK_UNLOCK;
   			    }
   		    }
   		    return MUTEX_LOCK_FAIL;
	}
        int mutex_lock::unlock()
	{
	    if(state&MUTEX_LOCKED)
	    {
		    if(pthread_mutex_unlock(&lock)==0)
		    {
                            state=MUTEX_LOCK_UNLOCK|MUTEX_LOCK_SUCCEED;
			    return MUTEX_LOCK_SUCCEED;
		    }
		    state&=MUTEX_LOCK_FAIL;
	    }
            else if((state&MUTEX_LOCK_UNLOCK)==MUTEX_LOCK_UNLOCK)
	    {
                    state=MUTEX_LOCK_UNLOCK|MUTEX_LOCK_SUCCEED;
		    return MUTEX_LOCK_SUCCEED;
	    }
	    return MUTEX_LOCK_FAIL;
	}
        
	mutex_lock::~mutex_lock()
	{
	    if(state&MUTEX_LOCKED)
	    {
		    pthread_mutex_unlock(&lock);
            }
	}

	mutex_lock::operator bool()const
	{
		return state&MUTEX_LOCKED;
	}
}

#endif
