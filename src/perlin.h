#ifndef _PERLIN_H
#define _PERLIN_H

#include "vector3.h"

// As we aren't using profile guided optimisation it helps to 
// manually force inlines for code we know is very expensive
#ifdef _MSC_VER //MSVC
	#if ((_MSC_VER >= 1500) && (_MSC_VER < 1700)) //1600-1700 is MSVC2010
		#define PERLIN_INLINE __forceinline 
	#endif
#else //MINGW-GCC/GCC
	#define PERLIN_INLINE __attribute__((always_inline))
#endif


extern PERLIN_INLINE double noise(const double x, const double y, const double z );
static PERLIN_INLINE double noise(const vector3d p) {
	return noise(p.x, p.y, p.z);
}
void perlin_init();

#endif /* _PERLIN_H */
