#include"light_shadow.h"
#include"init.h"

using namespace std;
using namespace thread_space;
using namespace init_space;
using namespace rwlock_space;
namespace light_shadow_space
{
	struct light_shadow_data
	{
		pthread_rwlock_t lock;
                shared_ptr<thread_pool> pool;
                int state;
		light_shadow_data()
		{
			if(::pthread_rwlock_init(&lock,NULL)!=0)
			{
				state=LIGHT_SHADOW_INIT_FAIL;
			}
			else
			{
				state=LIGHT_SHADOW_INIT_BEFORE;
			}
		}
	};
	light_shadow::light_shadow()
	{
		try
		{
			data=new light_shadow_data;
		}
		catch(...)
		{
			data=NULL;
		}
		if(data->state==LIGHT_SHADOW_INIT_FAIL)
		{
			delete data;
			data=NULL;
		}
	}

	light_shadow*light_shadow::get_light_shadow(int argc,char*argv[])
	{
		static light_shadow light_shadow_one;
		if(light_shadow_one.data==NULL)
		{
			return NULL;
		}
		if(light_shadow_one.data->state==LIGHT_SHADOW_INIT_BEFORE)
		{
			init initall(argc,argv,&light_shadow_one);
			if(initall.get_state()!=INIT_NO_HELP_SUCCEED)
			{
				light_shadow_one.data->state=LIGHT_SHADOW_INIT_FAIL;
				return NULL;      
			}
			light_shadow_one.data->state=LIGHT_SHADOW_SUCCEED;
			return &light_shadow_one;
		}
		if(light_shadow_one.data->state==LIGHT_SHADOW_INIT_FAIL)
		{
			return NULL;
		}
		return &light_shadow_one;
	}

	int light_shadow::start()
	{
		return LIGHT_SHADOW_SUCCEED;
	}

	light_shadow::~light_shadow()
	{
		if(data)
		{
			::pthread_rwlock_destroy(&(data->lock));
			free(data);
		}
	}

	int light_shadow::get_pool(weak_ptr<thread_pool>&pool)
	{
		rwlock lock(data->lock,RWLOCK_READ_LOCK);
		if(!lock)
		{
			return LIGHT_SHADOW_LOCK_FAIL;
		}
                pool=data->pool;
		return LIGHT_SHADOW_SUCCEED;
	}

	int light_shadow::set_pool(shared_ptr<thread_pool>&pool)
	{
		rwlock lock(data->lock,RWLOCK_WRITE_LOCK);
		if(!lock)
		{
			return LIGHT_SHADOW_LOCK_FAIL;
		}
                data->pool=pool;
		return LIGHT_SHADOW_SUCCEED;
	}
}
