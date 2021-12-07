
#define DASINT_IMPLEMENTATION
#include "dasint.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>

#include "profile.h"

void fib ( dasint_code_t * ctx ) {
	dasint_code_init(ctx);
	dasint_op_R0_WI(ctx, op_ldwi, 0, 0);				//	ldwi	r0,	0		//	last
	dasint_op_R0_WI(ctx, op_ldwi, 1, 1);				//	ldwi	r1,	1		//	cur
	dasint_op_R0_R1(ctx, op_lda,  3, 0);				//	lda		r3,	0		//	n			// arg0
	dasint_op_R0_R1_BI(ctx, op_subbi,  3, 3, 1);		//	sub		r3, r3, 1	//	n-1
	uint32_t * label_for = dasint_label(ctx);			//	.for:
	dasint_op_R0_R1(ctx, op_mov,  4, 1);				//	mov		r4,	r1
	dasint_op_R0_R1_R2(ctx, op_add, 1, 1, 0);			//	add		r1,	r1,	r0
	dasint_op_R0_R1(ctx, op_mov,  0, 4);				//	mov		r0, r4
	dasint_op_R0_JUMP(ctx, op_loopwi, 3, label_for);	//	loop	r3, .for
	dasint_op_R0(ctx, op_ret, 1);						//	ret		r1
}

int64_t ext_sin ( int64_t arg ) {
	float a = *((float *)(&arg));
	float r = sinf(a);
	return *((uint32_t*)&r);
}

int64_t ext_cos ( int64_t arg ) {
	float a = *((float *)(&arg));
	float r = cosf(a);
	return *((uint32_t*)&r);
}

void extcall ( dasint_code_t * ctx ) {
	dasint_code_init(ctx);
	dasint_op_R0_FLOAT(ctx, op_ldf, 0, 1.23);				// t = 1.23
	dasint_op_R0_WI(ctx, op_ldwi, 1, 1000);					// r1 = 100
	uint32_t * label_for_1 = dasint_label(ctx);				//	.for_1:
	dasint_op_R0_WI(ctx, op_ldwi, 2, 0);					// i = 0
	dasint_op_R0_R1(ctx, op_lda, 3, 0);						// r3 = n	// arg0
	uint32_t * label_for_2 = dasint_label(ctx);				//	.for_2:
	dasint_op_R0_R1(ctx, op_i2f, 4, 2);						// r4 = float(i)
	dasint_op_R0_R1_ADDR(ctx, op_call1, 5, 4, &ext_sin);	// r5 = sin(r4)
	dasint_op_R0_R1_ADDR(ctx, op_call1, 6, 4, &ext_cos);	// r6 = cos(r4)
	dasint_op_R0_R1_R2(ctx, op_addf, 7, 5, 6);				// r7 + r4 + r5 = sin(float(i)) + cos(float(i))
	dasint_op_R0_R1_R2(ctx, op_addf, 0, 0, 7);				// addf	r0, r0, r7
	dasint_op_R0_R1_BI(ctx, op_addbi,  2, 2, 1);			// addbi	r2, r2, 1		// i++
	dasint_op_R0_R1(ctx, op_cmp,  2, 3);					// if i != n
	dasint_op_JUMP(ctx, op_jnewi, label_for_2);				//    goto for2
	dasint_op_R0_JUMP(ctx, op_loopwi, 1, label_for_1);		// loop for_1
	dasint_op_R0(ctx, op_ret, 0);							// ret r0		// t
}

void test_fib() {
	double msec_min = 100500.;
	int64_t t1 = ref_time_ticks ();
	int FIBI = 6511134;
	dasint_code_t ctx;
	fib(&ctx);
	int64_t res = 0;
	int64_t rest = 0;
	for ( int i=0; i!=20; ++i ) {
		uint64_t args[1];
		args[0] = FIBI;
		int64_t t0 = ref_time_ticks ();
		int64_t result = dasint_eval(ctx.code, args);
		double msec = get_time_usec(t0) / 1000000.0;
		assert(result==1781508648);
		res += result;
		rest = result;
		if ( msec<msec_min ) msec_min = msec;
	}
	dasint_code_free(&ctx);
	printf("test took %.1f sec\n", ((double)get_time_usec(t1)) / 1000000.0 );
	printf("fibI: %.6f msec\n", ((double)(msec_min)) );
	printf("rest = %i // %i\n", (int)rest, (int)res);
}

void test_extcall() {
	double msec_min = 100500.;
	int64_t t1 = ref_time_ticks ();
	float res = 0;
	float rest = 0;
	dasint_code_t ctx;
	extcall(&ctx);
	for ( int i=0; i!=20; ++i ) {
		uint64_t args[1];
		args[0] = 1000;
		int64_t t0 = ref_time_ticks ();
		int64_t result = dasint_eval(ctx.code, args);
		float tresult = *((float *)&result);
		double msec = get_time_usec(t0) / 1000000.0;
		res += tresult;
		rest = tresult;
		if ( msec<msec_min ) msec_min = msec;
	}
	dasint_code_free(&ctx);
	printf("test took %.1f sec\n", ((double)get_time_usec(t1)) / 1000000.0 );
	printf("extcall: %.6f msec\n", ((double)(msec_min)) );
	printf("res = %f // %f\n", rest, res);
}

int main(int argc, char **argv) {
	// test_fib();
	test_extcall();
	return 0;
}