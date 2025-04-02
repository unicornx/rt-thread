/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
*/

#include <rtthread.h>
#include <rttypes.h>
#include <rtservice.h>
#include <dfs.h>
#include <dfs_fs.h>
#include <dfs_mnt.h>
#include <dfs_file.h>
#include <dfs_dentry.h>
#include <dfs_posix.h>
#include "dfs_procfs.h"

static RT_DEFINE_SPINLOCK(procfs_lock);
static struct rt_proc_entry procfs_root =
{
    .parent.list = RT_LIST_OBJECT_INIT(procfs_root.parent.list),
#if defined(RT_USING_POSIX_FS)
    .fops = 0,
#endif
};

rt_proc_entry_t rt_proc_entry_find(const char *name)
{
    rt_base_t level;
    rt_object_t object;
    rt_proc_entry_t entry = RT_NULL;

    level = rt_spin_lock_irqsave(&procfs_lock);
    rt_list_for_each_entry(object, &(procfs_root.parent.list), list)
    {
        if (rt_strncmp(object->name, name, RT_NAME_MAX) == 0)
        {
            entry = (rt_proc_entry_t)object;
            break;
        }
    }
    rt_spin_unlock_irqrestore(&procfs_lock, level);

    return entry;
}

rt_err_t rt_proc_entry_register(rt_proc_entry_t entry, const char *name)
{
    if (entry == RT_NULL)
        return -RT_EINVAL;

    if (rt_proc_entry_find(name) != RT_NULL)
        return -RT_ERROR;

    rt_base_t level;
    level = rt_spin_lock_irqsave(&procfs_lock);
    rt_strncpy(entry->parent.name, name, RT_NAME_MAX);
    rt_list_insert_after(&(procfs_root.parent.list), &(entry->parent.list));
    rt_spin_unlock_irqrestore(&procfs_lock, level);

#if defined(RT_USING_POSIX_FS)
    entry->fops = NULL;
#endif

    return RT_EOK;
}

rt_err_t rt_proc_entry_unregister(rt_proc_entry_t entry)
{
    if (entry == RT_NULL)
        return -RT_EINVAL;

    rt_base_t level;
    level = rt_spin_lock_irqsave(&procfs_lock);
    rt_list_remove(&(entry->parent.list));
    rt_spin_unlock_irqrestore(&procfs_lock, level);

    return RT_EOK;
}

rt_proc_entry_t rt_proc_entry_create(int type, int attach_size)
{
    int size;
    rt_proc_entry_t entry;

    size = RT_ALIGN(sizeof(struct rt_proc_entry), RT_ALIGN_SIZE);
    attach_size = RT_ALIGN(attach_size, RT_ALIGN_SIZE);
    /* use the total size */
    size += attach_size;

    entry = (rt_proc_entry_t)rt_malloc(size);
    if (entry)
    {
        rt_memset(entry, 0x0, sizeof(struct rt_proc_entry));
        entry->type = (enum rt_proc_class_type)type;
    }

    return entry;
}

void rt_proc_entry_destory(rt_proc_entry_t entry)
{
    RT_ASSERT(entry != RT_NULL);

    rt_free(entry);
}

static int dfs_procfs_mount(struct dfs_mnt *mnt, unsigned long rwflag, const void *data)
{
    return RT_EOK;
}

static int dfs_procfs_stat(struct dfs_dentry *dentry, struct stat *buf)
{
    /* stat root directory */
    if ((dentry->pathname[0] == '/') && (dentry->pathname[1] == '\0'))
    {
        buf->st_dev = 0;
        buf->st_mode = S_IRUSR | S_IRGRP | S_IROTH |
                       S_IWUSR | S_IWGRP | S_IWOTH |
                       S_IXUSR | S_IXGRP | S_IXOTH |
                       S_IFDIR;
        buf->st_size = 0;
        return RT_EOK;
    }
    else
    {
        rt_proc_entry_t entry_id;
        entry_id = rt_proc_entry_find(&dentry->pathname[1]);
        if (entry_id != RT_NULL)
        {
            buf->st_dev = 0;
            buf->st_mode = S_IRUSR | S_IRGRP | S_IROTH |
                           S_IWUSR | S_IWGRP | S_IWOTH |
                           S_IFREG;
            buf->st_size = 0;
            return RT_EOK;
        }
    }
    return -ENOENT;
}

static struct dfs_vnode *dfs_procfs_lookup(struct dfs_dentry *dentry)
{
    struct stat st;
    struct dfs_vnode *vnode;

    if (dentry == NULL || dentry->mnt == NULL)
    {
        return NULL;
    }

    if (dfs_procfs_stat(dentry, &st) != 0)
    {
        return RT_NULL;
    }

