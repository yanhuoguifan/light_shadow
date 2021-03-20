#ifndef ARG
#define ARG

namespace arg_space
{
	const int ARG_CONF_FILE_PATH=1;//有配置文件路径
	
	const int ARG_HELP=2;//请求打印帮助文件
	
	const int ARG_SUCCEED=ARG_CONF_FILE_PATH|ARG_HELP;//成功读取
	
	const int ARG_FAIL=4;//解析参数失败

	struct arg_data;

	class arg
	{
		private:
		       arg_data*data;//数据放在这
		public:
		       arg(int argc,char *argv[]);//构造arg并解析参数
		       int get_state()const;//获取当前状态
		       char*get_conf_file_path()const;//获取配置文件路径 
	               ~arg();
	};
}
#endif 
