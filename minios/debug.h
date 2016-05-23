#ifndef _DEBUG_H_
#define _DEBUG_H_

//#include <target/sys.h>
#include <sys/time.h>
#include <inttypes.h>

extern struct timeval __debug_tsref;

#define init_debug() (gettimeofday(&__debug_tsref, NULL))

#ifndef STRINGIFY
#define STRINGIFY(s) #s
#endif
#ifndef XSTRINGIFY
#define XSTRINGIFY(s) STRINGIFY(s)
#endif

#ifdef ENABLE_DEBUG
#ifdef __MINIOS__
#if (defined __x86_64__ || defined __x86_32__)
#define CONFIG_DEBUG_CALLDEPTH
#endif
#define __debug_printf(fmt, ...) printk((fmt), ##__VA_ARGS__)
#else
#define __debug_printf(fmt, ...) fprintf(stderr, (fmt), ##__VA_ARGS__)
#endif /* __MINIOS__ */

#if defined __MINIOS__ && defined __x86_64__
#define __get_bp() \
	({ \
		unsigned long bp; \
		asm("movq %%rbp, %0":"=r"(bp)); \
		bp; \
	})
#elif defined __MINIOS__ && defined __x86_32__
#define __get_bp() \
	({ \
		unsigned long bp; \
		asm("movq %%ebp, %0":"=r"(bp)); \
		bp; \
	})
#endif

#if defined __MINIOS__ && (defined __x86_64__ || defined __x86_32__)
/**
 * get_caller(): returns calling address for the current function
 *
 * Note: On non-x86 platforms, 0xBADC0DED is returned
 */
#define get_caller()	  \
	({ \
		unsigned long *frame = (void *) __get_bp(); \
		frame[1]; \
	})

/**
 * get_calldepth(): returns the current number of invoked function calls
 *
 * Note: On non-x86 platforms, 0x0 is returned always
 */
#define get_calldepth()	  \
	({ \
		unsigned long depth = 0;		      \
		unsigned long *frame = (void *) __get_bp();   \
		while (frame[0]) {			      \
			++depth;			      \
			frame = (void *) frame[0];	      \
		}					      \
		depth;					      \
	})
#else
#define get_caller() (0xBADC0DED)
#define get_calldepth() (0x0)
#endif

/**
 * printd(): prints a debug message to stdout
 */
#ifdef CONFIG_DEBUG_CALLDEPTH
#define printd(fmt, ...)						\
	do {								\
	    struct timeval now;					\
	    uint64_t mins, secs, usecs;				\
	    unsigned long cd = get_calldepth();			\
	    								\
	    gettimeofday(&now, NULL);					\
	    if (now.tv_usec < __debug_tsref.tv_usec) {			\
	        now.tv_usec += 1000000l;				\
	        now.tv_sec--;						\
	    }								\
	    usecs = (now.tv_usec - __debug_tsref.tv_usec);		\
	    								\
	    secs  = (now.tv_sec - __debug_tsref.tv_sec);		\
	    secs += usecs / 1000000l;					\
	    usecs %= 1000000l;						\
	    mins = secs / 60;						\
	    secs %= 60;						\
	    								\
	    __debug_printf("[%"PRIu64"m%02"PRIu64".%06"PRIu64"s] ",	\
			     mins, secs, usecs);			\
	    while (cd--)						\
		__debug_printf("-");					\
	    __debug_printf(" %s():%d: "				\
			   fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
	} while(0)
#else
#define printd(fmt, ...)						\
	do {								\
	    struct timeval now;					\
	    uint64_t mins, secs, usecs;				\
	    								\
	    gettimeofday(&now, NULL);					\
	    if (now.tv_usec < __debug_tsref.tv_usec) {			\
	        now.tv_usec += 1000000l;				\
	        now.tv_sec--;						\
	    }								\
	    usecs = (now.tv_usec - __debug_tsref.tv_usec);		\
	    								\
	    secs  = (now.tv_sec - __debug_tsref.tv_sec);		\
	    secs += usecs / 1000000l;					\
	    usecs %= 1000000l;						\
	    mins = secs / 60;						\
	    secs %= 60;						\
	    								\
	    __debug_printf("[%"PRIu64"m%02"PRIu64".%06"PRIu64"s] %s:%4d: %s(): " \
			   fmt, mins, secs, usecs,			\
			   __FILE__, __LINE__, __FUNCTION__,		\
			   ##__VA_ARGS__);				\
	} while(0)
#endif

#else /* ENABLE_DEBUG */

#define printd(fmt, ...) do {} while(0)
#define get_caller() (0xDEADC0DE)
#define get_calldepth() (0x0)

#endif /* ENABLE_DEBUG */

#define tprobe_start(x) \
	uint64_t x; \
	x = target_now_ns();
#define tprobe_end(x) \
	x = target_now_ns() - x; \
	printk(STRINGIFY(x)": %01"PRIu64".%09"PRIu64"s\n", x / 1000000000ull, x % 1000000000ull);

#endif /* _DEBUG_H_ */
