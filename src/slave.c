#include <slave.h>
#include "swarg.h"

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
#endif

__thread_local volatile unsigned long get_reply, put_reply;
__thread_local volatile int my_id;

void wait_reply(volatile unsigned long *reply, int m) {
	while(*reply != m) {};
}

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
	
	// ------------ Get Info ------------ //
	
	my_id = athread_get_id(-1);
	get_reply = 0;
	athread_get(PE_MODE,
                marg,
                &sarg,
                sizeof(struct _swarg),
                &get_reply,
                0, 0, 0);
	//while(get_reply != 1);
	wait_reply(&get_reply, 1);
	
	double coef[sarg.ncoef];
	athread_get(PE_MODE,
                sarg.coef,
                coef,
                sarg.ncoef * sizeof(double),
                &get_reply,
                0, 0, 0);
	wait_reply(&get_reply, 2);
	
	// ---------- Load Balance ---------- //
	
	long load = sarg.n / NTHREAD;
	long rest = sarg.n % NTHREAD;
	long stid = (my_id < rest) ? my_id * (load + 1) : my_id * load + rest;
	if (my_id < rest) load++;
	long edid = stid + load;
	
	// ---------- Main Compute ---------- //
	
#ifndef USE_SIMD
	double localbuf[MAXDMA];
#else
	doublev4 sumv4 = 0;
	double localbuf[MAXDMA + 4];
#endif
	double sum = 0;
	long batch = load;
	while (batch > 0) {
		if (batch > MAXDMA)
			batch = MAXDMA;
		get_reply = 0;
		athread_get(PE_MODE,
					sarg.arr + stid,
					localbuf,
					batch * sizeof(double),
					&get_reply,
					0, 0, 0);
		wait_reply(&get_reply, 1);
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
	while (divisor <= 8) {
		if (my_id % divisor == quotient) {
			REG_PUTR(sum, colid - quotient);
		}
		if (my_id % divisor == 0) {
			REG_GETR(recvval);
			sum += recvval;
		}
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
		divisor  *= 2;
		quotient *= 2;
	}
	
	// --------- Put Result Reg --------- //
	
	if (my_id == 0)  {
		put_reply = 0;
		athread_put(PE_MODE,
					&sum,
					sarg.result,
					sizeof(double),
					&put_reply,
					0, 0);
		wait_reply(&put_reply, 1);
	}

	return;
#endif
	
	// -------- Put Result Noreg -------- //
	
	put_reply = 0;
	athread_put(PE_MODE,
				&sum,
				sarg.result + my_id,
				sizeof(double),
				&put_reply,
				0, 0);
	wait_reply(&put_reply, 1);
}
