/*
 * Block device wrapper for MiniOS
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2013-2017, NEC Europe Ltd., NEC Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef _BLKDEV_H_
#define _BLKDEV_H_

#include <mini-os/blkfront.h>
#include <mini-os/semaphore.h>
#include <fcntl.h>

#include "mempool.h"

#define MAX_REQUESTS ((__RING_SIZE((struct blkif_sring *)0, PAGE_SIZE)) - 1)
#define MAX_DISKSIZE (1ll << 40) /* 1 TB */

typedef unsigned int blkdev_id_t; /* device id is a uint */
typedef uint64_t sector_t;
#define PRIsctr PRIu64

typedef void (blkdev_aiocb_t)(int ret, void *argp);

struct blkdev {
  struct blkfront_dev *dev;
  struct blkfront_info info;
  struct mempool *reqpool;
  char nname[64];
  blkdev_id_t id;
#ifdef CONFIG_SELECT_POLL
  int fd;
#endif

  int exclusive;
  unsigned int refcount;
  struct blkdev *_next;
  struct blkdev *_prev;
};

struct _blkdev_req {
  struct mempool_obj *p_obj; /* reference to dependent memory pool object */
  struct blkdev *bd;
  struct blkfront_aiocb aiocb;
  sector_t sector;
  sector_t nb_sectors;
  int write;
  blkdev_aiocb_t *cb;
  void *cb_argp;
};

#define CAN_DETECT_BLKDEVS
unsigned int detect_blkdevs(blkdev_id_t ids_out[], unsigned int max_nb);
struct blkdev *open_blkdev(blkdev_id_t id, int mode);
void close_blkdev(struct blkdev *bd);
#define blkdev_refcount(bd) ((bd)->refcount)

int blkdev_id_parse(const char *id, blkdev_id_t *out);
#define blkdev_id_unparse(id, out, maxlen) \
     (snprintf((out), (maxlen), "%u", (id)))
#define blkdev_id_cmp(id0, id1) \
     ((id0) != (id1))
#define blkdev_id_cpy(dst, src) \
     ((dst) = (src))
#define blkdev_id(bd) ((bd)->id)
#define blkdev_ioalign(bd) blkdev_ssize((bd))

/**
 * Retrieve device information
 */
static inline sector_t blkdev_sectors(struct blkdev *bd)
{
  /* WORKAROUND: blkfront cannot handle > 1TB -> limit the disk size */
  if (((sector_t) bd->info.sectors * (sector_t) bd->info.sector_size) > MAX_DISKSIZE)
	return (MAX_DISKSIZE / (sector_t) bd->info.sector_size);
  return (sector_t) bd->info.sectors;
}
#define blkdev_ssize(bd) ((uint32_t) (bd)->info.sector_size)
#define blkdev_size(bd) (blkdev_sectors((bd)) * (sector_t) blkdev_ssize((bd)))
#define blkdev_avail_req(bd) mempool_free_count((bd)->reqpool)


/**
 * Async I/O
 *
 * Note: target buffer has to be aligned to device sector size
 */
void _blkdev_async_io_cb(struct blkfront_aiocb *aiocb, int ret);

#define blkdev_async_io_submit(bd) blkfront_aio_submit((bd)->dev)
#define blkdev_async_io_wait_slot(bd) blkfront_wait_slot((bd)->dev)

static inline int blkdev_async_io_nocheck(struct blkdev *bd, sector_t start, sector_t len,
                                          int write, void *buffer, blkdev_aiocb_t *cb, void *cb_argp)
{
  struct mempool_obj *robj;
  struct _blkdev_req *req;
  int ret;

  robj = mempool_pick(bd->reqpool);
  if (unlikely(!robj))
	return -EAGAIN; /* too many requests on queue */

  req = robj->data;
  req->p_obj = robj;

  req->aiocb.data = NULL;
  req->aiocb.aio_dev = bd->dev;
  req->aiocb.aio_buf = buffer;
  req->aiocb.aio_offset = (off_t) (start * blkdev_ssize(bd));
  req->aiocb.aio_nbytes = len * blkdev_ssize(bd);
  req->aiocb.aio_cb = _blkdev_async_io_cb;
  req->bd = bd;
  req->sector = start;
  req->nb_sectors = len;
  req->write = write;
  req->cb = cb;
  req->cb_argp = cb_argp;

 retry:
  ret = blkfront_aio_enqueue(&(req->aiocb), write);
  if (unlikely(ret == -EBUSY)) {
	blkdev_async_io_submit(bd);
	blkdev_async_io_wait_slot(bd); /* yields CPU */
	goto retry;
  }
  return ret;
}
#define blkdev_async_write_nocheck(bd, start, len, buffer, cb, cb_argp) \
	blkdev_async_io_nocheck((bd), (start), (len), 1, (buffer), (cb), (cb_argp))
