/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
*/
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include <dfs.h>
#include <dfs_fs.h>
#include <dfs_dentry.h>
#include <dfs_posix.h>
#include <unistd.h>

#ifdef RT_USING_DFS_PROCFS
#ifdef RT_USING_PROC

struct proc_dirent
{
    rt_proc_entry_t *entrys;
    rt_uint16_t read_index;
    rt_uint16_t device_count;
};

int dfs_proc_fs_mount(struct dfs_mnt *mnt, unsigned long rwflag, const void *data)
{
    return RT_EOK;
}

static ssize_t dfs_proc_fops_read(struct dfs_file *file, void *buf, size_t count, off_t *pos)
{
    ssize_t ret = -RT_EIO;
    rt_proc_entry_t entry;
    ssize_t result;

    RT_ASSERT(file != RT_NULL);

    if (file->vnode && file->vnode->data)
    {
        entry = rt_proc_entry_find(&file->dentry->pathname[1]);
        if (entry == RT_NULL)
        {
            return -ENODEV;
        }

#ifdef RT_USING_POSIX_FS
        if (entry->fops)
        {

            file->vnode->fops = entry->fops;
            file->vnode->data = (void *)entry;

            if (file->vnode->fops->read)
            {
                result = file->vnode->fops->read(file, buf, count, pos);
                if (result == RT_EOK || result == -RT_ENOSYS)
                {
                    return 0;
                }
            }
        }
#endif

        file->vnode->data = RT_NULL;
    }

    return ret;
}

int dfs_proc_fops_close(struct dfs_file *file)
{
    rt_err_t result;
    rt_proc_entry_t entry_id;

    RT_ASSERT(file != RT_NULL);
    RT_ASSERT(file->vnode->ref_count > 0);

    if (file->vnode->ref_count > 1)
    {
        return 0;
    }

    if (file->vnode->type == FT_DIRECTORY)
    {
        struct proc_dirent *root_dirent;

        root_dirent = (struct proc_dirent *)file->vnode->data;
        RT_ASSERT(root_dirent != RT_NULL);

        /* release dirent */
        rt_free(root_dirent);
        return RT_EOK;
    }

    entry_id = (rt_proc_entry_t)file->vnode->data;
    RT_ASSERT(entry_id != RT_NULL);
    file->vnode->data = RT_NULL;

    return RT_EOK;

}

int dfs_proc_fops_open(struct dfs_file *file)
{
    rt_err_t result;
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
        struct rt_object *object;
        struct rt_list_node *node;
        struct rt_object_information *information;
        struct proc_dirent *root_dirent;
        rt_uint32_t count = 0;

        /* disable interrupt */
        level = rt_hw_interrupt_disable();

        information = rt_object_get_information(RT_Object_Class_Proc);
        RT_ASSERT(information != RT_NULL);
        for (node = information->object_list.next; node != &(information->object_list); node = node->next)
        {
            count ++;
        }

        root_dirent = (struct proc_dirent *)rt_malloc(sizeof(struct proc_dirent) +
                      count * sizeof(rt_proc_entry_t));
        if (root_dirent != RT_NULL)
        {
            root_dirent->entrys = (rt_proc_entry_t *)(root_dirent + 1);
            root_dirent->read_index = 0;
            root_dirent->device_count = count;
            count = 0;
            /* get all proc entry  node */
            for (node = information->object_list.next; node != &(information->object_list); node = node->next)
            {
                object = rt_list_entry(node, struct rt_object, list);
                root_dirent->entrys[count] = (rt_proc_entry_t)object;
                count ++;
            }
        }
        rt_hw_interrupt_enable(level);

        /* set data */
        file->vnode->data = root_dirent;

        return RT_EOK;
    }

    entry = rt_proc_entry_find(&file->dentry->pathname[1]);
    if (entry == RT_NULL)
    {
        return -ENODEV;
    }

#ifdef RT_USING_POSIX_FS
    if (entry->fops)
    {

        file->vnode->fops = entry->fops;
        file->vnode->data = (void *)entry;

        if (file->vnode->fops->open)
        {
            result = file->vnode->fops->open(file);
            if (result == RT_EOK || result == -RT_ENOSYS)
            {
                file->vnode->type = FT_DEVICE;
                return 0;
            }
        }
    }
#endif

    file->vnode->data = RT_NULL;
    return -EIO;
}

int dfs_proc_fs_stat(struct dfs_dentry *dentry, struct stat *buf)
{
    /* stat root directory */
    if ((dentry->pathname[0] == '/') && (dentry->pathname[1] == '\0'))
    {
        buf->st_dev = 0;

        buf->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH |
                      S_IWUSR | S_IWGRP | S_IWOTH;
        buf->st_mode &= ~S_IFREG;
        buf->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;

        buf->st_size  = 0;
        buf->st_mtime = 0;

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
                          S_IWUSR | S_IWGRP | S_IWOTH;

            buf->st_size  = 0;
            buf->st_mtime = 0;
            return RT_EOK;
        }
    }
    return -ENOENT;
}

int dfs_proc_fops_getdents(struct dfs_file *file, struct dirent *dirp, uint32_t count)
{
    rt_uint32_t index;
    rt_object_t object;
    struct dirent *d;
    struct proc_dirent *root_dirent;

    root_dirent = (struct proc_dirent *)file->vnode->data;
    RT_ASSERT(root_dirent != RT_NULL);

    /* make integer count */
    count = (count / sizeof(struct dirent));
    if (count == 0)
        return -EINVAL;

    for (index = 0; index < count && index + root_dirent->read_index < root_dirent->device_count; 
        index ++)
    {
        object = (rt_object_t)root_dirent->entrys[root_dirent->read_index + index];

        d = dirp + index;
        d->d_type = DT_REG;
        d->d_namlen = RT_NAME_MAX;
        d->d_reclen = (rt_uint16_t)sizeof(struct dirent);
        rt_strncpy(d->d_name, object->name, RT_NAME_MAX);
    }

    root_dirent->read_index += index;

    return index * sizeof(struct dirent);
    return 0;
}

