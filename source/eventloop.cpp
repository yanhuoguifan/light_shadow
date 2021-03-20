#include<unordered_map>
#include<queue>
#include<sys/epoll.h>
#include<sys/eventfd.h>
#include"rwlock.h"
#include"mutex_lock.h"
#include"fd.h"
#include"eventloop.h"

using namespace std;
using namespace std::placeholders;
using namespace rwlock_space;
using namespace fd_space;
using namespace mutex_lock_space;

namespace eventloop_space
{
	struct epoll_fd:public _fd
	{
		int size;
		int remainder_size;
		int evaluate_wait_events;
		int max_wait_listen;
		int events_num;
		int wait_time;
		epoll_event* events;
		
		epoll_fd(int _size=1024,int _max_wait_listen=1024):
			size(_size),
			remainder_size(_size),
			evaluate_wait_events(3),
			max_wait_listen(_max_wait_listen),
			events_num(8),
			wait_time(-1)
		{
			int fd=::epoll_create(size);
			set_fd(fd);
			events=(epoll_event*)malloc(sizeof(epoll_event)*8);
		}

		int wait()
		{
			if(evaluate_wait_events+2>=events_num&&events_num*2<max_wait_listen)
			{
				epoll_event* biger_events=(epoll_event*)malloc(sizeof(epoll_event)*events_num*2);
				if(biger_events)
				{
					free(events);
					events=biger_events;
					events_num<<=1;
				}
			}
			else if(evaluate_wait_events*4<=events_num)
			{
				if(events_num>=8)
				{
					events_num/=2;
					events=(epoll_event*)realloc(events,events_num);
					assert(events);
				}
			}
			int fd,events_action_num;
			get_fd(fd);
			events_action_num= epoll_wait(fd,events,events_num,wait_time);
                        evaluate_wait_events=(evaluate_wait_events*2+events_action_num)/3;
			return events_action_num;
		}

	};
        
	struct insert_func_s
	{
		function<void(void*)>func;
		void*arg;

		insert_func_s(function<void(void*)>_func=NULL,void*_arg=NULL):
			func(_func),
			arg(_arg)
		{
		}
	};

	struct insert_func_fd:public _fd
	{
                queue<insert_func_s> que;
                pthread_mutex_t lock;
		
		insert_func_fd()
		{
			if(::pthread_mutex_init(&lock,NULL)!=0)
			{
				return;
			}
			int fd=::eventfd(0,EFD_NONBLOCK);
			set_fd(fd);
		}

		~insert_func_fd()
		{
			int fd;
			get_fd(fd);
			if(fd!=-1)
			{
				::pthread_mutex_destroy(&lock);
			}
		}
                
	        int insert_func(function<void(void*)>&func,void*arg)
		{
			insert_func_s fun(func,arg);
			mutex_lock _lock(lock);
			if(!_lock)
			{
				return EVENTLOOP_LOCK_FAIL;
			}
			que.push(fun);
			_lock.unlock();
			int fd;
			uint64_t awake=1;
			get_fd(fd);
			::write(fd,&awake,8);
			return EVENTLOOP_SUCCEED;
		}

		int do_func(shared_ptr<fd_callback>)
		{
			int fd;
			get_fd(fd);
			uint64_t get_awake;
			::read(fd,&get_awake,8);
			insert_func_s func;
			mutex_lock _lock(lock);
			if(!_lock)
			{
				return EVENTLOOP_NOT_FINISH;
			}
			for(int i=0;i<5&&que.size();++i)
			{
				func=que.front();
				que.pop();
				_lock.unlock();
				func.func(func.arg);
				_lock.locked();
			}
		        if(que.size())
			{
				return EVENTLOOP_NOT_FINISH;
			}
			return EVENTLOOP_FINISH;
		}

	};

	struct eventloop_data
	{
		pthread_rwlock_t lock;
		pthread_cond_t wait_cond;
		pthread_mutex_t wait_mutex_lock;
		int wait;
		int state;
		unordered_map<int,shared_ptr<fd_callback>>all_callback_map,loop_callback_map[2];
		epoll_fd loop_fd;
                insert_func_fd insert_fd;

		eventloop_data(int size,int max_wait_listen):wait(EVENTLOOP_CONNTINUE),loop_fd(size,max_wait_listen)
		{
			if(!loop_fd.events)
			{
				goto loop_fd_bad;
			}
			int fd;
			loop_fd.get_fd(fd);
			if(fd<0)
			{
				goto fd_bad;
			}
			if(::pthread_rwlock_init(&lock,NULL)!=0)
			{
				goto lock_bad;
			}
			if(::pthread_mutex_init(&wait_mutex_lock,NULL)!=0)
			{
				goto wait_mutex_lock_bad;
			}
			if(::pthread_cond_init(&wait_cond,NULL)!=0)
			{
				goto wait_cond_bad;
			}
			insert_fd.get_fd(fd);
			if(fd<0)
			{
				goto insert_fd_bad;
			}
			state=EVENTLOOP_SUCCEED;
			return;
insert_fd_bad:
			::pthread_cond_destroy(&wait_cond);
wait_cond_bad:
			::pthread_mutex_destroy(&wait_mutex_lock);
wait_mutex_lock_bad:
			::pthread_rwlock_destroy(&lock);
lock_bad:
fd_bad:
loop_fd_bad:
			state=EVENTLOOP_INIT_FAIL;
		}

		~eventloop_data()
		{
			if(state==EVENTLOOP_SUCCEED)
			{
				::pthread_rwlock_destroy(&lock);
				::pthread_mutex_destroy(&wait_mutex_lock);
				::pthread_cond_destroy(&wait_cond);
				free(loop_fd.events);
			}
		}
	};

