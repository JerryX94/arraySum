#include <stdio.h>
#include "swarg.h"
#include "dma_macros.h"

#ifdef USE_SIMD
 #include <simd.h>
#endif

#ifdef REG_COM
 #define REG_PUTR(var,dest) \
  asm volatile ("putr %0,%1\n"::"r"(var),"r"(dest):"memory")
 #define REG_GETR(var) \
  asm volatile ("getr %0\n":"=r"(var)::"memory")
 #define REG_PUTC(var,dest) \
  asm volatile ("putc %0,%1\n"::"r"(var),"r"(dest):"memory")
 #define REG_GETC(var) \
  asm volatile ("getc %0\n":"=r"(var)::"memory")
 #define REG_SYNR(mask) \
  asm volatile ("synr %0"::"r"(mask))
 #define REG_SYNC(mask) \
  asm volatile ("sync %0"::"r"(mask))
#endif

__thread_local volatile int my_id;

#ifndef USE_SIMD
double polynomial_s(double x, int ncoef, double *coef) {
	int i;
	double y = 0;
	double p = 1.0;
	for (i = 0; i < ncoef; i++) {
		y += p * coef[i];
		p *= x;
	}
	return y;
}
#else
doublev4 polynomial_v4(doublev4 x, int ncoef, double *coef) {
	int i;
	doublev4 y = 0;
	doublev4 p = 1.0;
	for (i = 0; i < ncoef; i++) {
		y = y + p * coef[i];
		p = p * x;
	}
	return y;
}
#endif

void slave_sumsw(struct _swarg *marg) {
	long i;
	struct _swarg sarg;
	
	dma_init();
	
	// ------------ Get Info ------------ //
	
	my_id = athread_get_id(-1);
	pe_get(marg, &sarg, sizeof(struct _swarg));
	dma_syn();
	
	double coef[sarg.ncoef];
	pe_get(sarg.coef, coef, sarg.ncoef * sizeof(double));
	dma_syn();
	
	// ---------- Load Balance ---------- //
	
	long load = sarg.n / NTHREAD;
	long rest = sarg.n % NTHREAD;
	long stid = (my_id < rest) ? my_id * (load + 1) : my_id * load + rest;
	if (my_id < rest) load++;
	long edid = stid + load;
	
	// ---------- Main Compute ---------- //
	
#ifndef USE_SIMD
	double localbuf[MAXDMA];
	if (my_id == 0) printf("SIMD    OFF\n");
#else
	doublev4 sumv4 = 0;
	double localbuf[MAXDMA + 4];
	if (my_id == 0) printf("SIMD    ON\n");
#endif
	double sum = 0;
	long batch = load;
	while (batch > 0) {
		if (batch > MAXDMA)
			batch = MAXDMA;
		pe_get(sarg.arr + stid, localbuf, batch * sizeof(double));
		dma_syn();
#ifndef USE_SIMD
		for (i = 0; i < batch; i++) {
			sum += polynomial_s(localbuf[i], sarg.ncoef, coef);
		}
#else
		doublev4 tmpv4;
		if (batch % 4 != 0) {
			localbuf[batch] = 0;
			localbuf[batch + 1] = 0;
			localbuf[batch + 2] = 0;
			localbuf[batch + 3] = 0;
		}
		for (i = 0; i < batch; i += 4) {
			simd_load(tmpv4, &localbuf[i]);
			sumv4 = sumv4 + polynomial_v4(tmpv4, sarg.ncoef, coef);
		}
#endif
		stid += batch;
		batch = edid - stid;
	}
	
#ifdef USE_SIMD
	simd_store(sumv4, localbuf);
	sum = localbuf[0] + localbuf[1] + localbuf[2] + localbuf[3];
#endif

	// ----- Register Communication ----- //

#ifdef REG_COM
	double recvval	= 0;
	int divisor		= 2;
	int quotient	= 1;
	int rowid		= my_id / 8;
	int colid		= my_id % 8;
	if (my_id == 0) printf("REG_COM ON\n");
	while (divisor <= 8) {
		if (my_id % divisor == quotient) {
			REG_PUTR(sum, colid - quotient);
		}
		if (my_id % divisor == 0) {
			REG_GETR(recvval);
			sum += recvval;
		}
		REG_SYNR(0xff);
		divisor  *= 2;
		quotient *= 2;
	}
	while (divisor <= NTHREAD) {
		if (my_id % divisor == quotient) {
			REG_PUTC(sum, rowid - quotient / 8);
		}
		if (my_id % divisor == 0) {
			REG_GETC(recvval);
			sum += recvval;
		}
		REG_SYNC(0xff);
		divisor  *= 2;
		quotient *= 2;
	}
	
	// --------- Put Result Reg --------- //
	
	if (my_id == 0)  {
		pe_put(sarg.result, &sum, sizeof(double));
		dma_syn();
	}

	return;
#endif
	
	// -------- Put Result Noreg -------- //
	
	if (my_id == 0) printf("REG_COM OFF\n");
	pe_put(sarg.result + my_id, &sum, sizeof(double));
	dma_syn();
}