    vnode = dfs_vnode_create();
    if (vnode)
    {
        vnode->mnt = dentry->mnt;
        vnode->size = st.st_size;
        vnode->mode = st.st_mode;
        if (S_ISDIR(st.st_mode))
        {
            vnode->type = FT_DIRECTORY;
        }
        else
        {
            vnode->type = FT_REGULAR;
        }
        vnode->data = NULL;
    }

    return vnode;
}

static int dfs_procfs_free_vnode(struct dfs_vnode *vnode)
{
    return RT_EOK;
}

static ssize_t dfs_proc_fops_read(struct dfs_file *file, void *buf, size_t count, off_t *pos)
{
    rt_proc_entry_t entry;
    ssize_t result;

    RT_ASSERT(file != RT_NULL);

    if (file->vnode && file->vnode->data)
    {
        entry = file->vnode->data;
#ifdef RT_USING_POSIX_FS
        if (entry->fops)
        {
            if (entry->fops->read)
            {
                result = entry->fops->read(file, buf, count, pos);
                if (result == RT_EOK || result == -RT_ENOSYS)
                {
                    return 0;
                }
            }
        }
#endif
    }

    return -RT_EIO;
}

static int dfs_proc_fops_close(struct dfs_file *file)
{
    rt_err_t result = RT_EOK;
    rt_proc_entry_t entry;

    RT_ASSERT(file != RT_NULL);
    RT_ASSERT(file->vnode->ref_count > 0);

    if (file->vnode->ref_count > 1)
    {
        return 0;
    }

    if (file->vnode && file->vnode->data)
    {
        entry = file->vnode->data;
#ifdef RT_USING_POSIX_FS
        if (entry->fops)
        {
            if (entry->fops->close)
            {
                result = entry->fops->close(file);
            }
        }
#endif
    }

    file->vnode->data = RT_NULL;

    return result;
}

static int dfs_proc_fops_open(struct dfs_file *file)
{
    rt_err_t result = RT_EOK;
    rt_proc_entry_t entry;
    rt_base_t level;
    RT_ASSERT(file->vnode->ref_count > 0);

    if (file->vnode->ref_count > 1)
    {
        file->fpos = 0;
        return 0;
    }

    /* open root directory */
    if ((file->dentry->pathname[0] == '/') && (file->dentry->pathname[1] == '\0') &&
        (file->flags & O_DIRECTORY))
    {
        file->vnode->data = &procfs_root;
        return RT_EOK;
    }

    entry = rt_proc_entry_find(&file->dentry->pathname[1]);
    if (entry == RT_NULL)
    {
        return -ENODEV;
    }

    file->vnode->data = entry;
#ifdef RT_USING_POSIX_FS
    if (entry->fops)
    {
        if (entry->fops->open)
        {
            result = entry->fops->open(file);
        }
    }
#endif

    return result;
}

static int dfs_proc_fops_getdents(struct dfs_file *file, struct dirent *dirp, uint32_t count)
{
    rt_base_t level;
    rt_object_t object;
    rt_proc_entry_t dir;
    struct dirent *d;
    uint32_t index = 0, offset;

    dir = (rt_proc_entry_t)file->vnode->data;
    RT_ASSERT(dir != RT_NULL);

    count = (count / sizeof(struct dirent));
    if (count == 0)
        return -EINVAL;

    offset = file->fpos / sizeof(struct dirent);
    level = rt_spin_lock_irqsave(&procfs_lock);
    rt_list_for_each_entry(object, &(dir->parent.list), list)
    {
        if (offset) {
            offset--;
            continue;
        }
        if (index >= count)
            break;
        d = dirp + index;
        d->d_type = DT_REG;
        d->d_namlen = RT_NAME_MAX;
        d->d_reclen = (rt_uint16_t)sizeof(struct dirent);
        rt_strncpy(d->d_name, object->name, RT_NAME_MAX);
        index++;
    }
    rt_spin_unlock_irqrestore(&procfs_lock, level);
    file->fpos += index * sizeof(struct dirent);

    return index * sizeof(struct dirent);
}

static const struct dfs_file_ops _procfs_fops =
{
    .open = dfs_proc_fops_open,
    .close = dfs_proc_fops_close,
    .read = dfs_proc_fops_read,
    .getdents = dfs_proc_fops_getdents,
};

static const struct dfs_filesystem_ops _procfs_fsops =
{
    .name = "procfs",
    .flags = DFS_FS_FLAG_DEFAULT,
    .default_fops = &_procfs_fops,
    .mount = dfs_procfs_mount,
    .stat = dfs_procfs_stat,
    .lookup = dfs_procfs_lookup,
    .free_vnode = dfs_procfs_free_vnode,
};

static struct dfs_filesystem_type _procfs =
{
    &_procfs_fsops,
};

static int procfs_init(void)
{
    dfs_register(&_procfs);

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(procfs_init);