        eventloop::eventloop(int size,int max_wait_listen):data(new eventloop_data(size,max_wait_listen))
	{
			int fd;
			data->insert_fd.get_fd(fd);
			shared_ptr<fd_callback> insert_callback;
			try
			{
			        insert_callback=shared_ptr<fd_callback>(new fd_callback(fd,EPOLLIN|EPOLLET,bind(&insert_func_fd::do_func,&(data->insert_fd),_1)));	
			}
			catch(...)
			{
				goto init_bad;
			}
			if(add_fd(insert_callback)!=EVENTLOOP_SUCCEED)
			{
				goto init_bad;
			}
		if(data->state==EVENTLOOP_INIT_FAIL)
		{
init_bad:
			delete data;
			data=NULL;
		}
	}

	int eventloop::loop_wait()
	{
		assert(data);
		mutex_lock lock(data->wait_mutex_lock,RWLOCK_NOT_TRY);
		if(!lock)
		{
			return EVENTLOOP_LOCK_FAIL;
		}
		data->wait=EVENTLOOP_STOP;
		return EVENTLOOP_SUCCEED;
	}

	int eventloop::loop_continue()
	{
		assert(data);
		mutex_lock lock(data->wait_mutex_lock,RWLOCK_NOT_TRY);
		if(!lock)
		{
			return EVENTLOOP_LOCK_FAIL;
		}
		data->wait=EVENTLOOP_CONNTINUE;
		lock.unlock();
		if(::pthread_cond_signal(&data->wait_cond)!=0)
		{
			return EVENTLOOP_FAIL;
		}
		return EVENTLOOP_SUCCEED;
	}

	int eventloop::loop_end()
	{
		assert(data);
		mutex_lock lock(data->wait_mutex_lock,RWLOCK_NOT_TRY);
		if(!lock)
		{
			return EVENTLOOP_LOCK_FAIL;
		}
		data->wait=EVENTLOOP_END;
		lock.unlock();
		if(::pthread_cond_signal(&data->wait_cond)!=0)
		{
			return EVENTLOOP_FAIL;
		}
		return EVENTLOOP_SUCCEED;
	}

	int eventloop::start()
	{
		assert(data);
		int now=0,next=1;
		while(1)
		{
			{
				mutex_lock lock(data->wait_mutex_lock);
				while(!lock)
				{
					lock.locked();
				}
				while(data->wait!=EVENTLOOP_CONNTINUE)
				{
cond_back:
					if(::pthread_cond_wait(&data->wait_cond,&data->wait_mutex_lock)!=0)
					{
						goto cond_back;
					}
					if(data->wait==EVENTLOOP_END)
					{
						return EVENTLOOP_SUCCEED;
					}
				}
			}
			
		}
		int events_action_num=data->loop_fd.wait();
		for(int i=0;i<events_action_num;++i)
		{
			int fd=data->loop_fd.events[i].data.fd;
			auto callback_from_now=data->loop_callback_map[now].find(fd);
			if(callback_from_now!=data->loop_callback_map[now].end())
			{
				int event;
				callback_from_now->second->get_event(event);
				event|=data->loop_fd.events[fd].events;
				callback_from_now->second->set_event(event);
			}
			else
			{
				auto callback_from_all=data->all_callback_map.find(fd);
				assert(callback_from_all!=data->all_callback_map.end());
				callback_from_all->second->set_event(data->loop_fd.events[fd].events);
				data->loop_callback_map[now][fd]=callback_from_all->second;
			}
		}
		for(auto ptr=data->loop_callback_map[now].begin();ptr!=data->loop_callback_map[now].end();++ptr)
		{
			int event;
			ptr->second->get_event(event);
			if(event&EPOLLPRI)
			{
                             if(ptr->second->urgent_data_come()==EVENTLOOP_FINISH)
			     {
				     event&=~EPOLLPRI;
			     }
			}
			if(event&EPOLLIN)
			{
                             if(ptr->second->read()==EVENTLOOP_FINISH)
			     {
				     event&=~EPOLLIN;
			     }
			}
			if(event&EPOLLOUT)
			{
                             if(ptr->second->write()==EVENTLOOP_FINISH)
			     {
				     event&=~EPOLLOUT;
			     }
			}
			if(event&EPOLLRDHUP)
			{
                             if(ptr->second->shutdown()==EVENTLOOP_FINISH)
			     {
				     event&=~EPOLLRDHUP;
			     }
			}
			if(event&EPOLLHUP)
			{
                             ptr->second->close();
		             event=0;
			     
			}

		}
		data->loop_callback_map[now].clear();
                int temp=now;
		now=next;
		next=temp;
	}

	int eventloop::add_fd(shared_ptr<fd_callback>&callback)
	{
		int fd;
		callback->get_fd(fd);
		rwlock lock(data->lock,RWLOCK_WRITE_LOCK|RWLOCK_NOT_TRY);
		if(!lock)
		{
			return EVENTLOOP_LOCK_FAIL;
		}
		try
		{
			data->all_callback_map[fd]=callback;
		}
		catch(...)
		{
			return EVENTLOOP_FAIL;
		}
		lock.unlock();
		epoll_event event;
		callback->get_listen_event(event.events);
		event.data.fd=fd;
		if(::epoll_ctl(data->loop_fd,EPOLL_CTL_ADD,fd,&event)!=0)
		{
			return EVENTLOOP_FAIL;
		}
		return EVENTLOOP_SUCCEED;
	}

	int eventloop::del_fd(int fd,shared_ptr<fd_callback>
}
