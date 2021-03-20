#include "conf.h"
#include<pthread.h>
#include<fstream>
#include<stdio.h>
#include<string.h>
#include<limits.h>
#include<unistd.h>
#include<sstream>

namespace conf_space
{
	static const char* default_conf_path="../conf/light_shadow.conf";
        
	static const char* IO_THREAD_NUM="io_thread_num";
	
	static const char* CALCULATE_THREAD_NUM="calculate_thread_num";
	
	static const char* PORT="port";
	
	static const char* BIND_IP="bind_ip";
	
	static const char* BIND_DOMAIN_NAME="bind_domain_name";
	
	static const char* DB_USERNAME="db_username";
	
	static const char* DB_IP="db_ip";
	
	static const char* DB_PASSWORD="db_password";
	
	static const char* LOG_FILE_PATH="log_file_path";
	
        inline string& path_to_absolute_path(string&path)
	{
		if(path[0]=='/')
		{
			return path;
		}
      	        char cwd[PATH_MAX+1];
		string absolute_path(getcwd(cwd,PATH_MAX+1));
		absolute_path.append("/").append(path);
		return path=absolute_path;
	}

	struct conf_data
	{
		int state;//状态码
		unsigned int  io_thread_num;//io线程数量
		char*conf_path;
		unsigned int calculate_thread_num;//计算线程数量
 		unsigned  int  port;//绑定端口
		string  bind_ip;//绑定ip
		string bind_domain_name;//绑定域名
		string db_ip;//数据库ip
		string db_username;//数据库用户名
		string db_password;//数据库密码
		string log_file_path;//日志文件路径
		pthread_rwlock_t conf_lock;//保护类全局对象的读写锁
	};
       
	conf::conf(char*conf_path)
	{
	   using namespace std;
	   static conf_data _data;
	   string conf_message_err;
           data=&_data;
	   data->calculate_thread_num=1;
	   data->io_thread_num=1;
	   data->port=80;
	   if(::pthread_rwlock_init(&(data->conf_lock),NULL)!=0)
	   {
	       ::fprintf(stderr,"fail init conf_data::pthread_rwlock_t,because %s\n",::strerror(errno));
	       data->state=CONF_ELSS_ERR;
	       return;
	   }
           if(read_conf_file(conf_path)!=CONF_SUCCEED)
	   {
	      while(get_conf_err(conf_message_err)!=CONF_SUCCEED)
	      {
		      conf_message_err.clear();
	      }
      	      ::fprintf(stderr,"%s",conf_message_err.c_str());
	      return;
	   }
	   while(get_conf_message(conf_message_err)!=CONF_SUCCEED)
	   {
	      conf_message_err.clear();
	   }
           ::printf("%s",conf_message_err.c_str());
	}

       int  conf::read_conf_file(char*conf_path)
        {
	   using namespace std;
           if(!conf_path)
	   {
              conf_path=(char*)default_conf_path;
	   }
	   data->conf_path=conf_path;
           ifstream conf_file(conf_path);
	   if(!conf_file.is_open())
	   {
	       ::fprintf(stderr,"fail open %s,because %s\n",conf_path,::strerror(errno));
	       data->state=CONF_FILE_MISS;
               return CONF_FILE_MISS;	       
	   }
	   data->state=CONF_MISS;
	   for(string key,line;getline(conf_file,line);)
	   {
		   stringstream sline(line);
		   if(!sline||!(sline>>key))
		   {
			   continue;
		   }
		   if(key==IO_THREAD_NUM)
		   {
		           sline>>data->io_thread_num;
		   }
		   else if(key==CALCULATE_THREAD_NUM)
		   {
                           sline>>data->calculate_thread_num;
		   }
		   else if(key==PORT)
		   {
			   sline>>data->port;
		   }
		   else if(key==BIND_IP)
		   {
			   sline>>data->bind_ip;
		   }
		   else if(key==BIND_DOMAIN_NAME)
		   {
			   sline>>data->bind_domain_name;
		   }
		   else if(key==LOG_FILE_PATH)
		   {
			   sline>>data->log_file_path;
			   path_to_absolute_path(data->log_file_path);
		   }
		   else if(key==DB_IP)
		   {
			   sline>>data->db_ip;
			   data->state&=~CONF_DB_IP_MISS;
		   }
		   else if(key==DB_PASSWORD)
		   {
			   sline>>data->db_password;
			   data->state&=~CONF_DB_PASSWORD_MISS;
		   }
		   else if(key==DB_USERNAME)
		   {
			   sline>>data->db_username;
			   data->state&=~CONF_DB_USERNAME_MISS;
		   }
		   else
		   {
	                   ::fprintf(stderr,"fail read %s,",key.c_str());
			   key.clear();
			   getline(sline,key);//重用了key
	                   ::fprintf(stderr,"skip %s\n",key.c_str());
		   }
  	   }
           return data->state;
       }
       
       conf::~conf()
       {
	       ::pthread_rwlock_wrlock(&(data->conf_lock));
	       ::pthread_rwlock_destroy(&(data->conf_lock));
       }

       conf*conf::get_conf(char*conf_path)
       {
	       static conf conf_one(conf_path);
	       return &conf_one;
       }
       
