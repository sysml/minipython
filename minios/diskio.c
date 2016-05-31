/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs on MiniOS                         */
/*-----------------------------------------------------------------------*/
#include "diskio.h"		/* FatFs lower layer API      */
#include "blkdev.h"             /* MiniOS block device driver */

#define XEN_XVDA_DEVID 51712

/* The block device */
static struct blkdev *bd = NULL;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
	BYTE pdrv	/* Physical drive nmuber to identify the drive */
)
{
	if (bd == NULL) {
	  return STA_NOINIT;
	}
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
	BYTE pdrv  	/* Physical drive nmuber to identify the drive */
)
{
        bd = open_blkdev((blkdev_id_t)pdrv * 16 + XEN_XVDA_DEVID, O_RDWR);
	if (!bd) return STA_NOINIT;
	return RES_OK;
}
 
/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
	BYTE pdrv,	/* Physical drive nmuber to identify the drive */
	BYTE *buff,	/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count	/* Number of sectors to read */
)
{
        void *iobuf;
	sector_t slen;
	
	if (bd == NULL) {
	  return RES_PARERR;
	}

	slen = count * blkdev_ssize(bd);
	iobuf = _xmalloc(slen, blkdev_ioalign(bd));
  
	if (blkdev_sync_io(bd, (sector_t)sector, count, 0, iobuf) < 0) {
  	    xfree(iobuf);
	    return RES_ERROR;
	}
	memcpy(buff, iobuf, slen);
	xfree(iobuf);
	       
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
DRESULT disk_write (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count		/* Number of sectors to write */
)
{
        void *iobuf;
	sector_t slen;
	
	if (bd == NULL) {
	  return RES_PARERR;
	}	

        slen = count * blkdev_ssize(bd);
	iobuf = _xmalloc(slen, blkdev_ioalign(bd));
	memcpy(iobuf, buff, slen);
	
	if (blkdev_sync_io(bd, (sector_t)sector, count, 1, iobuf) < 0) {
	  xfree(iobuf);
	  return RES_ERROR;
	}

	xfree(iobuf);
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	if (bd == NULL) {
	  return RES_PARERR;
	}

	switch (cmd) {
	  case CTRL_SYNC: {
	    /* not yet supported by the block device driver */
	    return RES_OK;
	  }
	  case GET_SECTOR_COUNT: {
	    *((DWORD*)buff) = blkdev_sectors(bd);
	    return RES_OK;
	  }
	  case GET_SECTOR_SIZE: {
	    *((DWORD*)buff) = blkdev_ssize(bd);
	    return RES_OK;
	  }
	  case GET_BLOCK_SIZE: {
	    *((DWORD*)buff) = 1; // erase block size in units of sector size
	    return RES_OK;
	  }
	  default: {
	    return RES_PARERR;
	  }
	}
	    
	return RES_PARERR;
}
