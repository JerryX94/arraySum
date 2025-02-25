#include <stdio.h>
#include "swarg.h"

#ifdef ATHREAD
 #include <athread.h>
extern void SLAVE_FUN(sumsw)(struct _swarg *);
#endif

double polynomial(double x, int ncoef, double *coef) {
	int i;
	double y = 0;
	double p = 1.0;
	for (i = 0; i < ncoef; i++) {
		y += p * coef[i];
		p *= x;
	}
	return y;
}

void sum(long n, double *arr, int ncoef, double *coef, double *result) {
	long i;
	
#ifndef ATHREAD
	printf("ATHREAD OFF\n");
	*result = 0;
	for (i = 0; i < n; i++) {
		*result += polynomial(arr[i], ncoef, coef);
	}
#else
	printf("ATHREAD ON\n");
	athread_init();
	struct _swarg arg;
	arg.n		= n;
	arg.arr		= arr;
	arg.ncoef	= ncoef;
	arg.coef	= coef;
 #ifndef REG_COM
	double tmp[NTHREAD] = {0};
	arg.result	= tmp;
	
	athread_spawn(sumsw, &arg);
	athread_join();
	
	*result = 0;
	for (i = 0; i < NTHREAD; i++) {
		*result += tmp[i];
	}
 #else
	arg.result = result;
	athread_spawn(sumsw, &arg);
	athread_join();
 #endif
	athread_halt();
#endif
}
