#define NTHREAD 64
#define MAXDMA	512
#define ATHREAD

#ifdef ATHREAD
 #define USE_SIMD
// #define REG_COM
#endif

struct _swarg {
	long	n;
	double *arr;
	int		ncoef;
	double *coef;
	double *result;
};
