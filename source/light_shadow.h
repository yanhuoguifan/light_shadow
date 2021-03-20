#ifndef LIGHT_SHADOW
#define LIGHT_SHADOW
#include"thread.h"

namespace light_shadow_space
{
	const int LIGHT_SHADOW_SUCCEED=0;

	const int LIGHT_SHADOW_LOCK_FAIL=-1;

	const int LIGHT_SHADOW_INIT_FAIL=-2;

	const int LIGHT_SHADOW_INIT_BEFORE=1;

	struct light_shadow_data;

	class light_shadow
	{
		private:
			light_shadow_data*data;
			light_shadow();
		public:
			static light_shadow*get_light_shadow(int argc,char *argv[]);
			int start();//开始整个程序
			~light_shadow();
			int get_pool(std::weak_ptr<thread_space::thread_pool>&);
			int set_pool(std::shared_ptr<thread_space::thread_pool>&);
	};
}
#endif
