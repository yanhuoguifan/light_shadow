#ifndef FD
#define FD

#include<memory>
#include<functional>
#include<assert.h>
#include<unistd.h>
#include<vector>
#include<sys/socket.h>
#include"mutex_lock.h"
#include"eventloop.h"

namespace fd_space
{
	const int FD_SUCCEED=0;

	const int FD_FAIL=-1;

	const int FD_LOCK_FAIL=-2;

	const unsigned int FD_DESTROY_NOTHING=0;

	const unsigned int FD_DESTROY_CLOSE=1;

	const unsigned int FD_DESTROY_SHUTOWN=2;

	class fd_callback:public std::enable_shared_from_this<fd_callback>//由于是性能热点，没有进行实现与接口分离
	{
		private:
			std::function<int(std::shared_ptr<fd_callback>)>readable_callback;
	    		std::function<int(std::shared_ptr<fd_callback>)>writeable_callback;
	    		std::function<int(std::shared_ptr<fd_callback>)>urgentable_callback;
			std::function<int(std::shared_ptr<fd_callback>)>shutdown_callback;
	    		std::function<int(std::shared_ptr<fd_callback>)>closeable_callback;
			int fd;
			int event;
			int listen_event;
			int state;
			pthread_mutex_t lock;
		public:
			fd_callback(int _fd,int _listen_event=0,std::function<int(std::shared_ptr<fd_callback>)> _readable_callback=NULL,std::function<int(std::shared_ptr<fd_callback>)>_writeable_callback=NULL,std::function<int(std::shared_ptr<fd_callback>)>_urgentable_callback=NULL,std::function<int(std::shared_ptr<fd_callback>)>_shutdown_callback=NULL,std::function<int(std::shared_ptr<fd_callback>)>_closeable_callback=NULL):
                        readable_callback(_readable_callback),
			writeable_callback(_writeable_callback),
			urgentable_callback(_urgentable_callback),
			shutdown_callback(_shutdown_callback),
			closeable_callback(_closeable_callback),
			fd(_fd),
			event(0),
			listen_event(_listen_event),
			state(FD_SUCCEED)
			{
				if(::pthread_mutex_init(&lock,NULL)!=0)
				{
    					state=FD_LOCK_FAIL;
				}
			}

			~fd_callback()
			{
				if(state!=FD_LOCK_FAIL)
				{
					::pthread_mutex_destroy(&lock);
				}
			}

			int get_control()
			{
				if(::pthread_mutex_trylock(&lock)==0)
				{
					return FD_SUCCEED;
				}
				return FD_LOCK_FAIL;
			}

			int free_control()
			{
				if(::pthread_mutex_unlock(&lock)==0)
				{
					return FD_SUCCEED;
				}
				return FD_LOCK_FAIL;
			}

			int read()
			{
				int ret;
				if(readable_callback)
				{
				        ret=readable_callback(shared_from_this());
				}
				else
				{
					ret=eventloop_space::EVENTLOOP_FINISH;
				}
				return ret;
			}

			int write()
			{
				int ret;
				if(writeable_callback)
				{
					ret= writeable_callback(shared_from_this());
				}
				else
				{
					ret=eventloop_space::EVENTLOOP_FINISH;
				}
				return ret;
			}

			int shutdown()
			{
				int ret;
				if(shutdown_callback)
				{
					ret=shutdown_callback(shared_from_this());
				}
				else
				{
					ret=eventloop_space::EVENTLOOP_FINISH;
				}
				return ret;
			}

			int urgent_data_come()
			{
				int ret;
				if(urgentable_callback)
				{
			          	ret=urgentable_callback(shared_from_this());
				}
				else
				{
					ret=eventloop_space::EVENTLOOP_FINISH;
				}
				return ret;
			}

			int close()
			{
				int ret;
				if(closeable_callback)
				{
				        ret=closeable_callback(shared_from_this());
				}
				else
				{
					ret=eventloop_space::EVENTLOOP_FINISH;
				}
				return ret;
			}

			void set_event(int _event)
			{
				event=_event;
			}

			void get_event(int&_event)
			{
				_event=event;
			}

			void get_listen_event(int _listen_event)
			{
				_listen_event=listen_event;
			}

			void set_listen_event(int _listen_event)
			{
				listen_event=_listen_event;
			}

			void get_fd(int&_fd)
			{
				_fd=fd;
			}
	};

	class _fd
	{
		private:
			int __fd;
			unsigned int destroy_action;
			int shutdown_type;
		public:
			_fd(unsigned int _destroy_action=FD_DESTROY_NOTHING,int _shutdown_type=SHUT_RDWR):
			destroy_action(_destroy_action),shutdown_type(_shutdown_type)
			{
				__fd=-1;
			}
               
		       	~_fd()
			{
				if(destroy_action==FD_DESTROY_CLOSE)
				{
					::close(__fd);
				}
				else if(destroy_action==FD_DESTROY_SHUTOWN)
				{
					::shutdown(__fd,shutdown_type);
				}
		        }

			void get_fd(int&fd)
			{
				fd=__fd;
			}

			void set_fd(int fd)
			{
				__fd=fd;
			}

			void set_destroy_action(unsigned int _destroy_action)
			{
				assert(_destroy_action==FD_DESTROY_CLOSE||_destroy_action==FD_DESTROY_NOTHING||_destroy_action==FD_DESTROY_SHUTOWN);
				destroy_action=_destroy_action;
			}

			void get_destroy_action(unsigned int &_destroy_action)
			{
				_destroy_action=destroy_action;
			}

			void get_shutdown_type(unsigned int &_shutdown_type)
			{
				_shutdown_type=shutdown_type;
			}

			void set_shutdown_type(unsigned int _shutdown_type)
			{
				assert(_shutdown_type==SHUT_RDWR||_shutdown_type==SHUT_WR||_shutdown_type==SHUT_RD);
				shutdown_type=_shutdown_type;
			}

			operator int&()
			{
				return __fd;
			}
	};

}

#endif
