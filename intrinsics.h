
#ifdef HAS_SSE2
#include <xmmintrin.h>
#endif

#ifdef HAS_SSE2

#define MMAX(a, b) (_mm_cvtss_f32(_mm_max_ss(_mm_set_ss(a), _mm_set_ss(b))))
#define MMIN(a, b) (_mm_cvtss_f32(_mm_min_ss(_mm_set_ss(a), _mm_set_ss(b))))
#define MSQRT(a) (_mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(a))))

#else

inline float mmin_impl(float a, float b)
{
	return a < b ? a : b;
}
inline float mmax_impl(float a, float b)
{
	return a > b ? a : b;
}

#define MMAX(a, b) (mmax_impl(a, b))
#define MMIN(a, b) (mmin_impl(a, b))
#define MSQRT(a) (sqrtf(a))

#endif

