#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "swlu.h"

#define N 100000000
#define NCOEF 10
#define CLOCKRATE 1.45e9

double coef[NCOEF] = {19, 17, 15, 13, 11, 9, 7, 5, 3, 1};

static inline unsigned long rpcc() {
	unsigned long time;
	asm("rtc %0": "=r" (time) : );
	return time;
}

int main() {
	long			i;
	double			result, *arr;
	unsigned long	tst_rpcc, ted_rpcc;
	clock_t			tst_time, ted_time;
	struct timeval	tst_gtod, ted_gtod;
    
    swlu_debug_init();
    
	arr = (double *)malloc(N * sizeof(double));
	for (i = 0; i < N; i++) {
		arr[i] = 1.0;
	}
	
	tst_rpcc = rpcc();
	tst_time = clock();
	gettimeofday(&tst_gtod, NULL);

	sum(N, arr, NCOEF, coef, &result);

	ted_rpcc = rpcc();
	ted_time = clock();
	gettimeofday(&ted_gtod, NULL);
	
	printf("Sum of array is : %.2lf\n", result);
	printf("Time(rpcc) : %.2lf ms\n", (double)(ted_rpcc - tst_rpcc) * 1000 / CLOCKRATE);
	printf("Time(time) : %.2lf ms\n", (double)(ted_time - tst_time) * 1000 / CLOCKS_PER_SEC);
	printf("Time(gtod) : %.2lf ms\n", (ted_gtod.tv_sec - tst_gtod.tv_sec) * 1000 + \
		(double)(ted_gtod.tv_usec - tst_gtod.tv_usec) / 1000.0);

	free(arr);

	return 0;
}