static const struct dfs_file_ops _proc_fops;

static struct dfs_vnode *dfs_proc_fs_create_vnode(struct dfs_dentry *dentry, int type, mode_t mode)
{
    struct dfs_vnode *vnode = RT_NULL;
    char parent_path[DFS_PATH_MAX], file_name[DIRENT_NAME_MAX];

    if (dentry == NULL || dentry->mnt == NULL)
    {
        return NULL;
    }

    vnode = dfs_vnode_create();
    if (vnode)
    {
        vnode->nlink = 1;
        vnode->size = 0;
        vnode->mode = mode;
        vnode->mnt = dentry->mnt;
        vnode->fops = &_proc_fops;

        if (type == FT_DIRECTORY)
        {
            vnode->type = FT_DIRECTORY;
            vnode->mode &= ~S_IFMT;
            vnode->mode |= S_IFDIR;
        }
        else
        {
            vnode->type = FT_DEVICE;
        }
    }

    return vnode;
}

static int dfs_proc_fs_free_vnode(struct dfs_vnode *vnode)
{
    return RT_EOK;
}

static struct dfs_vnode *dfs_proc_fs_lookup(struct dfs_dentry *dentry)
{
    struct dfs_vnode *vnode = RT_NULL;
    rt_proc_entry_t entry;

    if (dentry == NULL || dentry->mnt == NULL)
    {
        return NULL;
    }

    entry = rt_proc_entry_find(dentry->pathname);   

    vnode = dfs_vnode_create();
    if (vnode)
    {
        vnode->nlink = 1;
        vnode->size = 0;
        vnode->mnt = dentry->mnt;
        vnode->fops = &_proc_fops;
        vnode->mode = S_IFDIR | (S_IRUSR | S_IRGRP | S_IROTH) | (S_IXUSR | S_IXGRP | S_IXOTH);
        vnode->type = FT_DIRECTORY;
    }

    return vnode;
}


static const struct dfs_file_ops _proc_fops =
{
    dfs_proc_fops_open,
    dfs_proc_fops_close,
    RT_NULL,                    
    dfs_proc_fops_read,           /* read */
    RT_NULL,                    /* write */
    RT_NULL,                    /* flush */
    RT_NULL,                    /* lseek */
    RT_NULL,                    /* truncate */
    dfs_proc_fops_getdents,
    RT_NULL,
};

static const struct dfs_filesystem_ops _proc_fs =
{
    "procfs",
    DFS_FS_FLAG_DEFAULT,
    &_proc_fops,

    dfs_proc_fs_mount,
    RT_NULL,
    RT_NULL,

    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,

    RT_NULL,
    dfs_proc_fs_stat,
    RT_NULL,
    RT_NULL,
    dfs_proc_fs_lookup,                 /* lookup */
    dfs_proc_fs_create_vnode,            /* create vnode */
    dfs_proc_fs_free_vnode,              /* free vnode */
};

static struct dfs_filesystem_type _proc_type =
{
    &_proc_fs,
    RT_NULL,
};

static void dfs_porcfs_mkdir(const char *fullpath, mode_t mode)
{
    int len = rt_strlen(fullpath);
    char *path = (char *)rt_malloc(len + 1);

    if (path)
    {
        int index = len - 1;

        rt_strcpy(path, fullpath);

        if (path[index] == '/')
        {
            path[index] = '\0';
            index --;
        }

        if (index > 0 && access(path, 0) != 0)
        {
            int i = 0;

            if (path[i] == '/')
            {
                i ++;
            }

            while (index > i)
            {
                if (path[i] == '/')
                {
                    path[i] = '\0';
                    mkdir(path, mode);
                    path[i] = '/';
                }

                i ++;
            }

            mkdir(path, mode);
        }
    }
}

void dfs_procfs_device_add(rt_proc_entry_t entry)
{
    int fd;
    char path[512];

    if (entry)
    {
        rt_snprintf(path, 512, "/proc/%s", entry->parent.name);

        if (access(path, 0) != 0)
        {
            mode_t mode = O_RDONLY | O_DIRECTORY;

            dfs_porcfs_mkdir(path, mode);

            fd = open(path, O_RDWR | O_CREAT, mode);
            if (fd >= 0)
            {
                close(fd);
            }
        }
    }
}

int dfs_procfs_update(void)
{
    int count = rt_object_get_length(RT_Object_Class_Proc);
    if (count > 0)
    {
        rt_proc_entry_t *entry = rt_malloc(count * sizeof(rt_proc_entry_t));
        if (entry)
        {
            rt_object_get_pointers(RT_Object_Class_Proc, (rt_object_t *)entry, count);

            for (int index = 0; index < count; index ++)
            {
                dfs_procfs_device_add(entry[index]);
            }
            rt_free(entry);
        }
    }

    return count;
}

int procfs_init(void)
{
    int ret = 0;

    ret = dfs_register(&_proc_type);
    if(ret < 0)
    {
        rt_kprintf("register procfs failed\n");
    }

    ret = dfs_mount(NULL, "/proc", "procfs", 0, 0);
    if (ret < 0)
    {
        rt_kprintf("mount /proc failed\n");
    }

    dfs_procfs_update();

    return ret;
}
INIT_FS_EXPORT(procfs_init);

#endif
#endif