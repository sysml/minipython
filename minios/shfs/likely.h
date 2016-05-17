#ifndef _LIKELY_H_
#define _LIKELY_H_

#ifndef likely
#define likely(_x)    (__builtin_expect((!!(_x)), 1))
#endif

#ifndef unlikely
#define unlikely(_x)  (__builtin_expect((!!(_x)), 0))
#endif

#endif /* _LIKELY_H_ */
