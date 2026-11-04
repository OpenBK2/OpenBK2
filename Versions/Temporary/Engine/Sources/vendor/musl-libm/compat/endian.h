/* libm.h reads __BYTE_ORDER only to pick a long double layout, and only
   little-endian x86 and x86-64 are built. glibc has a real endian.h; MSVC has
   none. */
#if defined( __GNUC__ ) && !defined( _MSC_VER )
#include_next <endian.h>
#else
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
