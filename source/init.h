#ifndef INIT
#define INIT

#include"light_shadow.h"

namespace init_space
{
	const int INIT_NO_HELP_SUCCEED=0;
	
	const int INIT_HELP_SUCCEED=-1;
	
	const int INIT_FAIL=-2;
        
	class init
	{
		private:
		    int	state;
		    int change_path_to_bin(void);
	    	    int daemonize(void);
		public:
		    init(int argc,char *argv[],light_shadow_space::light_shadow*);
		    int get_state()const;//获取状态
	};

}
#endif 
