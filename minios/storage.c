#include "storage.h"
#include "blkdev.h"

// GET equivalents from shfs/blkdev.h
//bool storage_read_block(uint8_t *dest, uint32_t block) {
// bool storage_write_block(const uint8_t *src, uint32_t block) {

static struct blkdev *bd = NULL;

STATIC bool xen_blkdev_readblocks(uint8_t *dest, uint32_t block) {
  if (bd == NULL) return false;
  
  return (blkdev_sync_io(bd, (sector_t) block, 1, 0, dest) >= 0);
}

STATIC bool xen_blkdev_writeblocks(const uint8_t *src, uint32_t block) {
  if (bd == NULL) return false;

  return (blkdev_sync_io(bd, (sector_t) block, 1, 1, src) >= 0);
}

int xen_blkdev_open(blkdev_id_t id)
{
  bd = open_blkdev(id, O_RDWR);
  if (!bd) return -1;
  return 0;
}

int minios_block_init_vfs(fs_user_mount_t *vfs) {

  //  bd = open_blkdev(id, O_RDWR);
  //  if (!bd) return -1;
		   
  vfs->flags |= FSUSER_NATIVE | FSUSER_HAVE_IOCTL;
  vfs->readblocks[0] = (mp_obj_t)&xen_blkdev_readblocks;
  vfs->readblocks[1] = (mp_obj_t)&xen_blkdev_readblocks;
  vfs->readblocks[2] = (mp_obj_t)xen_blkdev_readblocks; // native version
  vfs->writeblocks[0] = (mp_obj_t)&xen_blkdev_writeblocks;
  vfs->writeblocks[1] = (mp_obj_t)&xen_blkdev_writeblocks;
  vfs->writeblocks[2] = (mp_obj_t)xen_blkdev_writeblocks; // native version
  //  vfs->u.ioctl[0] = (mp_obj_t)&pyb_flash_ioctl_obj;
  //vfs->u.ioctl[1] = (mp_obj_t)&pyb_flash_obj;
  return 0;
}
