#include"init.h"
#include"arg.h"
#include"conf.h"
#include"thread.h"
#include<sys/stat.h>
#include<sys/resource.h>
#include<signal.h>
#include<limits.h>

using namespace conf_space;
using namespace arg_space;
using namespace thread_space;
using namespace light_shadow_space;
using namespace std;

void* func(int state,std::shared_ptr<_thread>ptr)
{
     state=1;
     pthread_t ppid;
     ptr->get_ppid(ppid);
     printf("%lu\n",ppid);
     pid_t tpid;
     ptr->get_tpid(tpid);
     printf("%d",tpid);
     return NULL;
}
class a
{
	public:
	void* operator()(int state,std::shared_ptr<_thread>ptr)
	{
     state=1;
     pthread_t ppid;
     ptr->get_ppid(ppid);
     printf("%lu\n",ppid);
     pid_t tpid;
     ptr->get_tpid(tpid);
     printf("%d",tpid);
     return NULL;
	}
};

namespace init_space
{

	init::init(int argc,char *argv[],light_shadow*light_shadow_one)
	{
		if(change_path_to_bin()==INIT_FAIL)
		{
			state=INIT_FAIL;
			return;
		}
		arg init_arg(argc,argv);
		int arg_state=init_arg.get_state();
		if((arg_state&ARG_FAIL)==ARG_FAIL)
		{
			state=INIT_FAIL;
			return;
		}
		else if((arg_state&ARG_HELP)==ARG_HELP)
		{
			state=INIT_HELP_SUCCEED;
			return;
		}
		conf *init_conf=conf::get_conf(init_arg.get_conf_file_path());
                int conf_state;
		if((init_conf->get_state(conf_state)!=0)||(conf_state&CONF_ERR))
		{
			state=INIT_FAIL;
			return;
		}
		unsigned int io_thread_num,calculate_thread_num;
		if((init_conf->get_io_thread_num(io_thread_num)!=CONF_SUCCEED)
		||(init_conf->get_calculate_thread_num(calculate_thread_num)!=CONF_SUCCEED)
		||(io_thread_num<3)||(calculate_thread_num<1))
		{
			state=INIT_FAIL;
			return;
		}
		shared_ptr<thread_pool> poolptr;
		try
		{
		     	poolptr=shared_ptr<thread_pool>(new thread_pool(io_thread_num,calculate_thread_num));
                }
		catch(...)
		{
				state=INIT_FAIL;
				return;
		}
		int pool_state=THREAD_POOL_INIT_FAIL;
		if((THREAD_POOL_SUCCEED!=poolptr->get_state(pool_state)||pool_state!=THREAD_POOL_SUCCEED))
		{
				state=INIT_FAIL;
				return;
		}
		if(light_shadow_one->set_pool(poolptr)!=LIGHT_SHADOW_SUCCEED)
		{
			state=INIT_FAIL;
			return;
		}
		if(daemonize()==INIT_FAIL)
		{
			state=INIT_FAIL;
			return;
		}
	auto *ptr=new thread<void*(*)(int,std::shared_ptr<_thread>)>(func,THREAD_IO_TYPE,poolptr);
	{
		auto sptr=std::shared_ptr<_thread>(ptr);
		sptr->start();
		sptr->start_wait(THREAD_MAIN);
		poolptr->set_thread(sptr);
	}
	a d;
	auto*aptr=new thread<a>(d,THREAD_ALL_TYPE,poolptr);
	{
		auto sptr=std::shared_ptr<_thread>(aptr);
		sptr->start();
		sptr->start_wait(THREAD_MAIN);
		poolptr->set_thread(sptr);

	}
//以后删了
	        state=INIT_NO_HELP_SUCCEED;
	}

	int init::get_state()const
	{
               return state;
	}
	int init::daemonize(void)
	{
		unsigned long i;
	      	pid_t pid;
	      	struct rlimit rl;
	      	struct sigaction sa;
	      	umask(0);
		if(getrlimit(RLIMIT_NOFILE,&rl)<0)
		{
		   	write(STDERR_FILENO,"get file limit fail\n",21);
		   	return INIT_FAIL;
		}
		if((pid=fork())<0)
		{
			write(STDERR_FILENO,"fork fail\n",11);
			return INIT_FAIL;
		}
		else if(pid!=0)
		{
			exit(0);
		}
		setsid();
		sa.sa_handler=SIG_IGN;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags=0;
		if(sigaction(SIGHUP,&sa,NULL)<0)
		{
			write(STDERR_FILENO,"can't ignore SIGHUP\n",21);
			return INIT_FAIL;
		}
		if((pid=fork())<0)
		{
			write(STDERR_FILENO,"fork fail\n",11);
			return INIT_FAIL;
		}
		else if(pid!=0)
		{
			exit(0);
		}
		if(chdir("/")<0)
		{
			write(STDERR_FILENO,"can't change directory to /\n",29);
			return INIT_FAIL;
		}
		for(i=3;i<rl.rlim_max;++i)
		{
			close(i);
		}
		/*
		fd0=open("/dev/null",O_RDWR);
		fd1=dup(0);
		fd2=dup(0);
		if(fd0!=0||fd1!=1||fd2!=2)
		{
			return INIT_FAIL;
		}
		*/
		return INIT_NO_HELP_SUCCEED;
	}

        int init::change_path_to_bin(void)
        {
		char dir[PATH_MAX];
		int n=::readlink("/proc/self/exe",dir,PATH_MAX);
		if(n==-1)
		{
			return INIT_FAIL;
		}
		while(--n>=0)
		{
			if(dir[n]=='/')
			{
				dir[++n]='\0';
				break;
			}
		}
		if(n<=0)
		{
			return INIT_FAIL;
		}
		if(chdir(dir)<0)
		{
			::fprintf(stderr,"can't change directory to %s\n",dir);
			return INIT_FAIL;
		}
		return INIT_NO_HELP_SUCCEED|INIT_HELP_SUCCEED;
	}
}
