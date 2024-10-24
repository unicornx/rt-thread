#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>
#include <dfs_romfs.h>
rt_weak uint8_t *cromfs_get_partition_data(uint32_t *len)
{
    return RT_NULL;
}

static int mnt_cromfs(void)
{
    uint32_t length = 0;
    uint8_t *data = cromfs_get_partition_data(&length);
    int ret = -1;

    if (data && length)
    {
        ret = dfs_mount(NULL, "/", "crom", 0, data);
    }

    return ret;
}

int mnt_init(void)
{
    rt_err_t ret;

    if (dfs_mount(RT_NULL, "/", "rom", 0, &romfs_root) != 0) {
        rt_kprintf("Dir / mount failed!\n");
        return -1;
    }

    mkdir("/dev/shm", 0x777);

    if (dfs_mount(RT_NULL, "/dev/shm", "tmp", 0, 0) != 0)
    {
        rt_kprintf("Dir /dev/shm mount failed!\n");
    }

#ifndef RT_FASTBOOT
    rt_kprintf("/dev/shm file system initialization done!\n");
#endif

#ifdef BSP_SD_SDIO_DEV
    while (mmcsd_wait_cd_changed(100) != MMCSD_HOST_PLUGED)
        ;

    if (dfs_mount(BSP_SD_MNT_DEVNAME, "/sdcard", "elm", 0, 0) != 0)
    {
        rt_kprintf("Dir /sdcard mount failed!\n");
    }
#endif
    return 0;
}
INIT_ENV_EXPORT(mnt_init);
#endif
