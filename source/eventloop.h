#ifndef EVENTLOOP
#define EVENTLOOP
#include<memory>
#include<vector>
#include<functional>
#include<sys/epoll.h>

namespace fd_space
{
	class fd_callback;
}

namespace eventloop_space
{
	const int EVENTLOOP_SUCCEED=0;

	const int EVENTLOOP_INIT_FAIL=-1;

	const int EVENTLOOP_LOCK_FAIL=-2;

	const int EVENTLOOP_FAIL=-3;

	const int EVENTLOOP_STOP=1;

	const int EVENTLOOP_CONNTINUE=2;

	const int EVENTLOOP_END=3;

	const int EVENTLOOP_FINISH=0;

	const int EVENTLOOP_NOT_FINISH=1;

	struct eventloop_data;

	class eventloop
	{
		private:
			eventloop_data*data;
		public:
			eventloop(int size=1024,int max_wait_listen=1024);
			int start();
			int loop_wait();
			int loop_continue();
			int loop_end(int *num=NULL,std::vector<std::shared_ptr<fd_space::fd_callback>>*vector=NULL);
			int insert_function();
			template<typename PARAMETER>
				int runat(std::function<int(PARAMETER)>&f,PARAMETER c);
			int add_fd(std::shared_ptr<fd_space::fd_callback>&);
			int del_fd(int fd,std::shared_ptr<fd_space::fd_callback>*ptr=NULL);
			int del_fd_num(int &num,std::vector<std::shared_ptr<fd_space::fd_callback>>*vector=NULL);
			int update_fd(int fd,int event);
	};

}

#endif

