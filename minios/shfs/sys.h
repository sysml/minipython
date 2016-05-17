#ifndef _SYS_H_
#define _SYS_H_

#include <mini-os/os.h>
#include <mini-os/types.h>
#include <mini-os/xmalloc.h>
#include <mini-os/lib.h>
#include <mini-os/kernel.h>

#define target_malloc(align, size) \
  ((void *) _xmalloc((size), (align)))
#define target_free(ptr) \
  xfree(ptr)

/* semaphores */
#include <mini-os/semaphore.h>
typedef struct semaphore sem_t;

/* shutdown */
#include <mini-os/shutdown.h>

#define TARGET_SHTDN_POWEROFF \
  SHUTDOWN_poweroff
#define TARGET_SHTDN_REBOOT \
  SHUTDOWN_reboot
#define TARGET_SHTDN_SUSPEND \
  SHUTDOWN_suspend

#define target_suspend() \
  kernel_suspend()

#define target_halt() \
  kernel_shutdown(SHUTDOWN_poweroff)

#define target_reboot() \
  kernel_shutdown(SHUTDOWN_reboot)

#define target_crash() \
  kernel_shutdown(SHUTDOWN_crash)

#define target_init() \
	do {} while(0)
#define target_exit() \
	do {} while(0)

/*
#ifdef CONFIG_ARM
#define target_now_ns() ({ \
	uint64_t r;						\
	struct timeval now;					\
	gettimeofday(&now, NULL);				\
	r = now.tv_usec * 1000 + now.tv_sec * 1000000000l;	\
	r; })
#else
*/
#define target_now_ns() (NOW())
/*
#endif
*/

#endif /* _SYS_H_ */
