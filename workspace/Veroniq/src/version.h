#ifndef VERSION_H_
#define VERSION_H_

#define VERSION_MAJOR       0
#define VERSION_MIDDLE      0
#define VERSION_MINOR       1

#define BUILD_NUMBER      310

/* Version macors to use */

#define VERSION_STR         _VERSION_STR_(_VERSION_CONCAT_(VERSION_MAJOR,VERSION_MIDDLE,VERSION_MINOR))
#define BUILD_NUMBER_STR    _VERSION_STR_(BUILD_NUMBER)
#define BUILD_DATE          __DATE__", "__TIME__

/* Some internal macros */
#define __VERSION_CONCAT_(a,b,c) a ## . ## b ## . ## c
#define __VERSION_STR_(v) #v
#define _VERSION_CONCAT_(a,b,c) __VERSION_CONCAT_(a,b,c)
#define _VERSION_STR_(v) __VERSION_STR_(v)


#endif /*VERSION_H_*/
