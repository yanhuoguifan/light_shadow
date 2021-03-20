#ifndef RWLOCK
#define RWLOCK

#include<pthread.h>

namespace rwlock_space
{
	const int RWLOCK_SUCCEED=0;

	const int RWLOCK_READ_LOCK=1;
	
	const int RWLOCK_WRITE_LOCK=2;
		
	const int RWLOCK_FAIL=4;	
	
	const int RWLOCK_UNLOCK=8;

	const int RWLOCK_TRY=16;

	const int RWLOCK_NOT_TRY=32;

	class rwlock//考虑到锁可能是性能瓶颈，所以没有进行接口与数据分离
	{
		private:
			pthread_rwlock_t &lock;
			int state;
		public:
			inline rwlock(pthread_rwlock_t&,unsigned char lock_type=RWLOCK_WRITE_LOCK|RWLOCK_NOT_TRY);//构造函数直接锁，默认写锁
			inline int get_state()const;//
		        inline int unlock();//主动解锁
			inline int locked(int lock_type);
			inline operator bool()const;//返回与fail&后和succeed是否相等
			inline ~rwlock();//释放锁
	};

	rwlock::rwlock(pthread_rwlock_t &_lock,unsigned char lock_type):lock(_lock)
	{
            if(lock_type&RWLOCK_WRITE_LOCK)
	    {
		 if(lock_type&RWLOCK_TRY)
		 {
			 if(pthread_rwlock_trywrlock(&lock)==0)
			 {
				 state=RWLOCK_WRITE_LOCK|RWLOCK_SUCCEED;
			 }
			 else
			 {
				 state=RWLOCK_FAIL|RWLOCK_UNLOCK;
			 }
		 }
		 else
		 {
			 if(pthread_rwlock_wrlock(&lock)==0)
			 {
				 state=RWLOCK_WRITE_LOCK|RWLOCK_SUCCEED;
			 }
			 else
			 {
				 state=RWLOCK_FAIL|RWLOCK_UNLOCK;
			 }
		 }
	    }
	    else
	    {
		 if(lock_type&RWLOCK_TRY)
		 {
			 if(pthread_rwlock_tryrdlock(&lock)==0)
			 {
				 state=RWLOCK_READ_LOCK|RWLOCK_SUCCEED;
			 }
			 else
			 {
				 state=RWLOCK_FAIL|RWLOCK_UNLOCK;
			 }
		 }
		 else
		 {
			 if(pthread_rwlock_rdlock(&lock)==0)
			 {
				 state=RWLOCK_READ_LOCK|RWLOCK_SUCCEED;
			 }
			 else
			 {
				 state=RWLOCK_FAIL|RWLOCK_UNLOCK;
			 }
		 }
	    }
	}

	int rwlock::get_state()const
	{
            return state;
	}
        int rwlock::locked(int lock_type)
	{
	    if(lock_type&RWLOCK_READ_LOCK)
	    {
		    if(state&RWLOCK_READ_LOCK)
		    {
			    return RWLOCK_SUCCEED;
		    }
		    else
		    {
	   		    if(lock_type&RWLOCK_NOT_TRY)
	   		    {
	   			    if(pthread_rwlock_rdlock(&lock)==0)
	   			    {
	   				    state=RWLOCK_READ_LOCK|RWLOCK_SUCCEED;
					    return RWLOCK_SUCCEED;
	   			    }
	   			    else
	   			    {
	   				    state=RWLOCK_FAIL|RWLOCK_UNLOCK;
	   			    }
	   		    }
	   		    else
	   		    {
	   			    if(pthread_rwlock_tryrdlock(&lock)==0)
	   			    {
	   				    state=RWLOCK_READ_LOCK|RWLOCK_SUCCEED;
					    return RWLOCK_SUCCEED;
	   			    }
	   			    else
	   			    {
	   				    state=RWLOCK_FAIL|RWLOCK_UNLOCK;
	   			    }
	   		    }
		    }
	    }
	    else
	    {
		    if(state&RWLOCK_WRITE_LOCK)
		    {
			    return RWLOCK_SUCCEED;
		    }
		    else
		    {
	   		    if(lock_type&RWLOCK_NOT_TRY)
	   		    {
	   			    if(pthread_rwlock_wrlock(&lock)==0)
	   			    {
	   				    state=RWLOCK_WRITE_LOCK|RWLOCK_SUCCEED;
					    return RWLOCK_SUCCEED;
	   			    }
	   			    else
	   			    {
	   				    state=RWLOCK_FAIL|RWLOCK_UNLOCK;
	   			    }
	   		    }
	   		    else
	   		    {
	   			    if(pthread_rwlock_trywrlock(&lock)==0)
	   			    {
	   				    state=RWLOCK_WRITE_LOCK|RWLOCK_SUCCEED;
					    return RWLOCK_SUCCEED;
	   			    }
	   			    else
	   			    {
	   				    state=RWLOCK_FAIL|RWLOCK_UNLOCK;
	   			    }
	   		    }
		    }
	    }
            return RWLOCK_FAIL;
	}
        int rwlock::unlock()
	{
	    if(state&(RWLOCK_READ_LOCK|RWLOCK_WRITE_LOCK))
	    {
		    if(pthread_rwlock_unlock(&lock)==0)
		    {
                            state=RWLOCK_UNLOCK|RWLOCK_SUCCEED;
			    return RWLOCK_SUCCEED;
		    }
		    state&=RWLOCK_FAIL;
	    }
            else if((state&RWLOCK_UNLOCK)==RWLOCK_UNLOCK)
	    {
                    state=RWLOCK_UNLOCK|RWLOCK_SUCCEED;
		    return RWLOCK_SUCCEED;
	    }
	    return RWLOCK_FAIL;
	}
        
	rwlock::~rwlock()
	{
	    if(state&(RWLOCK_READ_LOCK|RWLOCK_WRITE_LOCK))
	    {
		    pthread_rwlock_unlock(&lock);
            }
	}

	rwlock::operator bool()const
	{
		return state&(RWLOCK_READ_LOCK|RWLOCK_WRITE_LOCK);
	}
}

#endif
