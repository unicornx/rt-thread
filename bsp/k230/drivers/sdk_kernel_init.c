#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

/*sdk kernel space init*/
rt_int32_t sdk_kernel_init(void)
{
    return RT_EOK;
}

INIT_COMPONENT_EXPORT(sdk_kernel_init);