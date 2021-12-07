#ifndef _dasint_h_
#define _dasint_h

#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <malloc.h>

typedef enum dasint_op_e {
	op_nop		// nothing
,	op_ldwi		// load integer word into reg
,	op_lda		// load argument qword into reg
,	op_ldf		// load next float into reg
,	op_mov		// R0 = R1
,	op_add		// R0 = R1 + R2
,	op_addbi	// R0 = R1 + BI
,	op_subbi	// R0 = R1 - BI
,	op_cmp		// FLAGS = R1 == R0
,	op_addf		// R0 = float(R1) + float(R2)
,	op_i2f		// R0 = int2float(R1)
,	op_call1	// R0 = call(addr,R1)
,	op_jewi		// if flags CP+=WI;
,	op_jnewi	// if !flags CP+=WI;
,	op_loopwi	// R0--, if R0!=0 CP+=WI;
,	op_ret		// RESULT = R0
} dasint_op_t;

typedef struct dasint_code_s {
	uint32_t *	code;
	uint32_t *	cs;
	size_t		capacity;
} dasint_code_t;

typedef int64_t (* call0_t)();
typedef int64_t (* call1_t)(int64_t);

void dasint_code_init ( dasint_code_t * ctx );
void dasint_code_free ( dasint_code_t * ctx );
uint32_t * dasint_code_reserve ( dasint_code_t * ctx, size_t dw );

uint32_t * dasint_label ( dasint_code_t * ctx );
void dasint_op_R0 ( dasint_code_t * ctx, uint32_t op, uint32_t R0);
void dasint_op_R0_WI ( dasint_code_t * ctx, uint32_t op, uint32_t R0, int16_t WI );
void dasint_op_R0_FLOAT ( dasint_code_t * ctx, uint32_t op, uint32_t R0, float f );
void dasint_op_R0_R1 ( dasint_code_t * ctx, uint32_t op, uint32_t R0, uint32_t R1 );
void dasint_op_R0_R1_BI ( dasint_code_t * ctx, uint32_t op, uint32_t R0, uint32_t R1, int8_t BI );
void dasint_op_R0_R1_ADDR ( dasint_code_t * ctx, uint32_t op, uint32_t R0, uint32_t R1, void * addr );
void dasint_op_R0_R1_R2 ( dasint_code_t * ctx, uint32_t op, uint32_t R0, uint32_t R1, uint32_t R2 );
void dasint_op_JUMP ( dasint_code_t * ctx, uint32_t op, uint32_t * label );
void dasint_op_R0_JUMP ( dasint_code_t * ctx, uint32_t op, uint32_t R0, uint32_t * label );

void dasint_disasm ( dasint_code_t * ctx );

int64_t dasint_eval ( uint32_t * cp, int64_t * ap );

#ifdef DASINT_IMPLEMENTATION

void dasint_code_init ( dasint_code_t * ctx ) {
	ctx->code = (uint32_t *) malloc(32*sizeof(uint32_t));
	ctx->cs = ctx->code;
	ctx->capacity = 32;
}

void dasint_code_free ( dasint_code_t * ctx ) {
	free(ctx->code);
}

uint32_t * dasint_code_reserve ( dasint_code_t * ctx, size_t dw ) {
	uint32_t * ncs = ctx->cs + dw;
	if ( (ncs - ctx->code) > ctx->capacity ) {
		size_t nc1 = ctx->capacity * 2;
		size_t nc2 = ctx->capacity + dw;
		ctx->capacity = nc1 < nc2 ? nc2 : nc1;
		uint32_t * ncode = realloc(ctx->code, ctx->capacity);
		ctx->cs = ctx->cs - ctx->code + ncode;
		ctx->code = ncode;
		ncs = ctx->cs + dw;
	}
	uint32_t * ccs = ctx->cs;
	ctx->cs = ncs;
	return ccs;
}

uint32_t * dasint_label ( dasint_code_t * ctx ) {
	return ctx->cs;
}

void dasint_op_R0 ( dasint_code_t * ctx, uint32_t op, uint32_t R0) {
	uint32_t * code = dasint_code_reserve(ctx, 1);
	*code = op | (R0<<8);
}