       int conf::get_conf_err(string&err)const
       {
           err.append("conf_err:\n");
	   int ret,state;
	   if((ret=pthread_rwlock_rdlock(&(data->conf_lock)))!=0)
	   {
		   return ret;
	   }
	   state=data->state;
           if(state&CONF_FILE_MISS)
	   {
		   err.append("can't find conf_file :");
		   err.append(path_to_absolute_path(data->log_file_path));
		   ret=pthread_rwlock_unlock(&(data->conf_lock));
	   }
	   else if(state&CONF_ERR)
	   {
		   ret=pthread_rwlock_unlock(&(data->conf_lock));
		   if(state&CONF_DB_IP_MISS)
	           {
                       err.append("miss database ip");
	           }
	           if(state&CONF_DB_PASSWORD_MISS)
	           {
                       err.append("miss database password");
	           }
	           if(state&CONF_DB_USERNAME_MISS)
	           {
                       err.append("miss database username");
	           }
	           if(state&CONF_ELSS_ERR)
	           {
                       err.append("unknown error");
	           }
	   }
	   else
	   {
		 ret=pthread_rwlock_unlock(&(data->conf_lock));
                 err.append("nothing");
	   }	
	   err.append("\n");
           return ret;	   
       }
       
       int conf::get_conf_message(string&message)const
       {
	  using namespace std;
	  int ret;
          message.append("conf message:\n");
	   if((ret=pthread_rwlock_rdlock(&(data->conf_lock)))!=0)
	   {
		   return ret;
	   }
          message.append("io_thread_num:").append(to_string(data->io_thread_num));
          message.append("\ncalculate_thread_num:").append(to_string(data->calculate_thread_num));
          message.append("\nbind port:").append(to_string(data->port));
          message.append("\nbind ip:").append(data->bind_ip);
          message.append("\nbind domain name:").append(data->bind_domain_name);
          message.append("\nbind database ip:").append(data->db_ip);
          message.append("\nbind database username:").append(data->db_username);
          message.append("\nbind database password:").append(data->db_password);
          message.append("\nlog file path:");
          ret=pthread_rwlock_unlock(&(data->conf_lock));
	  string path=data->conf_path;
	  message.append(path_to_absolute_path(path));
	  message.append("\n");
	  return ret;
       }
       
       int conf::get_io_thread_num(unsigned int&io_thread_num)const//获取io线程数量
       {
            int ret=::pthread_rwlock_rdlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    io_thread_num=data->io_thread_num;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }

       int conf::get_calculate_thread_num(unsigned int&calculate_thread_num)const//获取计算线程数量
       {
            int ret=::pthread_rwlock_rdlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    calculate_thread_num=data->calculate_thread_num;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }
       
       int conf::get_state(int&state)const//获取状态码
       {
            int ret=::pthread_rwlock_rdlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    state=data->state;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }

       int conf::get_bind_ip(string&db_ip)const//获取绑定ip
       {
            int ret=::pthread_rwlock_rdlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    db_ip=data->db_ip;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }

       int conf::get_port(unsigned int &port)const//获取绑定端口
       {
            int ret=::pthread_rwlock_rdlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    port=data->port;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }

       int conf::get_bind_domain_name(string&bind_domain_name)const//获取绑定域名
       {
            int ret=::pthread_rwlock_rdlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    bind_domain_name=data->bind_domain_name;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }

       int conf::get_db_ip(string&db_ip)const//获取数据库ip
       {
            int ret=::pthread_rwlock_rdlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    db_ip=data->db_ip;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }

       int conf::get_db_username(string&db_username)const//获取数据库用户名
       {
            int ret=::pthread_rwlock_rdlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    db_username=data->db_username;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }

       int conf::get_db_password(string&db_password)const//获取数据库密码
       {
            int ret=::pthread_rwlock_rdlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    db_password=data->db_password;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }
 
       int conf::get_log_file_path(string&log_file_path)const//获取日志文件路径
       {
            int ret=::pthread_rwlock_rdlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    log_file_path=data->log_file_path;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }
       
       int conf::set_io_thread_num(unsigned int io_thread_num)//设置io线程数量
       {
            int ret=::pthread_rwlock_wrlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    data->io_thread_num=io_thread_num;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }

       int conf::set_calculate_thread_num(unsigned int calculate_thread_num)//设置计算线程数量
       {
            int ret=::pthread_rwlock_wrlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    data->calculate_thread_num=calculate_thread_num;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }
	                   
       int conf::set_state(int state)//设置状态码
       {
            int ret=::pthread_rwlock_wrlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    data->state=state;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }
	                
       int conf::set_bind_ip(string&bind_ip)//设置绑定ip
       {
            int ret=::pthread_rwlock_wrlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    data->bind_ip=bind_ip;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }
	
       int conf::set_port(unsigned int port)//设置绑定端口
       {
            int ret=::pthread_rwlock_wrlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    data->port=port;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }
	               
       int conf::set_bind_domain_name(string&bind_domain_name)//设置绑定域名
       {
            int ret=::pthread_rwlock_wrlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    data->bind_domain_name=bind_domain_name;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }

       int conf::set_db_ip(string&db_ip)//设置数据库ip
       {
            int ret=::pthread_rwlock_wrlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    data->db_ip=db_ip;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }
	
       int conf::set_db_username(string&db_username)//设置数据库用户名
       {
            int ret=::pthread_rwlock_wrlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    data->db_username=db_username;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }
		              
       int conf::set_db_password(string&db_password)//设置数据库密码
       {
            int ret=::pthread_rwlock_wrlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    data->db_password=db_password;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }
		              
       int conf::set_log_file_path(string&log_file_path)//设置日志文件路径
       {
            int ret=::pthread_rwlock_wrlock(&(data->conf_lock));
	    if(ret!=0)
	    {
		    return ret;
	    }
	    data->log_file_path=log_file_path;
	    return ::pthread_rwlock_unlock(&(data->conf_lock));
       }
}


