#include "blkdev.h"
#include "lib/fatfs/ff.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "extmod/fsusermount.h"

int xen_blkdev_open(blkdev_id_t id);
int minios_block_init_vfs(fs_user_mount_t *vfs);