void dasint_op_R0_WI ( dasint_code_t * ctx, uint32_t op, uint32_t R0, int16_t WI ) {
	uint32_t * code = dasint_code_reserve(ctx, 1);
	*code = op | (R0<<8) | (((uint32_t)WI)<<16);
}

void dasint_op_R0_FLOAT ( dasint_code_t * ctx, uint32_t op, uint32_t R0, float f ) {
	uint32_t * code = dasint_code_reserve(ctx, 2);
	*code = op | (R0<<8);	code ++;
	*((float*)code) = f;
}

void dasint_op_R0_R1 ( dasint_code_t * ctx, uint32_t op, uint32_t R0, uint32_t R1 ) {
	uint32_t * code = dasint_code_reserve(ctx, 1);
	*code = op | (R0<<8) | (R1<<16);
}

void dasint_op_R0_R1_BI ( dasint_code_t * ctx, uint32_t op, uint32_t R0, uint32_t R1, int8_t BI ) {
	uint32_t * code = dasint_code_reserve(ctx, 1);
	*code = op | (R0<<8) | (R1<<16) | (((uint32_t)BI)<<24);
}

void dasint_op_R0_R1_ADDR ( dasint_code_t * ctx, uint32_t op, uint32_t R0, uint32_t R1, void * addr ) {
	uint32_t * code = dasint_code_reserve(ctx, 3);
	*code = op | (R0<<8) | (R1<<16);	code++;
	*((void **)code) = addr;
}

void dasint_op_R0_R1_R2 ( dasint_code_t * ctx, uint32_t op, uint32_t R0, uint32_t R1, uint32_t R2 ) {
	uint32_t * code = dasint_code_reserve(ctx, 1);
	*code = op | (R0<<8) | (R1<<16) | (R2<<24);
}

void dasint_op_JUMP ( dasint_code_t * ctx, uint32_t op, uint32_t * label ) {
	uint32_t * code = dasint_code_reserve(ctx, 1);
	ptrdiff_t diff = label - code - 1;
	assert ( diff<=INT16_MAX && diff>=INT16_MIN );
	int16_t WI = diff;
	*code = op | (((uint32_t)diff)<<16);
}

void dasint_op_R0_JUMP ( dasint_code_t * ctx, uint32_t op, uint32_t R0, uint32_t * label ) {
	uint32_t * code = dasint_code_reserve(ctx, 1);
	ptrdiff_t diff = label - code - 1;
	assert ( diff<=INT16_MAX && diff>=INT16_MIN );
	int16_t WI = diff;
	*code = op | (R0<<8) | (((uint32_t)diff)<<16);
}

int64_t dasint_eval ( uint32_t * cp, int64_t * ap ) {
	int64_t	r[32];
	uint32_t flags = 0;
	for ( ;; ) {
		uint32_t inst = *cp++;
#define R0		((inst  >>  8) & 0xff)
#define R1		((inst  >> 16) & 0xff)
#define R2		((inst  >> 24) & 0xff)
#define WI		(((int32_t)inst) >> 16)
#define BI		(((int32_t)inst) >> 24)
		switch ( inst & 0xff ) {
			case op_nop:	break;
			case op_ldf:	r[R0] = *cp++;	break;
			case op_ldwi:	r[R0] = WI;	break;
			case op_lda:	r[R0] = ap[R1];	break;
			case op_mov:	r[R0] = r[R1];	break;
			case op_add:	r[R0] = r[R1] + r[R2];	break;
			case op_addbi:	r[R0] = r[R1] + BI; break;
			case op_subbi:	r[R0] = r[R1] - BI; break;
			case op_cmp:	flags = r[R0] == r[R1];	break;
			case op_addf:	*((float*)(r+R0)) = *((float*)(r+R1)) + *((float*)(r+R2));	break;
			case op_i2f:	*((float*)(r+R0)) = (float) r[R1]; break;
			case op_call1:	{ call1_t c1 = *((void **)cp); cp += 2; r[R0] = (*c1)(r[R1]); }	break;
			case op_jewi:	if (flags ) cp += WI; break;
			case op_jnewi:	if (!flags ) cp += WI; break;
			case op_loopwi:	if ( --(r[R0]) ) cp += WI; break;
			case op_ret:	return r[R0];
			default:
				assert(0 && "unsupported instruction");
		}
	}
#undef R0
#undef R1
#undef R2
#undef WI
#undef BI
}

