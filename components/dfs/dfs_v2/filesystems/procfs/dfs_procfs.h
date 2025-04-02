/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
*/

#ifndef __DFS_PROCFS_H__
#define __DFS_PROCFS_H__

#include <rtdef.h>

enum rt_proc_class_type
{
    RT_Proc_Class_CpuInfo = 0,
    RT_Proc_Class_MemInfo,
    RT_Proc_Class_DeviceInfo,
    RT_Proc_Class_Unknow
};

struct rt_proc_entry
{
    struct rt_object            parent;                   /**< inherit from rt_object */
    rt_uint16_t                 flag;
    rt_uint16_t                 open_flag;
    rt_uint8_t                  ref_count;
    enum rt_proc_class_type     type;                     /**< device type */
#if defined(RT_USING_POSIX_FS)
    const struct dfs_file_ops   *fops;
#endif
    void *data;
};

typedef struct rt_proc_entry *rt_proc_entry_t;

rt_proc_entry_t rt_proc_entry_find(const char *name);
rt_err_t rt_proc_entry_register(rt_proc_entry_t entry, const char *name);
rt_err_t rt_proc_entry_unregister(rt_proc_entry_t entry);
rt_proc_entry_t rt_proc_entry_create(int type, int attach_size);
void rt_proc_entry_destory(rt_proc_entry_t entry);

#endif
