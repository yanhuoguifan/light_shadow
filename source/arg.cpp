#include"arg.h"
#include<stdlib.h>
#include<stdio.h>
#include <unistd.h>
#include <fcntl.h> 

namespace arg_space
{
        static const char*HELP_FILE_PATH="../help/help.txt";//帮助文件路径;
        
	static const char*ARG_PARSEMODULE="c:h";//参数解析模式
        
        static void  printf_help();

	struct arg_data
	{
		char*conf_file_path;//配置文件路径
		int state;//状态
	};

	arg::arg(int argc,char *argv[])
	{
               data=(arg_data*)malloc(sizeof(arg_data));
	       if(data==NULL)
	       {
		       ::fprintf(stderr,"malloc arg_data fail\n ");
		       return;
	       }
	       data->state=0;
	       data->conf_file_path=NULL;
               extern char*optarg;
	       extern int optind,opterr,optopt;
	       int ret;
	       while((ret=::getopt(argc,argv,ARG_PARSEMODULE))!=-1)
	       {
		       switch(ret)
		       {
			       case('h'):
			       {
				   data->state|=ARG_HELP;
				   break;
			       }
			       case('c'):
			       {
				   data->conf_file_path=argv[::optind];
				   data->state|=ARG_CONF_FILE_PATH;
			           break;
			       }
			       case('?'):
			       {
                                   ::fprintf(stderr,"you can't input %c\n",::optopt);
				   data->state|=ARG_FAIL;
				   break;
			       }
			       case(':'):
			       {
                                   ::fprintf(stderr,"you should input conf file path after"
				   " '-c'\n");
				   data->state|=ARG_FAIL;
                                   break;
			       }
		       }
	       }
               if((data->state!=ARG_FAIL)&&(data->state&ARG_HELP)==ARG_HELP)
	       {
		       printf_help();
	       }
	}

	arg::~arg()
	{
		free(data);
	}

	int arg::get_state()const
	{
		if(data==NULL)
		{
			return ARG_FAIL;
		}
		return data->state;
	}

	char* arg::get_conf_file_path()const
	{
		if(data==NULL)
		{
                     ::fprintf(stderr,"don't use bad arg\n");
		     exit(-1);
		}
	        return data->conf_file_path;
	}

	static void printf_help()
	{
      		int help=::open(HELP_FILE_PATH,O_RDONLY);
      		if(help<0)
      		{
	  		::fprintf(stderr,"fail open %s file\n",HELP_FILE_PATH);
	  		return;
       		}
		char buf[1024];
      		int ret;
     		while((ret=::read(help,buf,1024))>0)
      		{
			if(ret==0)
	      		{
		   		break;
	      		}
	      		if(ret<0)
	      		{
	  		        ::fprintf(stderr,"fail read %s file\n",HELP_FILE_PATH);
		   		break;
	      		}
	       		write(STDOUT_FILENO,buf,ret);
	      		if(ret<1024)
	      		{
		   		break;
	      		}
      		}
      		close(help);              
	}
}
