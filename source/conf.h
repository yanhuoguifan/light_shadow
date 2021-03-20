#ifndef CONF
#define CONF

#include<string>
#include<errno.h>

namespace conf_space
{

const int CONF_SUCCEED=0;  //成功

const int CONF_DB_IP_MISS=1;  //数据库ip配置缺少

const int CONF_DB_USERNAME_MISS=2;  //数据库用户名配置缺少

const int CONF_DB_PASSWORD_MISS=4;  //数据库密码配置缺少

const int CONF_FILE_MISS=8;  //配置文件缺失

const int CONF_ELSS_ERR=16;  //其他错误

const int CONF_MISS=CONF_DB_IP_MISS|CONF_DB_USERNAME_MISS|CONF_DB_PASSWORD_MISS;  //配置缺少

const int CONF_ERR=CONF_MISS|CONF_FILE_MISS|CONF_ELSS_ERR;//配置初始化错误

using std::string;

struct conf_data; 

class conf
{
	private:
	      conf_data* data;//把数据封装进去
	      conf(char*);//参数是配置文件路径，若为空则配置文件在默认路径
	      int read_conf_file(char*);//读取配置文件信息，填充私有数据成员
	public:
              ~conf();//析构conf
	      static  conf*get_conf(char*);//构造全局唯一的conf，内部返回一个静态conf对象地址
	      int get_conf_err(string&)const;//报告配置错误信息
	      int get_conf_message(string&)const;//报告已读取的配置信息
	      int get_io_thread_num(unsigned int&)const;//获取io线程数量
	      int get_calculate_thread_num(unsigned int&)const;//获取计算线程数量
	      int get_state(int&)const;//获取状态码
	      int get_bind_ip(string&)const;//获取绑定ip
	      int get_port(unsigned int &)const;//获取绑定端口
	      int get_bind_domain_name(string&)const;//获取绑定域名
	      int get_db_ip(string&)const;//获取数据库ip
	      int get_db_username(string&)const;//获取数据库用户名
	      int get_db_password(string&)const;//获取数据库密码
	      int get_log_file_path(string&)const;//获取日志文件路径
	      int set_io_thread_num(unsigned int);//设置io线程数量
	      int set_calculate_thread_num(unsigned int );//设置计算线程数量
	      int set_state(int );//设置状态码
	      int set_bind_ip(string&);//设置绑定ip
	      int set_port(unsigned int);//设置绑定端口
	      int set_bind_domain_name(string&);//设置绑定域名
	      int set_db_ip(string&);//设置数据库ip
	      int set_db_username(string&);//设置数据库用户名
	      int set_db_password(string&);//设置数据库密码
	      int set_log_file_path(string&);//设置日志文件路径
};
}
#endif