#define blkdev_async_read_nocheck(bd, start, len, buffer, cb, cb_argp) \
	blkdev_async_io_nocheck((bd), (start), (len), 0, (buffer), (cb), (cb_argp))

static inline int blkdev_async_io(struct blkdev *bd, sector_t start, sector_t len,
                                  int write, void *buffer, blkdev_aiocb_t *cb, void *cb_argp)
{
	if (unlikely(write && !(bd->info.mode & (O_WRONLY | O_RDWR)))) {
		/* write access on non-writable device or read access on non-readable device */
		return -EACCES;
	}

	if (unlikely((len * blkdev_ssize(bd)) / PAGE_SIZE > BLKIF_MAX_SEGMENTS_PER_REQUEST)) {
		/* request too big -> cannot be handled with a single request */
		return -ENXIO;
	}

	if (unlikely(((uintptr_t) buffer) & ((uintptr_t) blkdev_ssize(bd) - 1))) {
		/* buffer is not aligned to device sector size */
		return -EINVAL;
	}

	return blkdev_async_io_nocheck(bd, start, len, write, buffer, cb, cb_argp);
}
#define blkdev_async_write(bd, start, len, buffer, cb, cb_argp)	  \
	blkdev_async_io((bd), (start), (len), 1, (buffer), (cb), (cb_argp))
#define blkdev_async_read(bd, start, len, buffer, cb, cb_argp)	  \
	blkdev_async_io((bd), (start), (len), 0, (buffer), (cb), (cb_argp))

#define blkdev_poll_req(bd) \
	blkfront_aio_poll((bd)->dev)

#ifdef CONFIG_SELECT_POLL
#define CAN_POLL_BLKDEV
#define blkdev_get_fd(bd) ((bd)->fd)
#endif /* CONFIG_SELECT_POLL */

/**
 * Sync I/O
 */
void _blkdev_sync_io_cb(int ret, void *argp);

struct _blkdev_sync_io_sync {
	struct semaphore sem;
	int ret;
};

static inline int blkdev_sync_io_nocheck(struct blkdev *bd, sector_t start, sector_t len,
                                             int write, void *target)
{
	struct _blkdev_sync_io_sync iosync;
	int ret;

	init_SEMAPHORE(&iosync.sem, 0);
 retry:
	ret = blkdev_async_io_nocheck(bd, start, len, write, target,
	                              _blkdev_sync_io_cb, &iosync);
	blkdev_async_io_submit(bd);
	if (unlikely(ret == -EAGAIN)) {
		/* try again, queue was full */
		blkdev_poll_req(bd);
		schedule();
		goto retry;
	}
	if (unlikely(ret == -EBUSY)) {
		blkdev_async_io_wait_slot(bd); /* yields CPU */
		goto retry;
	}
	if (unlikely(ret < 0))
		return ret;

	/* wait for I/O completion */
	while (trydown(&iosync.sem) == 0) {
		blkdev_poll_req(bd);
		schedule(); /* yield CPU */
	}

	return iosync.ret;
}
#define blkdev_sync_write_nocheck(bd, start, len, buffer)	  \
	blkdev_sync_io_nocheck((bd), (start), (len), 1, (buffer))
#define blkdev_sync_read_nocheck(bd, start, len, buffer)	  \
	blkdev_sync_io_nocheck((bd), (start), (len), 0, (buffer))

static inline int blkdev_sync_io(struct blkdev *bd, sector_t start, sector_t len,
                                 int write, void *target)
{
	if (unlikely(write && !(bd->info.mode & (O_WRONLY | O_RDWR)))) {
		/* write access on non-writable device or read access on non-readable device */
		return -EACCES;
	}

	if (unlikely((len * blkdev_ssize(bd)) / PAGE_SIZE > BLKIF_MAX_SEGMENTS_PER_REQUEST)) {
		/* request too big -> cannot be handled with a single request */
		return -ENXIO;
	}

	if (unlikely(((uintptr_t) target) & ((uintptr_t) blkdev_ssize(bd) - 1))) {
		/* buffer is not aligned to device sector size */
		return -EINVAL;
	}

	return blkdev_sync_io_nocheck(bd, start, len, write, target);
}
#define blkdev_sync_write(bd, start, len, buffer)	  \
	blkdev_sync_io((bd), (start), (len), 1, (buffer))
#define blkdev_sync_read(bd, start, len, buffer)	  \
	blkdev_sync_io((bd), (start), (len), 0, (buffer))

#endif /* _BLKDEV_H_ */