#define OP_R0		(1<<0)
#define OP_R1		(1<<1)
#define OP_R2		(1<<2)
#define OP_BI		(1<<3)
#define OP_WI		(1<<4)
#define OP_FLOAT	(1<<5)
#define OP_ADDR		(1<<6)
#define OP_ARG		(1<<7)
#define OP_WI_CADDR	(1<<8)

struct dasint_dis_s {
	int	opCode;
	const char * opCodeName;
	int mask;
} dasint_disasm_table [] = {
	op_nop,		"nop",			0,
	op_ldwi,	"ldwi",			OP_R0 | OP_WI,
	op_lda,		"lda",			OP_R0 | OP_ARG,
	op_ldf,		"ldf",			OP_R0 | OP_FLOAT,
	op_mov,		"mov",			OP_R0 | OP_R1,
	op_add,		"add",			OP_R0 | OP_R1 | OP_R2,
	op_addbi,	"addbi",		OP_R0 | OP_R1 | OP_BI,
	op_subbi,	"subbi",		OP_R0 | OP_R1 | OP_BI,
	op_cmp,		"cmp",			OP_R0 | OP_R1,
	op_addf,	"addf",			OP_R0 | OP_R1 | OP_R2,
	op_i2f,		"i2f",			OP_R0 | OP_R1,
	op_call1,	"call1",		OP_R0 | OP_R1 | OP_ADDR,
	op_jewi,	"jewi",			OP_WI_CADDR,
	op_jnewi,	"jnewi",		OP_WI_CADDR,
	op_loopwi,	"loopwi",		OP_R0 | OP_WI_CADDR,
	op_ret,		"ret",			OP_R0,
	-1,			"",				0,
};

void dasint_disasm ( dasint_code_t * ctx ) {
	uint32_t * cp = ctx->code;
	while ( cp != ctx->cs ) {
		uint32_t inst = *cp;
		int op = inst & 0xff;
		struct dasint_dis_s * d = dasint_disasm_table;
		while ( d->opCode!=-1 && d->opCode!=op ) d ++;
		if ( d->opCode==-1 ) {
			printf("%8p\tinvalid opcode %i\n", cp, op );
			return;
		}
		printf("%8p\t%s\t", cp, d->opCodeName);
		cp ++;
		if ( d->mask & OP_R0 ) {
			int R0 = (inst >> 8) & 0xff;
			printf("r%i\t", R0);
		}
		if ( d->mask & OP_R1 ) {
			int R1 = (inst >> 16) & 0xff;
			printf("r%i\t", R1);
		}
		if ( d->mask & OP_ARG ) {
			int R1 = (inst >> 16) & 0xff;
			printf("a%i\t", R1);
		}
		if ( d->mask & OP_R2 ) {
			int R2 = (inst >> 24) & 0xff;
			printf("r%i\t", R2);
		}
		if ( d->mask & OP_WI ) {
			int WI = (((int32_t)inst) >> 16);
			printf("%i\t", WI);
		}
		if ( d->mask & OP_WI_CADDR ) {
			int WI = (((int32_t)inst) >> 16);
			printf("%p\t", cp + WI);
		}
		if ( d->mask & OP_BI ) {
			int BI = (((int32_t)inst) >> 24);
			printf("%i\t", BI);
		}
		if ( d->mask & OP_FLOAT ) {
			printf("%f\t", *(float*)cp);	cp ++;
		}
		if ( d->mask & OP_ADDR ) {
			printf("%p\t", *(void **)cp);	cp += 2;
		}
		printf("\n");
	}
}

#undef OP_R0
#undef OP_R1
#undef OP_R2
#undef OP_BI
#undef OP_WI
#undef OP_FLOAT
#undef OP_ADDR
#undef OP_ARG



#endif

#endif

