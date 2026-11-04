/* Stands in for musl's internal features.h, which libm needs only for the
   `hidden` visibility keyword. Empty is fine here: the library is static, and
   CMakeLists.txt sets hidden visibility for the whole target instead.

   gcc and clang look this file up for glibc's own headers too (math.h includes
   <features.h>), so it hands those on to the real one. MSVC has no such
   header. */
#if defined( __GNUC__ ) && !defined( _MSC_VER )
#include_next <features.h>
#endif

#ifndef hidden
#define hidden
#endif
