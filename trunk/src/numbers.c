/*-
 * Copyright (c) 2005-2014 Michael Scholz <mi-scholz@users.sourceforge.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This product includes software written by Eric Young (eay@cryptsoft.com).
 *
 * @(#)numbers.c	1.164 1/23/14
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

/* Required for C99 prototypes (log2, trunc, clog10 etc)! */
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE 1
#endif

#include "fth.h"
#include "utils.h"

static FTH	llong_tag;
static FTH	float_tag;

#if HAVE_COMPLEX
static FTH	complex_tag;
#endif

#if HAVE_BN
static FTH	bignum_tag;
static FTH	ratio_tag;
static BIGNUM	fth_bn_zero;
static BIGNUM	fth_bn_one;
static FRatio	fth_rt_zero;

#if defined(HAVE_OPENSSL_ERR_H)
#include <openssl/err.h>
#endif

#define BN_ERROR_THROW() do {						\
	char _e_[130];							\
									\
	ERR_load_crypto_strings();					\
	ERR_error_string(ERR_get_error(), _e_);				\
	ERR_free_strings();						\
	fth_throw(FTH_BIGNUM_ERROR, "%s: %s", RUNNING_WORD(), _e_);	\
} while (0)

#define BN_CHECK(Bn) do {						\
	if ((Bn) == 0)							\
		BN_ERROR_THROW();					\
} while (0)

#define BN_CHECKP(Bn) do {						\
	if ((Bn) == NULL)						\
		BN_ERROR_THROW();					\
} while (0)
#endif				/* HAVE_BN */

#define FTH_MATH_ERROR_THROW(Desc)					\
	fth_throw(FTH_MATH_ERROR, "%s: %s", RUNNING_WORD(), Desc)

#if !defined(HAVE_LOG2)
static double	log2(double x);
#endif
#if !defined(HAVE_RINT)
static ficlFloat fth_round(ficlFloat r);
#endif

static ficlFloat fth_log(ficlFloat x);
static ficlFloat fth_log10(ficlFloat x);
static ficlFloat fth_log2(ficlFloat x);
static FTH	make_object_number_type(const char *name, 
		    fobj_t type, int flags);

static void	ficl_to_s(ficlVm *vm);

static FTH	ll_inspect(FTH self);
static FTH	ll_to_string(FTH self);
static FTH	ll_copy(FTH self);
static FTH	ll_equal_p(FTH self, FTH obj);

static void	ficl_bignum_p(ficlVm *vm);
static void	ficl_complex_p(ficlVm *vm);
static void	ficl_even_p(ficlVm *vm);
static void	ficl_exact_p(ficlVm *vm);
static void	ficl_fixnum_p(ficlVm *vm);
static void	ficl_inexact_p(ficlVm *vm);
static void	ficl_integer_p(ficlVm *vm);
static void	ficl_number_p(ficlVm *vm);
static void	ficl_odd_p(ficlVm *vm);
static void	ficl_llong_p(ficlVm *vm);
static void	ficl_prime_p(ficlVm *vm);
static void	ficl_ratio_p(ficlVm *vm);
static void	ficl_unsigned_p(ficlVm *vm);
static void	ficl_ullong_p(ficlVm *vm);

static void	ficl_make_long_long(ficlVm *vm);
static void	ficl_make_ulong_long(ficlVm *vm);

static void	ficl_frandom(ficlVm *vm);
static void	ficl_rand_seed_ref(ficlVm *vm);
static void	ficl_rand_seed_set(ficlVm *vm);
static void	ficl_random(ficlVm *vm);
static ficlFloat next_rand(void);

static void	ficl_d_dot(ficlVm *vm);
static void	ficl_d_dot_r(ficlVm *vm);
static void	ficl_dot_r(ficlVm *vm);
static void	ficl_u_dot_r(ficlVm *vm);
static void	ficl_ud_dot(ficlVm *vm);
static void	ficl_ud_dot_r(ficlVm *vm);

static void	ficl_dabs(ficlVm *vm);
static void	ficl_dmax(ficlVm *vm);
static void	ficl_dmin(ficlVm *vm);
static void	ficl_dnegate(ficlVm *vm);
static void	ficl_dtwoslash(ficlVm *vm);
static void	ficl_dtwostar(ficlVm *vm);
static void	ficl_to_d(ficlVm *vm);
static void	ficl_to_s(ficlVm *vm);

static char    *format_double(char *buf, size_t size, ficlFloat f);
static FTH	fl_inspect(FTH self);
static FTH	fl_to_string(FTH self);
static FTH	fl_copy(FTH self);
static FTH	fl_equal_p(FTH self, FTH obj);

static void	ficl_dfloats(ficlVm *vm);
static void	ficl_f_dot_r(ficlVm *vm);
static void	ficl_falign(ficlVm *vm);
static void	ficl_falog(ficlVm *vm);
static void	ficl_fexpm1(ficlVm *vm);
static void	ficl_float_p(ficlVm *vm);
static void	ficl_flogp1(ficlVm *vm);
static void	ficl_fsincos(ficlVm *vm);
static void	ficl_inf(ficlVm *vm);
static void	ficl_inf_p(ficlVm *vm);
static void	ficl_nan(ficlVm *vm);
static void	ficl_nan_p(ficlVm *vm);
static void	ficl_to_f(ficlVm *vm);
static void	ficl_uf_dot_r(ficlVm *vm);

static void	ficl_cimage(ficlVm *vm);
static void	ficl_creal(ficlVm *vm);

#if HAVE_COMPLEX
static FTH	cp_inspect(FTH self);
static FTH	cp_to_string(FTH self);
static FTH	cp_copy(FTH self);
static FTH	cp_equal_p(FTH self, FTH obj);

static void	ficl_c_dot(ficlVm *vm);
static void	ficl_ceq(ficlVm *vm);
static void	ficl_ceqz(ficlVm *vm);
static void	ficl_cnoteq(ficlVm *vm);
static void	ficl_cnoteqz(ficlVm *vm);
static void	ficl_creciprocal(ficlVm *vm);
static void	ficl_make_complex_polar(ficlVm *vm);
static void	ficl_make_complex_rectangular(ficlVm *vm);
static void	ficl_to_c(ficlVm *vm);
static ficlComplex make_polar(ficlFloat real, ficlFloat theta);
#endif				/* HAVE_COMPLEX */

#if HAVE_BN
static FTH	bn_inspect(FTH self);
static FTH	bn_to_string(FTH self);
static FTH	bn_copy(FTH self);
static FTH	bn_equal_p(FTH self, FTH obj);
static void	bn_free(FTH self);

static BN_CTX  *bn_ctx_init(void);
static void	int_to_bn(ficlBignum m, ficlInteger x);
static void	float_to_bn(ficlBignum m, ficlFloat x);
static void	fth_to_bn(ficlBignum m, FTH x);
static ficlInteger bn_to_int(ficlBignum m);
static ficlBignum bn_addsub(FTH m, FTH n, int type);
static ficlBignum bn_muldiv(FTH m, FTH n, int type);
static FTH	bn_add(FTH m, FTH n);
static FTH	bn_sub(FTH m, FTH n);
static FTH	bn_mul(FTH m, FTH n);
static FTH	bn_div(FTH m, FTH n);
static void	ficl_bn_dot(ficlVm *vm);

static void	ficl_bpow(ficlVm *vm);
static void	ficl_bnegate(ficlVm *vm);
static void	ficl_babs(ficlVm *vm);
static void	ficl_bmin(ficlVm *vm);
static void	ficl_bmax(ficlVm *vm);
static void	ficl_btwostar(ficlVm *vm);
static void	ficl_btwoslash(ficlVm *vm);
static void	ficl_blshift(ficlVm *vm);
static void	ficl_brshift(ficlVm *vm);

static FTH	rt_inspect(FTH self);
static FTH	rt_to_string(FTH self);
static FTH	rt_copy(FTH self);
static FTH	rt_equal_p(FTH self, FTH obj);
static void	rt_free(FTH self);

static ficlRatio rt_init(void);
static void	rt_bn_free(ficlRatio r);
static void	fth_to_rt(ficlRatio m, FTH x);
static ficlInteger rt_to_int(ficlRatio r);
static ficlFloat rt_to_float(ficlRatio r);
static void	rt_canonicalize(ficlRatio r);
static FTH	make_rational(ficlBignum num, ficlBignum den);
static void	ficl_to_ratio(ficlVm *vm);
static void	ficl_q_dot(ficlVm *vm);
static void	ficl_s_to_q(ficlVm *vm);
static void	ficl_f_to_q(ficlVm *vm);
static void	ficl_qnegate(ficlVm *vm);
static void	ficl_qfloor(ficlVm *vm);
static void	ficl_qceil(ficlVm *vm);
static void	ficl_qabs(ficlVm *vm);
static void	ficl_qinvert(ficlVm *vm);
static int	rt_cmp(ficlRatio m, ficlRatio n);
static ficlRatio rt_addsub(FTH m, FTH n, int type);
static ficlRatio rt_muldiv(FTH m, FTH n, int type);
static FTH	rt_add(FTH m, FTH n);
static FTH	rt_sub(FTH m, FTH n);
static FTH	rt_mul(FTH m, FTH n);
static FTH	rt_div(FTH m, FTH n);
static FTH	number_floor(FTH m);
static FTH	number_inv(FTH m);
static void	ficl_rationalize(ficlVm *vm);
#endif				/* HAVE_BN */

#define NUMB_FIXNUM_P(Obj)	(IMMEDIATE_P(Obj) && FIXNUM_P(Obj))
#define FTH_FLOAT_REF_INT(Obj)	FTH_ROUND(FTH_FLOAT_OBJECT(Obj))

#if HAVE_COMPLEX
#define FTH_COMPLEX_REAL(Obj)	creal(FTH_COMPLEX_OBJECT(Obj))
#define FTH_COMPLEX_IMAG(Obj)	cimag(FTH_COMPLEX_OBJECT(Obj))

ficlComplex
ficlStackPopComplex(ficlStack *stack)
{
	ficlComplex cp;

	cp = fth_complex_ref(ficl_to_fth(STACK_FTH_REF(stack)));
	stack->top--;
	return (cp);
}

void
ficlStackPushComplex(ficlStack *stack, ficlComplex cp)
{
	FTH fp;

	fp = fth_make_complex(cp);
	++stack->top;
	STACK_FTH_SET(stack, fp);
}
#else				/* !HAVE_COMPLEX */
#define FTH_COMPLEX_REAL(Obj)	fth_real_ref(Obj)
#define FTH_COMPLEX_IMAG(Obj)	0.0
#endif				/* HAVE_COMPLEX */

#if HAVE_BN
#define FTH_BIGNUM_REF_INT(Obj)  bn_to_int(FTH_BIGNUM_OBJECT(Obj))
#define FTH_BIGNUM_REF_UINT(Obj) BN_get_word(FTH_BIGNUM_OBJECT(Obj))
#define FTH_RATIO_REF_INT(Obj)   rt_to_int(FTH_RATIO_OBJECT(Obj))
#define FTH_RATIO_REF_FLOAT(Obj) rt_to_float(FTH_RATIO_OBJECT(Obj))
#else				/* !HAVE_BN */
#define FTH_BIGNUM_REF_INT(Obj)  0L
#define FTH_BIGNUM_REF_UINT(Obj) 0UL
#define FTH_RATIO_REF_INT(Obj)   0L
#define FTH_RATIO_REF_FLOAT(Obj) 0.0
#endif				/* HAVE_BN */

static FTH
make_object_number_type(const char *name, fobj_t type, int flags)
{
	FTH new;

	new = make_object_type(name, type);
	FTH_OBJECT_FLAG(new) = N_NUMBER_T | flags;
	return (new);
}

#if defined(HAVE_POW)
#define FTH_POW(x, y)		pow(x, y)
#else
#define FTH_POW(x, y)		FTH_NOT_IMPLEMENTED(pow)
#endif

#if defined(HAVE_FLOOR)
#define FTH_FLOOR(r)		floor(r)
#else
#define FTH_FLOOR(r)		((ficlFloat)((ficlInteger)(r)))
#endif

#if defined(HAVE_CEIL)
#define FTH_CEIL(r)		ceil(r)
#else
#define FTH_CEIL(r)		((ficlFloat)((ficlInteger)((r) + 1.0)))
#endif

#if defined(HAVE_RINT)
#define FTH_ROUND(r)		rint(r)
#else
static ficlFloat
fth_round(ficlFloat r)
{
	if (r != FTH_FLOOR(r)) {
		ficlFloat half, half2, res;

		half = r + 0.5;
		half2 = half * 0.5;
		res = FTH_FLOOR(half);
		if (half == res && half2 != FTH_FLOOR(half2))
			return (res - 1.0);
		return (res);
	}
	return (r);
}

#define FTH_ROUND(r)		fth_round(r)
#endif

#if defined(HAVE_TRUNC)
#define FTH_TRUNC(r)		trunc(r)
#else
#define FTH_TRUNC(r)		(((r) < 0.0) ? -FTH_FLOOR(-(r)) : FTH_FLOOR(r))
#endif

#if defined(INFINITY)
#define FTH_INF			(ficlFloat)INFINITY
#else
static double	fth_infinity;
#define FTH_INF			fth_infinity
#endif

#if defined(NAN)
#define FTH_NAN			(ficlFloat)NAN
#else
#define FTH_NAN			sqrt(-1.0)
#endif

bool
fth_isinf(ficlFloat x)
{
#if defined(HAVE_DECL_ISINF)
	return ((bool)isinf(x));
#else
	return (false);
#endif
}

bool
fth_isnan(ficlFloat x)
{
#if defined(HAVE_DECL_ISNAN)
	return ((bool)isnan(x));
#else
	return (x == sqrt(-1.0));	/* NaN */
#endif
}

#if !defined(HAVE_LOG2)
static double
log2(double x)
{
	return (log10(x) / log10(2.0));
}

#endif

static ficlFloat
fth_log(ficlFloat x)
{
	if (x >= 0.0)
		return (log(x));
	FTH_MATH_ERROR_THROW("log, x < 0");
	/* NOTREACHED */
	return (0.0);
}

static ficlFloat
fth_log2(ficlFloat x)
{
	if (x >= 0.0)
		return (log2(x));
	FTH_MATH_ERROR_THROW("log2, x < 0");
	/* NOTREACHED */
	return (0.0);
}

static ficlFloat
fth_log10(ficlFloat x)
{
	if (x >= 0.0)
		return (log10(x));
	FTH_MATH_ERROR_THROW("log10, x < 0");
	/* NOTREACHED */
	return (0.0);
}

/*
 * Minix seems to lack asinh, acosh, atanh
 */
#if !defined(HAVE_ASINH)
ficlFloat
asinh(ficlFloat x)
{
	return (log(x + sqrt(x * x + 1.0)));
}

#endif

#if !defined(HAVE_ACOSH)
ficlFloat
acosh(ficlFloat x)
{
	return (log(x + sqrt(x * x - 1.0)));
}

#endif

#if !defined(HAVE_ATANH)
ficlFloat
atanh(ficlFloat x)
{
	/* from freebsd (/usr/src/lib/msun/src/e_atanh.c) */
	if (fabs(x) > 1.0)
		return (FTH_NAN);
	if (fabs(x) == 1.0)
		return (FTH_INF);
	if (fth_isnan(x))
		return (FTH_NAN);
	return (log((1.0 + x) / (1.0 - x)) * 0.5);
}
#endif

/* === Begin of missing complex functions. === */

#if HAVE_COMPLEX

/*
 * Some libc/libm do provide them, but others do not (like FreeBSD).
 */

/* Trigonometric functions. */

#if !defined(HAVE_CSIN)
ficlComplex
csin(ficlComplex z)
{
	return (sin(creal(z)) * cosh(cimag(z)) +
	    (cos(creal(z)) * sinh(cimag(z))) * _Complex_I);
}
#endif

#if !defined(HAVE_CCOS)
ficlComplex
ccos(ficlComplex z)
{
	return (cos(creal(z)) * cosh(cimag(z)) +
	    (-sin(creal(z)) * sinh(cimag(z))) * _Complex_I);
}
#endif

#if !defined(HAVE_CTAN)
ficlComplex
ctan(ficlComplex z)
{
	return (csin(z) / ccos(z));
}
#endif

#if !defined(HAVE_CASIN)
ficlComplex
casin(ficlComplex z)
{
	return (-_Complex_I * clog(_Complex_I * z + csqrt(1.0 - z * z)));
}
#endif

#if !defined(HAVE_CACOS)
ficlComplex
cacos(ficlComplex z)
{
	return (-_Complex_I * clog(z + _Complex_I * csqrt(1.0 - z * z)));
}
#endif

#if !defined(HAVE_CATAN)
ficlComplex
catan(ficlComplex z)
{
	return (_Complex_I *
	    clog((_Complex_I + z) / (_Complex_I - z)) / 2.0);
}
#endif

#if !defined(HAVE_CATAN2)
ficlComplex
catan2(ficlComplex z, ficlComplex x)
{
	return (-_Complex_I *
	    clog((x + _Complex_I * z) / csqrt(x * x + z * z)));
}
#endif

/* Hyperbolic functions. */

#if !defined(HAVE_CSINH)
ficlComplex
csinh(ficlComplex z)
{
	return (sinh(creal(z)) * cos(cimag(z)) +
	    (cosh(creal(z)) * sin(cimag(z))) * _Complex_I);
}
#endif

#if !defined(HAVE_CCOSH)
ficlComplex
ccosh(ficlComplex z)
{
	return (cosh(creal(z)) * cos(cimag(z)) +
	    (sinh(creal(z)) * sin(cimag(z))) * _Complex_I);
}
#endif

#if !defined(HAVE_CTANH)
ficlComplex
ctanh(ficlComplex z)
{
	return (csinh(z) / ccosh(z));
}
#endif

#if !defined(HAVE_CASINH)
ficlComplex
casinh(ficlComplex z)
{
	return (clog(z + csqrt(1.0 + z * z)));
}
#endif

#if !defined(HAVE_CACOSH)
ficlComplex
cacosh(ficlComplex z)
{
	return (clog(z + csqrt(z * z - 1.0)));
}
#endif

#if !defined(HAVE_CATANH)
ficlComplex
catanh(ficlComplex z)
{
	return (clog((1.0 + z) / (1.0 - z)) / 2.0);
}
#endif

/* Exponential and logarithmic functions. */

#if !defined(HAVE_CEXP)
ficlComplex
cexp(ficlComplex z)
{
	return (exp(creal(z)) * cos(cimag(z)) +
	    (exp(creal(z)) * sin(cimag(z))) * _Complex_I);
}
#endif

#if !defined(HAVE_CLOG)
ficlComplex
clog(ficlComplex z)
{
	return (log(fabs(cabs(z))) + carg(z) * _Complex_I);
}
#endif

#if !defined(HAVE_CLOG10)
ficlComplex
clog10(ficlComplex z)
{
	return (clog(z) / log(10));
}
#endif

/* Power functions. */

#if !defined(HAVE_CPOW)
ficlComplex
cpow(ficlComplex a, ficlComplex z)
{
	/* from netbsd (/usr/src/lib/libm/complex/cpow.c) */
	double x, y, r, theta, absa, arga;

	x = creal(z);
	y = cimag(z);
	absa = cabs(a);

	if (absa == 0.0)
		return (0.0 + 0.0 * _Complex_I);

	arga = carg(a);
	r = FTH_POW(absa, x);
	theta = x * arga;

	if (y != 0.0) {
		r = r * exp(-y * arga);
		theta = theta + y * log(absa);
	}
	return (r * cos(theta) + (r * sin(theta)) * _Complex_I);
}
#endif

#if !defined(HAVE_CSQRT)
ficlComplex
csqrt(ficlComplex z)
{
	ficlFloat r, x;

	if (cimag(z) < 0.0)
		return (conj(csqrt(conj(z))));
	r = cabs(z);
	x = creal(z);
	return (sqrt((r + x) / 2.0) + sqrt((r - x) / 2.0) * _Complex_I);
}
#endif

/* Absolute value and conjugates. */

#if !defined(HAVE_CABS)
ficlFloat
cabs(ficlComplex z)
{
	return (hypot(creal(z), cimag(z)));
}
#endif

#if !defined(HAVE_CABS2)
ficlFloat
cabs2(ficlComplex z)
{
	return (creal(z) * creal(z) + cimag(z) * cimag(z));
}
#endif

#if !defined(HAVE_CARG)
ficlFloat
carg(ficlComplex z)
{
	return (atan2(cimag(z), creal(z)));
}
#endif

#if !defined(HAVE_CONJ)
ficlComplex
conj(ficlComplex z)
{
	return (~z);
}
#endif
#endif				/* HAVE_COMPLEX */

/* End of missing complex functions. */

#define N_FUNC_ONE_ARG(Name, CName, Type)				\
static void								\
ficl_ ## Name(ficlVm *vm)						\
{									\
	FTH_STACK_CHECK(vm, 1, 1);					\
	ficlStackPush ## Type(vm->dataStack,				\
	    CName(ficlStackPop ## Type(vm->dataStack)));		\
}									\
static char* h_ ## Name = "( x -- y )  y = " #CName "(x)"

#define N_FUNC_TWO_ARGS(Name, CName, Type)				\
static void								\
ficl_ ## Name(ficlVm *vm)						\
{									\
	ficl ## Type x;							\
	ficl ## Type y;							\
									\
	FTH_STACK_CHECK(vm, 2, 1);					\
	y = ficlStackPop ## Type(vm->dataStack);			\
	x = ficlStackPop ## Type(vm->dataStack);			\
	ficlStackPush ## Type(vm->dataStack, CName(x, y));		\
}									\
static char* h_ ## Name = "( x y -- z )  z = " #CName "(x, y)"

#define N_FUNC_TWO_ARGS_OP(Name, OP, Type)				\
static void								\
ficl_ ## Name(ficlVm *vm)						\
{									\
	ficl ## Type x;							\
	ficl ## Type y;							\
									\
	FTH_STACK_CHECK(vm, 2, 1);					\
	y = ficlStackPop ## Type(vm->dataStack);			\
	x = ficlStackPop ## Type(vm->dataStack);			\
	ficlStackPush ## Type(vm->dataStack, x OP y);			\
}									\
static char* h_ ## Name = "( x y -- z )  z = x " #OP " y"

#define N_FUNC_TEST_ZERO(Name, OP, Type)				\
static void								\
ficl_ ## Name(ficlVm *vm)						\
{									\
	FTH_STACK_CHECK(vm, 1, 1);					\
	ficlStackPushBoolean(vm->dataStack,				\
	    (ficlStackPop ## Type(vm->dataStack) OP 0));		\
}									\
static char* h_ ## Name = "( x -- f )  x " #OP " 0 => flag"

#define N_FUNC_TEST_TWO_OP(Name, OP, Type)				\
static void								\
ficl_ ## Name(ficlVm *vm)						\
{									\
	ficl ## Type x;							\
	ficl ## Type y;							\
									\
	FTH_STACK_CHECK(vm, 2, 1);					\
	y = ficlStackPop ## Type(vm->dataStack);			\
	x = ficlStackPop ## Type(vm->dataStack);			\
	ficlStackPushBoolean(vm->dataStack, (x OP y));			\
}									\
static char* h_ ## Name = "( x y -- f )  x " #OP " y => flag"

static void
ficl_to_s(ficlVm *vm)
{
#define h_to_s "( x -- y )  Convert any number X to ficlInteger"
	FTH n;

	FTH_STACK_CHECK(vm, 1, 1);
	n = fth_pop_ficl_cell(vm);
	ficlStackPushInteger(vm->dataStack, fth_int_ref(n));
}

static void
ficl_to_d(ficlVm *vm)
{
#define h_to_d "( x -- d )  Convert any number X to ficl2Integer"
	ficl2Integer d;

	FTH_STACK_CHECK(vm, 1, 1);
	d = ficlStackPop2Integer(vm->dataStack);
	ficlStackPushFTH(vm->dataStack, fth_make_llong(d));
}

static void
ficl_to_f(ficlVm *vm)
{
#define h_to_f "( x -- y )  Convert any number X to ficlFloat"
	ficlFloat f;

	FTH_STACK_CHECK(vm, 1, 1);
	f = ficlStackPopFloat(vm->dataStack);
	ficlStackPushFloat(vm->dataStack, f);
}

#if HAVE_COMPLEX
static void
ficl_to_c(ficlVm *vm)
{
#define h_to_c "( x -- y )  Convert any number X to ficlComplex"
	ficlComplex cp;

	FTH_STACK_CHECK(vm, 1, 1);
	cp = ficlStackPopComplex(vm->dataStack);
	ficlStackPushComplex(vm->dataStack, cp);
}
#endif

/* === LONG-LONG === */

#define h_list_of_llong_functions "\
*** NUMBER PRIMITIVES ***\n\
number?   fixnum?   unsigned?\n\
long-long? ulong-long?\n\
integer?  exact?    inexact?\n\
make-long-long     make-ulong-long\n\
rand-seed-ref  rand-seed-set!\n\
random    frandom\n\
.r   u.r  d.   ud.  d.r  ud.r\n\
u=   u<>  u<   u>   u<=  u>=\n\
s>d  d>s  f>d  d>f\n\
d0=  (dzero?)  d0<> d0<  (dnegative?) d0>  d0<= d0>= (dpositive?)\n\
d=   d<>  d<   d>   d<=  d>=\n\
du=  du<> du<  du>  du<= du>=\n\
d+   d-   d*   d/\n\
dnegate   dabs dmin dmax d2*  d2/"

static FTH
ll_inspect(FTH self)
{
	return (fth_make_string_format("%s: %lld",
	    FTH_INSTANCE_NAME(self), FTH_LONG_OBJECT(self)));
}

static FTH
ll_to_string(FTH self)
{
	return (fth_make_string_format("%lld", FTH_LONG_OBJECT(self)));
}

static FTH
ll_copy(FTH self)
{
	return (fth_make_llong(FTH_LONG_OBJECT(self)));
}

static FTH
ll_equal_p(FTH self, FTH obj)
{
	return (BOOL_TO_FTH(FTH_LONG_OBJECT(self) == FTH_LONG_OBJECT(obj)));
}

FTH
fth_make_llong(ficl2Integer d)
{
	FTH self;

	self = fth_make_instance(llong_tag, NULL);
	FTH_LONG_OBJECT_SET(self, d);
	return (self);
}

FTH
fth_make_ullong(ficl2Unsigned ud)
{
	FTH self;

	self = fth_make_instance(llong_tag, NULL);
	FTH_ULONG_OBJECT_SET(self, ud);
	return (self);
}

FTH
fth_llong_copy(FTH obj)
{
	if (FTH_LLONG_P(obj))
		return (ll_copy(obj));
	return (obj);
}

bool
fth_fixnum_p(FTH obj)
{
	return (NUMB_FIXNUM_P(obj));
}

bool
fth_number_p(FTH obj)
{
	return (NUMB_FIXNUM_P(obj) || FTH_NUMBER_T_P(obj));
}

bool
fth_exact_p(FTH obj)
{
	return (NUMB_FIXNUM_P(obj) || FTH_EXACT_T_P(obj));
}

bool
fth_integer_p(FTH obj)
{
	return (NUMB_FIXNUM_P(obj) || FTH_LLONG_P(obj));
}

bool
fth_char_p(FTH obj)
{
	return (NUMB_FIXNUM_P(obj) && isprint((int)FIX_TO_INT(obj)));
}

bool
fth_unsigned_p(FTH obj)
{
	return (fth_integer_p(obj) && fth_long_long_ref(obj) >= 0);
}

bool
fth_ullong_p(FTH obj)
{
	return (FTH_LLONG_P(obj) && FTH_LONG_OBJECT(obj) >= 0);
}

static void
ficl_number_p(ficlVm *vm)
{
#define h_number_p "( obj -- f )  test if OBJ is a number\n\
nil number? => #f\n\
0   number? => #t\n\
0i  number? => #t\n\
Return #t if OBJ is a number (ficlInteger, ficl2Integer, ficlFloat, \
ficlComplex, ficlBignum, ficlRatio), otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_NUMBER_P(obj));
}

static void
ficl_fixnum_p(ficlVm *vm)
{
#define h_fixnum_p "( obj -- f )  test if OBJ is a fixnum\n\
nil fixnum? => #f\n\
0   fixnum? => #t\n\
0x3fffffff    fixnum? => #t\n\
0x3fffffff 1+ fixnum? => #f\n\
Return #t if OBJ is a fixnum (ficlInteger/ficlUnsigned), otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, NUMB_FIXNUM_P(obj));
}

static void
ficl_unsigned_p(ficlVm *vm)
{
#define h_unsigned_p "( obj -- f )  test if OBJ is a unsigned integer\n\
nil unsigned? => #f\n\
-1  unsigned? => #f\n\
0   unsigned? => #t\n\
0xffffffffffff unsigned? => #t\n\
Return #t if OBJ is a unsigned integer (ficlUnsigned, \
ficl2Unsigned, ficlBignum), otherwise #f."
	FTH obj;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	if (FTH_UNSIGNED_P(obj))
		flag = true;
#if HAVE_BN
	else if (FTH_BIGNUM_P(obj))
		flag = (BN_cmp(FTH_BIGNUM_OBJECT(obj), &fth_bn_zero) >= 0);
#endif
	else
		flag = false;
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_llong_p(ficlVm *vm)
{
#define h_llong_p "( obj -- f )  test if OBJ is an long-long integer\n\
nil long-long? => #f\n\
-1  long-long? => #f\n\
-1 make-long-long long-long? => #t\n\
Return #t if OBJ is an long-long object (ficl2Integer/ficl2Unsigned), \
otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_LLONG_P(obj));
}

static void
ficl_ullong_p(ficlVm *vm)
{
#define h_ullong_p "( obj -- f )  test if OBJ is a unsigned long-long integer\n\
nil ulong-long? => #f\n\
1   ulong-long? => #f\n\
1 make-ulong-long ulong-long? => #t\n\
Return #t if OBJ is a ulong-long object (ficl2Unsigned), otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_ULLONG_P(obj));
}

static void
ficl_integer_p(ficlVm *vm)
{
#define h_integer_p "( obj -- f )  test if OBJ is an integer\n\
nil integer? => #f\n\
1.0 integer? => #f\n\
-1  integer? => #t\n\
1 make-long-long integer? => #t\n\
12345678901234567890 integer? => #t\n\
Return #t if OBJ is an integer (ficlInteger, ficl2Integer, or ficlBignum), \
otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack,
	    FTH_INTEGER_P(obj) || FTH_BIGNUM_P(obj));
}

static void
ficl_exact_p(ficlVm *vm)
{
#define h_exact_p "( obj -- f )  test if OBJ is an exact number\n\
1   exact? => #t\n\
1/2 exact? => #t\n\
1.0 exact? => #f\n\
1i  exact? => #f\n\
Return #t if OBJ is an exact number (not ficlFloat or \
ficlComplex), otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_EXACT_P(obj));
}

static void
ficl_inexact_p(ficlVm *vm)
{
#define h_inexact_p "( obj -- f )  test if OBJ is an inexact number\n\
1.0 inexact? => #t\n\
1i  inexact? => #t\n\
1   inexact? => #f\n\
1/2 inexact? => #f\n\
Return #t if OBJ is an inexact number (ficlFloat, ficlComplex), otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_INEXACT_P(obj));
}

/*
 * Integer types.
 *
 * Return a FTH fixnum or a FTH llong object depending on N.
 */
FTH
fth_make_int(ficlInteger n)
{
	if (FIXABLE_P(n))
		return (INT_TO_FIX(n));
	return (fth_make_llong((ficl2Integer)n));
}

/*
 * Return a FTH unsigned fixnum or a FTH ullong object depending on U.
 */
FTH
fth_make_unsigned(ficlUnsigned u)
{
	if (UFIXABLE_P(u))
		return (UNSIGNED_TO_FIX(u));
	return (fth_make_ullong((ficl2Unsigned)u));
}

/*
 * Return a FTH fixnum or a FTH llong object depending on D.
 */
FTH
fth_make_long_long(ficl2Integer d)
{
	if (FIXABLE_P(d))
		return (INT_TO_FIX((ficlInteger)d));
	return (fth_make_llong(d));
}

/*
 * Return a FTH unsigned fixnum or a FTH ullong object depending on UD.
 */
FTH
fth_make_ulong_long(ficl2Unsigned ud)
{
	if (UFIXABLE_P(ud))
		return (UNSIGNED_TO_FIX((ficlUnsigned)ud));
	return (fth_make_ullong(ud));
}

static void
ficl_make_long_long(ficlVm *vm)
{
#define h_make_long_long "( val -- d )  return ficl2Integer\n\
1 make-long-long long-long? => #t\n\
Return VAL as Forth double (ficl2Integer)."
	ficl2Integer d;

	FTH_STACK_CHECK(vm, 1, 1);
	d = ficlStackPop2Integer(vm->dataStack);
	ficlStackPushFTH(vm->dataStack, fth_make_llong(d));
}

static void
ficl_make_ulong_long(ficlVm *vm)
{
#define h_make_ulong_long "( val -- ud )  return ficl2Unsigned\n\
1 make-ulong-long ulong-long? => #t\n\
Return VAL as Forth unsigned double (ficl2Unsigned)."
	ficl2Unsigned ud;

	FTH_STACK_CHECK(vm, 1, 1);
	ud = ficlStackPop2Unsigned(vm->dataStack);
	ficlStackPushFTH(vm->dataStack, fth_make_ullong(ud));
}

/*
 * Supposed to be used in FTH_INT_REF() macro.
 */
ficlInteger
fth_integer_ref(FTH x)
{
	if (NUMB_FIXNUM_P(x))
		return (FIX_TO_INT(x));
	if (FTH_LLONG_P(x))
		return (ficlInteger)(FTH_LONG_OBJECT(x));
	return ((ficlInteger)x);
}

/*
 * Convert any number to type.
 *
 * Return C ficlInteger from X.
 */
ficlInteger
fth_int_ref(FTH x)
{
	return (fth_int_ref_or_else(x, (ficlInteger)x));
}

/*
 * Return C ficlInteger from X.  If X doesn't fit in Fixnum, FTH llong,
 * FTH float, FTH complex, or any bignum, return FALLBACK.
 */
ficlInteger
fth_int_ref_or_else(FTH x, ficlInteger fallback)
{
	int type = -1;
	ficlInteger i;

	if (NUMB_FIXNUM_P(x))
		return (FIX_TO_INT(x));
	if (x == 0)
		return (fallback);
	if (FTH_NUMBER_T_P(x))
		type = FTH_INSTANCE_TYPE(x);
	switch (type) {
	case FTH_LLONG_T:
		i = (ficlInteger)FTH_LONG_OBJECT(x);
		break;
	case FTH_FLOAT_T:
		i = (ficlInteger)FTH_FLOAT_REF_INT(x);
		break;
	case FTH_COMPLEX_T:
		i = (ficlInteger)FTH_ROUND(FTH_COMPLEX_REAL(x));
		break;
	case FTH_BIGNUM_T:
		i = FTH_BIGNUM_REF_INT(x);
		break;
	case FTH_RATIO_T:
		i = FTH_RATIO_REF_INT(x);
		break;
	default:
		i = fallback;
		break;
	}
	return (i);
}

/*
 * Return C ficl2Integer from X.
 */
ficl2Integer
fth_long_long_ref(FTH x)
{
	int type = -1;
	ficl2Integer d;

	if (NUMB_FIXNUM_P(x))
		return ((ficl2Integer)FIX_TO_INT(x));
	if (FTH_NUMBER_T_P(x))
		type = FTH_INSTANCE_TYPE(x);
	switch (type) {
	case FTH_LLONG_T:
		d = FTH_LONG_OBJECT(x);
		break;
	case FTH_FLOAT_T:
		d = (ficl2Integer)FTH_FLOAT_REF_INT(x);
		break;
	case FTH_COMPLEX_T:
		d = (ficl2Integer)FTH_ROUND(FTH_COMPLEX_REAL(x));
		break;
	case FTH_BIGNUM_T:
		d = (ficl2Integer)FTH_BIGNUM_REF_INT(x);
		break;
	case FTH_RATIO_T:
		d = (ficl2Integer)FTH_RATIO_REF_INT(x);
		break;
	default:
		d = (ficl2Integer)x;
		break;
	}
	return (d);
}

/*
 * Return C ficlUnsigned from X.
 */
ficlUnsigned
fth_unsigned_ref(FTH x)
{
	int type = -1;
	ficlUnsigned u;

	if (NUMB_FIXNUM_P(x))
		return (FIX_TO_UNSIGNED(x));
	if (FTH_NUMBER_T_P(x))
		type = FTH_INSTANCE_TYPE(x);
	switch (type) {
	case FTH_LLONG_T:
		u = (ficlUnsigned)FTH_LONG_OBJECT(x);
		break;
	case FTH_FLOAT_T:
		u = (ficlUnsigned)FTH_FLOAT_REF_INT(x);
		break;
	case FTH_COMPLEX_T:
		u = (ficlUnsigned)FTH_ROUND(FTH_COMPLEX_REAL(x));
		break;
	case FTH_BIGNUM_T:
		u = (ficlUnsigned)FTH_BIGNUM_REF_UINT(x);
		break;
	case FTH_RATIO_T:
		u = (ficlUnsigned)FTH_RATIO_REF_INT(x);
		break;
	default:
		u = (ficlUnsigned)x;
		break;
	}
	return (u);
}

/*
 * Return C ficl2Unsigned from X.
 */
ficl2Unsigned
fth_ulong_long_ref(FTH x)
{
	int type = -1;
	ficl2Unsigned ud;

	if (NUMB_FIXNUM_P(x))
		return ((ficl2Unsigned)FIX_TO_UNSIGNED(x));
	if (FTH_NUMBER_T_P(x))
		type = FTH_INSTANCE_TYPE(x);
	switch (type) {
	case FTH_LLONG_T:
		ud = FTH_ULONG_OBJECT(x);
		break;
	case FTH_FLOAT_T:
		ud = (ficl2Unsigned)FTH_FLOAT_REF_INT(x);
		break;
	case FTH_COMPLEX_T:
		ud = (ficl2Unsigned)FTH_ROUND(FTH_COMPLEX_REAL(x));
		break;
	case FTH_BIGNUM_T:
		ud = (ficl2Unsigned)FTH_BIGNUM_REF_UINT(x);
		break;
	case FTH_RATIO_T:
		ud = (ficl2Unsigned)FTH_RATIO_REF_INT(x);
		break;
	default:
		ud = (ficl2Unsigned)x;
		break;
	}
	return (ud);
}

/*
 * Return C ficlFloat from X.
 */
ficlFloat
fth_float_ref(FTH x)
{
	return (fth_float_ref_or_else(x, (ficlFloat)x));
}

/*
 * Alias for fth_float_ref().
 */
ficlFloat
fth_real_ref(FTH x)
{
	return (fth_float_ref(x));
}

/*
 * Return C ficlFloat from X.  If X isn't of type Fixnum, FTH llong,
 * FTH float, FTH complex, or any bignum, return FALLBACK.
 */
ficlFloat
fth_float_ref_or_else(FTH x, ficlFloat fallback)
{
	int type = -1;
	ficlFloat f;

	if (NUMB_FIXNUM_P(x))
		return ((ficlFloat)FIX_TO_INT(x));
	if (FTH_NUMBER_T_P(x))
		type = FTH_INSTANCE_TYPE(x);
	switch (type) {
	case FTH_FLOAT_T:
		f = FTH_FLOAT_OBJECT(x);
		break;
	case FTH_COMPLEX_T:
		f = FTH_COMPLEX_REAL(x);
		break;
	case FTH_LLONG_T:
		f = (ficlFloat)FTH_LONG_OBJECT(x);
		break;
	case FTH_BIGNUM_T:
		f = (ficlFloat)FTH_BIGNUM_REF_INT(x);
		break;
	case FTH_RATIO_T:
		f = FTH_RATIO_REF_FLOAT(x);
		break;
	default:
		f = fallback;
		break;
	}
	return (f);
}

#if HAVE_COMPLEX
/*
 * Return C ficlComplex from X.
 */
ficlComplex
fth_complex_ref(FTH x)
{
	if (FTH_COMPLEX_P(x))
		return (FTH_COMPLEX_OBJECT(x));
	return (fth_float_ref_or_else(x, 0.0) + 0.0 * _Complex_I);
}
#endif

/* === RANDOM === */

/*
 * From clm/cmus.c.
 */
static ficlUnsigned fth_rand_rnd;

#define INVERSE_MAX_RAND	0.0000610351563
#define INVERSE_MAX_RAND2	0.000030517579

void
fth_srand(ficlUnsigned val)
{
	fth_rand_rnd = val;
}

static ficlFloat
next_rand(void)
{
	unsigned long val;
	fth_rand_rnd = fth_rand_rnd * 1103515245 + 12345;
	val = (unsigned long)(fth_rand_rnd >> 16) & 32767;
	return ((ficlFloat)val);
}

/* -amp to amp as double */
ficlFloat
fth_frandom(ficlFloat amp)
{
	return (amp * (next_rand() * INVERSE_MAX_RAND - 1.0));
}

/* 0..amp as double */
ficlFloat
fth_random(ficlFloat amp)
{
	return (amp * (next_rand() * INVERSE_MAX_RAND2));
}

static void
ficl_random(ficlVm *vm)
{
#define h_random "( r -- 0.0..r)  return randomized value\n\
1 random => 0.513855\n\
Return pseudo randomized value between 0.0 and R.\n\
See also frandom."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushFloat(vm->dataStack,
	    fth_random(ficlStackPopFloat(vm->dataStack)));
}

static void
ficl_frandom(ficlVm *vm)
{
#define h_frandom "( r -- -r...r)  return randomized value\n\
1 frandom => -0.64856\n\
Return pseudo randomized value between -R and R.\n\
See also random."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushFloat(vm->dataStack,
	    fth_frandom(ficlStackPopFloat(vm->dataStack)));
}

static void
ficl_rand_seed_ref(ficlVm *vm)
{
#define h_rand_seed_ref "( -- seed )  return rand seed\n\
rand-seed-ref => 213\n\
Return content of the seed variable fth_rand_rnd.\n\
See also rand-seed-set!."
	FTH_STACK_CHECK(vm, 0, 1);
	ficlStackPushUnsigned(vm->dataStack, fth_rand_rnd);
}

static void
ficl_rand_seed_set(ficlVm *vm)
{
#define h_rand_seed_set "( seed -- )  set rand seed\n\
213 rand-seed-set!\n\
Set SEED to the seed variable fth_rand_rnd.\n\
See also rand-seed-ref."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_rand_rnd = ficlStackPopUnsigned(vm->dataStack);
}

/* === FORMATTED NUMBER OUTPUT === */

static void
ficl_dot_r(ficlVm *vm)
{
#define h_dot_r "( n1 n2 -- )  formatted number output\n\
17 3 .r => | 17 |\n\
Print integer N1 in a right-adjusted field of N2 characters.\n\
See also u.r"
	ficlInteger val;
	int i;

	FTH_STACK_CHECK(vm, 2, 0);
	i = (int)ficlStackPopInteger(vm->dataStack);
	val = ficlStackPopInteger(vm->dataStack);
	fth_printf("%*ld ", i, val);
}

static void
ficl_u_dot_r(ficlVm *vm)
{
#define h_u_dot_r "( u n -- )  formatted number output\n\
17 3 u.r => | 17 |\n\
Print unsigned integer U in a right-adjusted field of N characters.\n\
See also .r"
	ficlUnsigned val;
	int i;

	FTH_STACK_CHECK(vm, 2, 0);
	i = (int)ficlStackPopInteger(vm->dataStack);
	val = ficlStackPopUnsigned(vm->dataStack);
	fth_printf("%*lu ", i, val);
}

static void
ficl_d_dot(ficlVm *vm)
{
#define h_d_dot "( d -- )  number output\n\
17 d. => 17\n\
Print (Forth) double D (ficl2Integer).\n\
See also ud."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_printf("%lld ", ficlStackPop2Integer(vm->dataStack));
}

static void
ficl_ud_dot(ficlVm *vm)
{
#define h_ud_dot "( ud -- )  number output\n\
17 ud. => 17\n\
Print (Forth) unsigned double UD (ficl2Unsigned).\n\
See also d."
	FTH_STACK_CHECK(vm, 1, 0);
	fth_printf("%llu ", ficlStackPop2Unsigned(vm->dataStack));
}

static void
ficl_d_dot_r(ficlVm *vm)
{
#define h_d_dot_r "( d n -- )  formatted number output\n\
17 3 d.r => | 17 |\n\
Print (Forth) double D (ficl2Integer) \
in a right-adjusted field of N characters.\n\
See also ud.r"
	ficl2Integer val;
	int i;

	FTH_STACK_CHECK(vm, 2, 0);
	i = (int)ficlStackPopInteger(vm->dataStack);
	val = ficlStackPop2Integer(vm->dataStack);
	fth_printf("%*lld ", i, val);
}

static void
ficl_ud_dot_r(ficlVm *vm)
{
#define h_ud_dot_r "( ud n -- )  formatted number output\n\
17 3 ud.r => | 17 |\n\
Print (Forth) unsigned double UD (ficl2Unsigned) \
in a right-adjusted field of N characters.\n\
See also d.r"
	ficl2Unsigned val;
	int i;

	FTH_STACK_CHECK(vm, 2, 0);
	i = (int)ficlStackPopInteger(vm->dataStack);
	val = ficlStackPop2Unsigned(vm->dataStack);
	fth_printf("%*llu ", i, val);
}

static void
ficl_dnegate(ficlVm *vm)
{
#define h_dnegate "( x -- y )  y = -x"
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPush2Integer(vm->dataStack,
	    -ficlStackPop2Integer(vm->dataStack));
}

static void
ficl_dabs(ficlVm *vm)
{
#define h_dabs "( x -- y )  y = abs(x)"
	ficl2Integer x;

	FTH_STACK_CHECK(vm, 1, 1);
	x = ficlStackPop2Integer(vm->dataStack);
	ficlStackPush2Integer(vm->dataStack, (x < 0) ? -x : x);
}

static void
ficl_dmin(ficlVm *vm)
{
#define h_dmin "( x y -- z )  z = min(x, y)"
	ficl2Integer x;
	ficl2Integer y;

	FTH_STACK_CHECK(vm, 2, 1);
	y = ficlStackPop2Integer(vm->dataStack);
	x = ficlStackPop2Integer(vm->dataStack);
	ficlStackPush2Integer(vm->dataStack, (x < y) ? x : y);
}

static void
ficl_dmax(ficlVm *vm)
{
#define h_dmax "( x y -- z )  z = max(x, y)"
	ficl2Integer x;
	ficl2Integer y;

	FTH_STACK_CHECK(vm, 2, 1);
	y = ficlStackPop2Integer(vm->dataStack);
	x = ficlStackPop2Integer(vm->dataStack);
	ficlStackPush2Integer(vm->dataStack, (x > y) ? x : y);
}

static void
ficl_dtwostar(ficlVm *vm)
{
#define h_dtwostar "( x -- y )  y = x * 2"
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPush2Integer(vm->dataStack,
	    ficlStackPop2Integer(vm->dataStack) * 2);
}

static void
ficl_dtwoslash(ficlVm *vm)
{
#define h_dtwoslash "( x -- y )  y = x / 2"
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPush2Integer(vm->dataStack,
	    ficlStackPop2Integer(vm->dataStack) / 2);
}

N_FUNC_TEST_TWO_OP(ueq, ==, Unsigned);
N_FUNC_TEST_TWO_OP(unoteq, !=, Unsigned);
N_FUNC_TEST_TWO_OP(uless, <, Unsigned);
N_FUNC_TEST_TWO_OP(ulesseq, <=, Unsigned);
N_FUNC_TEST_TWO_OP(ugreater, >, Unsigned);
N_FUNC_TEST_TWO_OP(ugreatereq, >=, Unsigned);

N_FUNC_TEST_ZERO(dzero, ==, 2Integer);
N_FUNC_TEST_ZERO(dnotz, !=, 2Integer);
N_FUNC_TEST_ZERO(dlessz, <, 2Integer);
N_FUNC_TEST_ZERO(dlesseqz, <=, 2Integer);
N_FUNC_TEST_ZERO(dgreaterz, >, 2Integer);
N_FUNC_TEST_ZERO(dgreatereqz, >=, 2Integer);

N_FUNC_TEST_TWO_OP(deq, ==, 2Integer);
N_FUNC_TEST_TWO_OP(dnoteq, !=, 2Integer);
N_FUNC_TEST_TWO_OP(dless, <, 2Integer);
N_FUNC_TEST_TWO_OP(dlesseq, <=, 2Integer);
N_FUNC_TEST_TWO_OP(dgreater, >, 2Integer);
N_FUNC_TEST_TWO_OP(dgreatereq, >=, 2Integer);

N_FUNC_TEST_TWO_OP(dueq, ==, 2Unsigned);
N_FUNC_TEST_TWO_OP(dunoteq, !=, 2Unsigned);
N_FUNC_TEST_TWO_OP(duless, <, 2Unsigned);
N_FUNC_TEST_TWO_OP(dulesseq, <=, 2Unsigned);
N_FUNC_TEST_TWO_OP(dugreater, >, 2Unsigned);
N_FUNC_TEST_TWO_OP(dugreatereq, >=, 2Unsigned);

N_FUNC_TWO_ARGS_OP(dadd, +, 2Integer);
N_FUNC_TWO_ARGS_OP(dsub, -, 2Integer);
N_FUNC_TWO_ARGS_OP(dmul, *, 2Integer);
N_FUNC_TWO_ARGS_OP(ddiv, /, 2Integer);

/* === FLOAT === */

#define h_list_of_float_functions "\
*** FLOAT PRIMITIVES ***\n\
float?    inf?      nan?\n\
inf       nan\n\
f.r       uf.r\n\
floats    (sfloats and dfloats)\n\
falign    f>s       s>f\n\
f**       (fpow)    fabs\n\
fmod      floor     fceil     ftrunc\n\
fround    fsqrt     fexp      fexpm1\n\
flog      flogp1    flog2     flog10    falog\n\
fsin      fcos      ftan      fsincos\n\
fasin     facos     fatan     fatan2\n\
fsinh     fcosh     ftanh\n\
fasinh    facosh    fatanh\n\
*** FLOAT CONSTANTS ***\n\
euler  ln-two  ln-ten  pi  two-pi  half-pi  sqrt-two"

/*
 * XXX: ficl_float_precision
 */
#if 0
/* defined in ficl/primitives.c */
extern int ficl_float_precision;

static char *
format_double(char *buf, size_t size, ficlFloat f)
{
	snprintf(buf, size, "%.*f", ficl_float_precision, f);
	return (buf);
}
#else
static char *
format_double(char *buf, size_t size, ficlFloat f)
{
	int i, len, isize;
	bool okay;

	len = snprintf(buf, size, "%g", f);
	okay = false;
	for (i = 0; i < len; i++) {
		if (buf[i] == 'e' || buf[i] == '.') {
			okay = true;
			break;
		}
	}
	isize = (int)size;
	if (!okay && (len + 2) < isize)
		buf[len] = '.';
		buf[len + 1] = '0';
		buf[len + 2] = '\0';
	return (buf);
}
#endif

static char numbers_scratch[BUFSIZ];

static FTH
fl_inspect(FTH self)
{
	ficlFloat fl;
	FTH fs;

	fl = FTH_FLOAT_OBJECT(self);
	fs = fth_make_string_format("%s: ", FTH_INSTANCE_NAME(self));
	if (fth_isnan(fl))
		fth_string_sformat(fs, "NaN");
	else if (fth_isinf(fl))
		fth_string_sformat(fs, "%sInfinite", fl < 0 ? "-" : "");
	else
		fth_string_sformat(fs, "%s",
		    format_double(numbers_scratch,
		    sizeof(numbers_scratch), fl));
	return (fs);
}

static FTH
fl_to_string(FTH self)
{
	ficlFloat fl;

	fl = FTH_FLOAT_OBJECT(self);
	if (fth_isnan(fl))
		return (fth_make_string("#<nan>"));
	if (fth_isinf(fl))
		return (fth_make_string("#<inf>"));
	return (fth_make_string(format_double(numbers_scratch,
	    sizeof(numbers_scratch), fl)));
}

static FTH
fl_copy(FTH self)
{
	return (fth_make_float(FTH_FLOAT_OBJECT(self)));
}

static FTH
fl_equal_p(FTH self, FTH obj)
{
	return (BOOL_TO_FTH(FTH_FLOAT_OBJECT(self) == FTH_FLOAT_OBJECT(obj)));
}

/*
 * Return a FTH float object from F.
 */
FTH
fth_make_float(ficlFloat f)
{
	FTH self;

	self = fth_make_instance(float_tag, NULL);
	FTH_FLOAT_OBJECT_SET(self, f);
	return (self);
}

FTH
fth_float_copy(FTH obj)
{
	if (FTH_FLOAT_P(obj))
		return (fl_copy(obj));
	return (obj);
}

static void
ficl_float_p(ficlVm *vm)
{
#define h_float_p "( obj -- f )  test if OBJ is a float number\n\
nil float? => #f\n\
1   float? => #f\n\
1.0 float? => #t\n\
Return #t if OBJ is a float object, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_FLOAT_P(obj));
}

static void
ficl_inf_p(ficlVm *vm)
{
#define h_inf_p "( obj -- f )  test if OBJ is Infinite\n\
nil inf? => #f\n\
0   inf? => #f\n\
inf inf? => #t\n\
Return #t if OBJ is Infinite, otherwise #f.\n\
See also nan?, inf, nan."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushBoolean(vm->dataStack,
	    fth_isinf(fth_float_ref(fth_pop_ficl_cell(vm))));
}

static void
ficl_nan_p(ficlVm *vm)
{
#define h_nan_p "( obj -- f )  test if OBJ is Not a Number\n\
nil nan? => #f\n\
0   nan? => #f\n\
nan nan? => #t\n\
Return #t if OBJ is Not a Number, otherwise #f.\n\
See also inf?, inf, nan."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushBoolean(vm->dataStack,
	    fth_isnan(fth_float_ref(fth_pop_ficl_cell(vm))));
}

static void
ficl_inf(ficlVm *vm)
{
#define h_inf "( -- Inf )  Return Infinity."
	ficlStackPushFloat(vm->dataStack, FTH_INF);
}

static void
ficl_nan(ficlVm *vm)
{
#define h_nan "( -- NaN )  Return Not-A-Number."
	ficlStackPushFloat(vm->dataStack, FTH_NAN);
}

static void
ficl_f_dot_r(ficlVm *vm)
{
#define h_f_dot_r "( r n -- )  formatted number output\n\
17.0 3 f.r => |17.000 |\n\
17.0 6 f.r => |17.000000 |\n\
Print float R with N digits after decimal point."
	ficlFloat val;
	int i;

	FTH_STACK_CHECK(vm, 2, 0);
	i = (int)ficlStackPopInteger(vm->dataStack);
	val = ficlStackPopFloat(vm->dataStack);
	fth_printf("%.*f ", i, val);
}

static void
ficl_uf_dot_r(ficlVm *vm)
{
#define h_uf_dot_r "( r len-all len-after-comma -- )  formatted number\n\
17.0 8 3 uf.r => | 17.000 |\n\
17.0 8 2 uf.r => |  17.00 |\n\
Print float R in a right-adjusted field of LEN-ALL characters \
with LEN-AFTER-COMMA digits."
	ficlFloat val;
	int i, all;

	FTH_STACK_CHECK(vm, 2, 1);
	i = (int)ficlStackPopInteger(vm->dataStack);
	all = (int)ficlStackPopInteger(vm->dataStack);
	val = ficlStackPopFloat(vm->dataStack);
	fth_printf("%*.*f ", all, i, val);
}

static void
ficl_dfloats(ficlVm *vm)
{
#define h_dfloats "( n1 -- n2 )  return address units\n\
1 dfloats => 8\n\
4 dfloats => 32\n\
N2 is the number of address units of N1 dfloats (double)."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushInteger(vm->dataStack,
	    ficlStackPopInteger(vm->dataStack) *
	    (ficlInteger)sizeof(ficlFloat));
}

static void
ficl_falign(ficlVm *vm)
{
#define h_falign "( -- )  align dictionary"
	ficlDictionaryAlign(ficlVmGetDictionary(vm));
}

static void
ficl_fexpm1(ficlVm *vm)
{
#define h_fexpm1 "( x -- y )  y = exp(x) - 1.0"
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushFloat(vm->dataStack,
	    exp(ficlStackPopFloat(vm->dataStack)) - 1.0);
}

static void
ficl_flogp1(ficlVm *vm)
{
#define h_flogp1 "( x -- y )  y = log(x + 1.0)"
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushFloat(vm->dataStack,
	    fth_log(ficlStackPopFloat(vm->dataStack) + 1.0));
}

static void
ficl_falog(ficlVm *vm)
{
#define h_falog "( x -- y )  y = pow(10.0, x)"
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushFloat(vm->dataStack,
	    FTH_POW(10.0, ficlStackPopFloat(vm->dataStack)));
}

static void
ficl_fsincos(ficlVm *vm)
{
#define h_fsincos "( x -- y z )  y = sin(x), z = cos(x)"
	ficlFloat val;

	FTH_STACK_CHECK(vm, 1, 2);
	val = ficlStackPopFloat(vm->dataStack);
	ficlStackPushFloat(vm->dataStack, sin(val));
	ficlStackPushFloat(vm->dataStack, cos(val));
}

N_FUNC_ONE_ARG(fabs, fabs, Float);
N_FUNC_ONE_ARG(floor, floor, Float);
N_FUNC_ONE_ARG(fceil, ceil, Float);
N_FUNC_ONE_ARG(ftrunc, trunc, Float);
N_FUNC_ONE_ARG(fround, rint, Float);
N_FUNC_ONE_ARG(fsqrt, sqrt, Float);
N_FUNC_ONE_ARG(fexp, exp, Float);
N_FUNC_ONE_ARG(flog, fth_log, Float);
N_FUNC_ONE_ARG(flog2, fth_log2, Float);
N_FUNC_ONE_ARG(flog10, fth_log10, Float);
N_FUNC_ONE_ARG(fsin, sin, Float);
N_FUNC_ONE_ARG(fcos, cos, Float);
N_FUNC_ONE_ARG(ftan, tan, Float);
N_FUNC_ONE_ARG(fasin, asin, Float);
N_FUNC_ONE_ARG(facos, acos, Float);
N_FUNC_ONE_ARG(fatan, atan, Float);
N_FUNC_ONE_ARG(fsinh, sinh, Float);
N_FUNC_ONE_ARG(fcosh, cosh, Float);
N_FUNC_ONE_ARG(ftanh, tanh, Float);
N_FUNC_ONE_ARG(fasinh, asinh, Float);
N_FUNC_ONE_ARG(facosh, acosh, Float);
N_FUNC_ONE_ARG(fatanh, atanh, Float);
N_FUNC_TWO_ARGS(fmod, fmod, Float);
N_FUNC_TWO_ARGS(fpow, pow, Float);
N_FUNC_TWO_ARGS(fatan2, atan2, Float);

/*
 * Parse ficlInteger, ficl2Integer, ficlUnsigned, ficl2Unsigned, and 
 * ficlFloat (1, 1., 1.0, 1e, etc).
 */
int
ficl_parse_number(ficlVm *vm, ficlString s)
{
	int flag, base;
	bool allocated;
	char *test, *str;
	ficlInteger i;
	ficlUnsigned u;
	ficl2Integer di;
	ficl2Unsigned ud;
	ficlFloat f;

	if (s.length < 1)
		return (FICL_FALSE);

	flag = FICL_TRUE;
	base = (int)vm->base;
	if (s.length >= FICL_PAD_SIZE) {
		str = FTH_MALLOC(s.length + 1);
		allocated = true;
	} else {
		str = vm->pad;
		allocated = false;
	}
	strncpy(str, s.text, s.length);
	str[s.length] = '\0';
	/* ficlInteger */
	i = strtol(str, &test, base);
	if (*test == '\0' && errno != ERANGE) {
		ficlStackPushInteger(vm->dataStack, i);
		goto finish;
	}
	/* 3e => 3. */
	if (str[s.length - 1] == 'e')
		str[s.length - 1] = '.';
	/* ficlFloat */
	f = strtod(str, &test);
	if (*test == '\0' && errno != ERANGE) {
		ficlStackPushFloat(vm->dataStack, f);
		goto finish;
	}
	/* ficl2Integer */
	di = strtoll(str, &test, base);
	if (*test == '\0' && errno != ERANGE) {
		ficlStackPush2Integer(vm->dataStack, di);
		goto finish;
	}
	/* ficlUnsigned */
	u = strtoul(str, &test, base);
	if (*test == '\0' && errno != ERANGE) {
		ficlStackPushUnsigned(vm->dataStack, u);
		goto finish;
	}
	/* ficl2Unsigned */
	ud = strtoull(str, &test, base);
	if (*test == '\0' && errno != ERANGE) {
		ficlStackPush2Unsigned(vm->dataStack, ud);
		goto finish;
	}
	flag = FICL_FALSE;
finish:
	if (allocated)
		FTH_FREE(str);
	errno = 0;
	if (flag && vm->state == FICL_VM_STATE_COMPILE)
		ficlPrimitiveLiteralIm(vm);
	return (flag);
}

/* === COMPLEX === */

#if HAVE_COMPLEX

#define h_list_of_complex_functions "\
*** COMPLEX PRIMITIVES ***\n\
complex?  real-ref  imag-ref (image-ref)\n\
make-rectangular    alias >complex\n\
make-polar\n\
c.   s>c  c>s  f>c  c>f  q>c  >c\n\
c0=  c=<> c0<  c0<= c0>  c0>=\n\
c=   c<>  c<   c<=  c>   c>=\n\
c+   c-   c*   c/   1/c\n\
carg      cabs (magnitude)    cabs2\n\
c**  (cpow)    conj (conjugate)\n\
csqrt     cexp      clog      clog10\n\
csin      ccos      ctan\n\
casin     cacos     catan     catan2\n\
csinh     ccosh     ctanh\n\
casinh    cacosh    catanh\n\
See also long-long and float."

static char numbers_scratch_02[BUFSIZ];

static FTH
cp_inspect(FTH self)
{
	char *re, *im;
	size_t size;
	
	re = numbers_scratch;
	im = numbers_scratch_02;
	size = sizeof(numbers_scratch);
	return (fth_make_string_format("%s: real %s, image %s",
	    FTH_INSTANCE_NAME(self),
	    format_double(re, size, FTH_COMPLEX_REAL(self)),
	    format_double(im, size, FTH_COMPLEX_IMAG(self))));
}

static FTH
cp_to_string(FTH self)
{
	char *re, *im;
	size_t size;
	
	re = numbers_scratch;
	im = numbers_scratch_02;
	size = sizeof(numbers_scratch);
	return (fth_make_string_format("%s%s%si",
	    format_double(re, size, FTH_COMPLEX_REAL(self)),
	    FTH_COMPLEX_IMAG(self) >= 0.0 ? "+" : "",
	    format_double(im, size, FTH_COMPLEX_IMAG(self))));
}

static FTH
cp_copy(FTH self)
{
	return (fth_make_complex(FTH_COMPLEX_OBJECT(self)));
}

static FTH
cp_equal_p(FTH self, FTH obj)
{
	return (BOOL_TO_FTH(FTH_COMPLEX_REAL(self) == FTH_COMPLEX_REAL(obj) &&
	    FTH_COMPLEX_IMAG(self) == FTH_COMPLEX_IMAG(obj)));
}

#endif				/* HAVE_COMPLEX */

static void
ficl_complex_p(ficlVm *vm)
{
#define h_complex_p "( obj -- f )  test if OBJ is a complex number\n\
nil complex? => #f\n\
1   complex? => #f\n\
1+i complex? => #t\n\
Return #t if OBJ is a complex object, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_COMPLEX_P(obj));
}

static void
ficl_creal(ficlVm *vm)
{
#define h_creal "( numb -- re )  return number's real part\n\
1   real-ref => 1.0\n\
1.0 real-ref => 1.0\n\
1+i real-ref => 1.0\n\
Return the real part of NUMB.\n\
See also imag-ref."
	FTH obj;
	ficlFloat f;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
#if HAVE_COMPLEX
	if (FTH_COMPLEX_P(obj))
		f = FTH_COMPLEX_REAL(obj);
	else
		f = fth_float_ref(obj);
#else
	f = fth_float_ref(obj);
#endif
	ficlStackPushFloat(vm->dataStack, f);
}

static void
ficl_cimage(ficlVm *vm)
{
#define h_cimage "( numb -- im )  return number's image part\n\
1   imag-ref => 0.0\n\
1.0 imag-ref => 0.0\n\
1+i imag-ref => 1.0\n\
Return the image part of NUMB.\n\
See also real-ref."
	FTH obj;
	ficlFloat f;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	f = 0.0;
#if HAVE_COMPLEX
	if (FTH_COMPLEX_P(obj))
		f = FTH_COMPLEX_IMAG(obj);
#endif
	ficlStackPushFloat(vm->dataStack, f);
}

#if HAVE_COMPLEX

/*
 * Return a FTH complex object from Z.
 */
FTH
fth_make_complex(ficlComplex z)
{
	FTH self;

	self = fth_make_instance(complex_tag, NULL);
	FTH_COMPLEX_OBJECT_SET(self, z);
	return (self);
}

FTH
fth_make_rectangular(ficlFloat real, ficlFloat image)
{
	return (fth_make_complex(real + image * _Complex_I));
}

static void
ficl_make_complex_rectangular(ficlVm *vm)
{
#define h_make_complex_rectangular "( real image -- complex )  complex numb\n\
1 1 make-rectangular => 1.0+1.0i\n\
Return complex object with REAL and IMAGE part.\n\
See also make-polar."
	ficlFloat real, image;

	FTH_STACK_CHECK(vm, 2, 1);
	image = fth_float_ref(fth_pop_ficl_cell(vm));
	real = fth_float_ref(fth_pop_ficl_cell(vm));
	ficlStackPushFTH(vm->dataStack, fth_make_rectangular(real, image));
}

static ficlComplex
make_polar(ficlFloat real, ficlFloat theta)
{
	return (real * cos(theta) + real * sin(theta) * _Complex_I);
}

FTH
fth_make_polar(ficlFloat real, ficlFloat theta)
{
	return (fth_make_complex(make_polar(real, theta)));
}

static void
ficl_make_complex_polar(ficlVm *vm)
{
#define h_make_complex_polar "( real theta -- complex )  polar complex numb\n\
1 1 make-polar => 0.540302+0.841471i\n\
Return polar complex object from REAL and THETA.\n\
See also make-rectangular."
	ficlFloat real, theta;

	FTH_STACK_CHECK(vm, 2, 1);
	theta = fth_float_ref(fth_pop_ficl_cell(vm));
	real = fth_float_ref(fth_pop_ficl_cell(vm));
	ficlStackPushFTH(vm->dataStack, fth_make_polar(real, theta));
}

static void
ficl_c_dot(ficlVm *vm)
{
#define h_c_dot "( c -- )  print number\n\
1+i c. => |1.0+1.0i |\n\
Print complex number C."
	ficlComplex cp;

	FTH_STACK_CHECK(vm, 1, 0);
	cp = ficlStackPopComplex(vm->dataStack);
	fth_printf("%f%s%fi ",
	    creal(cp),
	    cimag(cp) >= 0.0 ? "+" : "",
	    cimag(cp));
}

static void
ficl_creciprocal(ficlVm *vm)
{
#define h_creciprocal "( x -- y )  y = 1 / x"
	ficlComplex cp;

	FTH_STACK_CHECK(vm, 1, 1);
	cp = ficlStackPopComplex(vm->dataStack);
	ficlStackPushComplex(vm->dataStack, 1.0 / cp);
}

static void
ficl_ceqz(ficlVm *vm)
{
#define h_ceqz "( x -- f )  x == 0 => flag"
	ficlComplex cp;

	FTH_STACK_CHECK(vm, 1, 1);
	cp = ficlStackPopComplex(vm->dataStack);
	if (creal(cp) == 0.0)
		ficlStackPushBoolean(vm->dataStack, cimag(cp) == 0.0);
	else
		ficlStackPushBoolean(vm->dataStack, creal(cp) == 0.0);
}

static void
ficl_cnoteqz(ficlVm *vm)
{
#define h_cnoteqz "( x -- f )  x != 0 => flag"
	ficlComplex cp;

	FTH_STACK_CHECK(vm, 1, 1);
	cp = ficlStackPopComplex(vm->dataStack);
	if (creal(cp) == 0.0)
		ficlStackPushBoolean(vm->dataStack, cimag(cp) != 0.0);
	else
		ficlStackPushBoolean(vm->dataStack, creal(cp) != 0.0);
}

static void
ficl_ceq(ficlVm *vm)
{
#define h_ceq "( x y -- f )  x == y => flag"
	ficlComplex x, y;

	FTH_STACK_CHECK(vm, 2, 1);
	y = ficlStackPopComplex(vm->dataStack);
	x = ficlStackPopComplex(vm->dataStack);
	if (creal(x) == creal(y))
		ficlStackPushBoolean(vm->dataStack, cimag(x) == cimag(y));
	else
		ficlStackPushBoolean(vm->dataStack, false);
}

static void
ficl_cnoteq(ficlVm *vm)
{
#define h_cnoteq "( x y -- f )  x != y => flag"
	ficlComplex x, y;

	FTH_STACK_CHECK(vm, 2, 1);
	y = ficlStackPopComplex(vm->dataStack);
	x = ficlStackPopComplex(vm->dataStack);
	if (creal(x) == creal(y))
		ficlStackPushBoolean(vm->dataStack, cimag(x) != cimag(y));
	else
		ficlStackPushBoolean(vm->dataStack, true);
}

N_FUNC_TWO_ARGS_OP(cadd, +, Complex);
N_FUNC_TWO_ARGS_OP(csub, -, Complex);
N_FUNC_TWO_ARGS_OP(cmul, *, Complex);
N_FUNC_TWO_ARGS_OP(cdiv, /, Complex);

N_FUNC_ONE_ARG(carg, carg, Complex);
N_FUNC_ONE_ARG(cabs, cabs, Complex);
N_FUNC_ONE_ARG(cabs2, cabs2, Complex);
N_FUNC_TWO_ARGS(cpow, cpow, Complex);
N_FUNC_ONE_ARG(cconj, conj, Complex);
N_FUNC_ONE_ARG(csqrt, csqrt, Complex);
N_FUNC_ONE_ARG(cexp, cexp, Complex);
N_FUNC_ONE_ARG(clog, clog, Complex);
N_FUNC_ONE_ARG(clog10, clog10, Complex);
N_FUNC_ONE_ARG(csin, csin, Complex);
N_FUNC_ONE_ARG(ccos, ccos, Complex);
N_FUNC_ONE_ARG(ctan, ctan, Complex);
N_FUNC_ONE_ARG(casin, casin, Complex);
N_FUNC_ONE_ARG(cacos, cacos, Complex);
N_FUNC_ONE_ARG(catan, catan, Complex);
N_FUNC_TWO_ARGS(catan2, catan2, Complex);
N_FUNC_ONE_ARG(csinh, csinh, Complex);
N_FUNC_ONE_ARG(ccosh, ccosh, Complex);
N_FUNC_ONE_ARG(ctanh, ctanh, Complex);
N_FUNC_ONE_ARG(casinh, casinh, Complex);
N_FUNC_ONE_ARG(cacosh, cacosh, Complex);
N_FUNC_ONE_ARG(catanh, catanh, Complex);

/*
 * Parse ficlComplex (1i, 1-i, -1+1i, 1.0+1.0i, etc).
 */
int
ficl_parse_complex(ficlVm *vm, ficlString s)
{
	ficlFloat re, im;
	size_t loc_len;
	char *locp, *locn, *test, *loc;
	char re_buf[FICL_PAD_SIZE];
	char *sreal, *simag;
	int flag;
	bool allocated;

	if (s.length < 2 || tolower((int)s.text[s.length - 1]) != 'i')
		return (FICL_FALSE);

	flag = FICL_FALSE;
	if (s.length >= FICL_PAD_SIZE) {
		sreal = FTH_MALLOC(s.length + 1);
		simag = FTH_MALLOC(s.length + 1);
		allocated = true;
	} else {
		sreal = re_buf;
		simag = vm->pad;
		allocated = false;
	}
	strncpy(simag, s.text, s.length);
	simag[s.length] = '\0';
	locp = strrchr(simag, '+');
	locn = strrchr(simag, '-');
	loc = FICL_MAX(locp, locn);
	if (loc == NULL) {
		loc = strrchr(simag, 'i');
		if (loc == NULL)
			loc = strrchr(simag, 'I');
	}
	if (loc == NULL)
		goto finish;
	strncpy(sreal, simag, (size_t)(loc - simag));
	sreal[loc - simag] = '\0';
	re = strtod(sreal, &test);
	if (*test != '\0' || errno == ERANGE)
		goto finish;
	loc_len = fth_strlen(loc);	/* skip \0 above */
	if (loc_len > 2) {
		loc[loc_len - 1] = '\0';	/* discard trailing i */
		im = strtod(loc, &test);
		if (*test != '\0' || errno == ERANGE)
			goto finish;
	} else {
		if (loc[0] == '+' || tolower((int)loc[0]) == 'i')
			im = 1.0;
		else
			im = -1.0;
	}
	ficlStackPushFTH(vm->dataStack, fth_make_rectangular(re, im));
	if (vm->state == FICL_VM_STATE_COMPILE)
		ficlPrimitiveLiteralIm(vm);
	flag = FICL_TRUE;
finish:
	if (allocated) {
		FTH_FREE(sreal);
		FTH_FREE(simag);
	}
	errno = 0;
	return (flag);
}

#endif				/* HAVE_COMPLEX */

/* === BIGNUM via bn(3) === */

#define h_list_of_bignum_functions "\
*** BIGNUMB PRIMITIVES ***\n\
bignum?  >bignum\n\
s>b  b>s  f>b  b>f\n\
b0=  b0<> b0<  b0>  b0<= b0>=\n\
b=   b<>  b<   b>   b<=  b>=\n\
b+   b-   b*   b/   b** (bpow)\n\
bnegate   babs"

static void
ficl_bignum_p(ficlVm *vm)
{
#define h_bignum_p "( obj -- f )  test if OBJ is a bignum\n\
nil bignum? => #f\n\
1e100 bignum? => #f\n\
12345678901234567890 bignum? => #t\n\
Return #t if OBJ is a bignum object, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_BIGNUM_P(obj));
}

#if HAVE_BN

static FTH
bn_inspect(FTH self)
{
	return (fth_make_string_format("%s: %S",
	    FTH_INSTANCE_NAME(self), bn_to_string(self)));
}

static FTH
bn_to_string(FTH self)
{
	FTH fs;
	char *buf;

	buf = BN_bn2dec(FTH_BIGNUM_OBJECT(self));
	BN_CHECKP(buf);
	fs = fth_make_string(buf);
	OPENSSL_free(buf);
	return (fs);
}

static FTH
bn_copy(FTH self)
{
	ficlBignum res;

	res = BN_new();
	BN_CHECKP(res);
	BN_CHECKP(BN_copy(res, FTH_BIGNUM_OBJECT(self)));
	return (fth_make_bignum(res));
}

static FTH
bn_equal_p(FTH self, FTH obj)
{
	return (BOOL_TO_FTH(BN_cmp(FTH_BIGNUM_OBJECT(self),
	    FTH_BIGNUM_OBJECT(obj)) == 0));
}

static void
bn_free(FTH self)
{
	BN_free(FTH_BIGNUM_OBJECT(self));
}

static BN_CTX *
bn_ctx_init(void)
{
	BN_CTX *ctx;

	ctx = BN_CTX_new();
	BN_CHECKP(ctx);
	BN_CTX_init(ctx);
	return (ctx);
}

static void 
int_to_bn(ficlBignum m, ficlInteger x)
{
	if (x < 0) {
		BN_CHECK(BN_set_word(m, -(unsigned long)x));
		BN_set_negative(m, 1);
	} else
		BN_CHECK(BN_set_word(m, (unsigned long)x));

}

static void
float_to_bn(ficlBignum m, ficlFloat x)
{
	char *buf;

	buf = fth_format("%.0f", FTH_ROUND(x));
	BN_CHECK(BN_dec2bn(&m, buf));
	FTH_FREE(buf);
}

static void
fth_to_bn(ficlBignum m, FTH x)
{
	int type = -1;

	if (FTH_NUMBER_T_P(x))
		type = FTH_INSTANCE_TYPE(x);
	switch (type) {
	case FTH_LLONG_T:
		int_to_bn(m, (ficlInteger)FTH_LONG_OBJECT(x));
		break;
	case FTH_FLOAT_T:
		float_to_bn(m, FTH_FLOAT_OBJECT(x));
		break;
	case FTH_BIGNUM_T:
		BN_CHECKP(BN_copy(m, FTH_BIGNUM_OBJECT(x)));
		break;
	case FTH_COMPLEX_T:
	case FTH_RATIO_T:
		float_to_bn(m, fth_float_ref(x));
		break;
	default:
		int_to_bn(m, fth_int_ref(x));
		break;
	}	
}

static ficlInteger
bn_to_int(ficlBignum m)
{
	unsigned long x;

	x = BN_get_word(m);
	if (BN_is_negative(m))
		return (-(ficlInteger)x);
	return ((ficlInteger)x);
}

enum {
	BN_ADD,
	BN_SUB,
	BN_MUL,
	BN_DIV
};

static ficlBignum
bn_addsub(FTH m, FTH n, int type)
{
	ficlBignum res;
	BIGNUM x, y;

	res = BN_new();
	BN_CHECKP(res);
	BN_init(&x);
	BN_init(&y);
	fth_to_bn(&x, m);
	fth_to_bn(&y, n);
	if (type == BN_ADD)
		BN_CHECK(BN_add(res, &x, &y));
	else
		BN_CHECK(BN_sub(res, &x, &y));
	BN_free(&x);
	BN_free(&y);
	return (res);
}

static ficlBignum
bn_muldiv(FTH m, FTH n, int type)
{
	ficlBignum res;
	BIGNUM x, y;
	BN_CTX *ctx;

	res = BN_new();
	BN_CHECKP(res);
	ctx = bn_ctx_init();
	BN_init(&x);
	BN_init(&y);
	fth_to_bn(&x, m);
	fth_to_bn(&y, n);
	if (type == BN_MUL)
		BN_CHECK(BN_mul(res, &x, &y, ctx));
	else
		BN_CHECK(BN_div(res, NULL, &x, &y, ctx));
	BN_CTX_free(ctx);
	BN_free(&x);
	BN_free(&y);
	return (res);
}

static FTH
bn_add(FTH m, FTH n)
{
	return (fth_make_bignum(bn_addsub(m, n, BN_ADD)));
}

static FTH
bn_sub(FTH m, FTH n)
{
	return (fth_make_bignum(bn_addsub(m, n, BN_SUB)));
}

static FTH
bn_mul(FTH m, FTH n)
{
	return (fth_make_bignum(bn_muldiv(m, n, BN_MUL)));
}

static FTH
bn_div(FTH m, FTH n)
{
	return (fth_make_bignum(bn_muldiv(m, n, BN_DIV)));
}

FTH
fth_make_bignum(ficlBignum m)
{
	FTH self;

	self = fth_make_instance(bignum_tag, NULL);
	FTH_BIGNUM_OBJECT_SET(self, m);
	return (self);
}

FTH
fth_make_big(FTH m)
{
#define h_make_big "( numb -- b )  NUMB to bignum\n\
1 make-bignum => 1\n\
1 make-bignum bignum? => #t\n\
Return a new bignum object from NUMB."
	ficlBignum res;

	res = BN_new();
	BN_CHECKP(res);
	if (FTH_BIGNUM_P(m))
		BN_CHECKP(BN_copy(res, FTH_BIGNUM_OBJECT(m)));
	else
		fth_to_bn(res, m);
	return (fth_make_bignum(res));
}

static void
ficl_bn_dot(ficlVm *vm)
{
#define h_bn_dot "( numb -- )  number output\n\
1 >bignum bn. => 1\n\
Print bignum number NUMB with space added."
	BIGNUM x;
	char *str;

	FTH_STACK_CHECK(vm, 1, 0);
	BN_init(&x);
	fth_to_bn(&x, fth_pop_ficl_cell(vm));
	str = BN_bn2dec(&x);
	BN_CHECKP(str);
	fth_printf("%s ", str);
	OPENSSL_free(str);
	BN_free(&x);

}

#define N_BIGNUM_FUNC_TEST_ZERO(Name, OP)				\
static bool								\
fth_bn_ ## Name(FTH m)							\
{									\
	bool flag; 							\
									\
	if (FTH_BIGNUM_P(m))						\
		flag = (BN_cmp(FTH_BIGNUM_OBJECT(m), &fth_bn_zero) OP 0); \
	else {								\
		BIGNUM x;						\
									\
		BN_init(&x);						\
		fth_to_bn(&x, m);					\
		flag = (BN_cmp(&x, &fth_bn_zero) OP 0);			\
		BN_free(&x);						\
	}								\
	return (flag);							\
}									\
static void								\
ficl_ ## Name(ficlVm *vm)						\
{									\
	FTH_STACK_CHECK(vm, 1, 1);					\
	ficlStackPushBoolean(vm->dataStack,				\
	    fth_bn_ ## Name(fth_pop_ficl_cell(vm)));			\
}									\
static char* h_ ## Name = "( x -- f )  x " #OP " 0 => flag (bignum)"

/*
 * build:
 *   bool fth_bn_beqz(FTH m) ...    for C fth_number_equal_p etc
 *   void ficl_beqz(ficlVm *vm) ... for Forth words
 */
N_BIGNUM_FUNC_TEST_ZERO(beqz, ==);
N_BIGNUM_FUNC_TEST_ZERO(bnoteqz, !=);
N_BIGNUM_FUNC_TEST_ZERO(blessz, <);
N_BIGNUM_FUNC_TEST_ZERO(bgreaterz, >);
N_BIGNUM_FUNC_TEST_ZERO(blesseqz, <=);
N_BIGNUM_FUNC_TEST_ZERO(bgreatereqz, >=);

#define N_BIGNUM_FUNC_TEST_TWO_OP(Name, OP)				\
static bool								\
fth_bn_ ## Name(FTH m, FTH n)						\
{									\
	bool flag;	 						\
									\
	if (FTH_BIGNUM_P(m)) {						\
		if (FTH_BIGNUM_P(n))					\
			flag = (BN_cmp(FTH_BIGNUM_OBJECT(m),		\
			    FTH_BIGNUM_OBJECT(n)) OP 0);		\
		else {							\
			BIGNUM y;					\
									\
			BN_init(&y);					\
			fth_to_bn(&y, n);				\
			flag = (BN_cmp(FTH_BIGNUM_OBJECT(m), &y) OP 0);	\
			BN_free(&y);					\
		}							\
	} else if (FTH_BIGNUM_P(n)) {					\
		BIGNUM x;						\
									\
		BN_init(&x);						\
		fth_to_bn(&x, m);					\
		flag = (BN_cmp(&x, FTH_BIGNUM_OBJECT(n)) OP 0);		\
		BN_free(&x);						\
	} else {							\
		BIGNUM x, y;						\
									\
		BN_init(&x);						\
		BN_init(&y);						\
		fth_to_bn(&x, m);					\
		fth_to_bn(&y, n);					\
		flag = (BN_cmp(&x, &y) OP 0);				\
		BN_free(&x);						\
		BN_free(&y);						\
	}								\
	return (flag);							\
}									\
static void								\
ficl_ ## Name(ficlVm *vm)						\
{									\
	FTH m, n;							\
									\
	FTH_STACK_CHECK(vm, 2, 1);					\
	n = fth_pop_ficl_cell(vm);					\
	m = fth_pop_ficl_cell(vm);					\
	ficlStackPushBoolean(vm->dataStack,				\
	    fth_bn_ ## Name(m, n));					\
}									\
static char* h_ ## Name = "( x y -- f )  x " #OP " y => flag (bignum)"

/*
 * build: 
 *   bool fth_bn_beq(FTH m, FTH n) ... for C fth_number_equal_p etc
 *   void ficl_beq(ficlVm *vm) ...     for Forth words
 */
N_BIGNUM_FUNC_TEST_TWO_OP(beq, ==);
N_BIGNUM_FUNC_TEST_TWO_OP(bnoteq, !=);
N_BIGNUM_FUNC_TEST_TWO_OP(bless, <);
N_BIGNUM_FUNC_TEST_TWO_OP(bgreater, >);
N_BIGNUM_FUNC_TEST_TWO_OP(blesseq, <=);
N_BIGNUM_FUNC_TEST_TWO_OP(bgreatereq, >=);

#define N_BIGNUM_MATH_FUNC_OP(Name, OP, FName)				\
static void								\
ficl_ ## Name(ficlVm *vm)						\
{									\
	FTH m, n;							\
									\
	FTH_STACK_CHECK(vm, 2, 1);					\
	n = fth_pop_ficl_cell(vm);					\
	m = fth_pop_ficl_cell(vm);					\
	ficlStackPushFTH(vm->dataStack, FName(m, n));			\
}									\
static char* h_ ## Name = "( x y -- z )  z = x " #OP " y (bignum/ratio)"

N_BIGNUM_MATH_FUNC_OP(badd, +, bn_add);
N_BIGNUM_MATH_FUNC_OP(bsub, -, bn_sub);
N_BIGNUM_MATH_FUNC_OP(bmul, *, bn_mul);
N_BIGNUM_MATH_FUNC_OP(bdiv, /, bn_div);

static void
ficl_bpow(ficlVm *vm)
{
#define h_bpow "( x y -- z )  z = x ** y"
	FTH m, n;
	ficlBignum res;
	BIGNUM x, y;
	BN_CTX *ctx;

	FTH_STACK_CHECK(vm, 2, 1);
	n = fth_pop_ficl_cell(vm);
	m = fth_pop_ficl_cell(vm);
	res = BN_new();
	BN_CHECKP(res);
	ctx = bn_ctx_init();
	BN_init(&x);
	BN_init(&y);
	fth_to_bn(&x, m);
	fth_to_bn(&y, n);
	BN_CHECK(BN_exp(res, &x, &y, ctx));
	BN_CTX_free(ctx);
	BN_free(&x);
	BN_free(&y);
	ficlStackPushFTH(vm->dataStack, fth_make_bignum(res));
}

static void
ficl_bnegate(ficlVm *vm)
{
	ficlBignum res;

	FTH_STACK_CHECK(vm, 1, 1);
	res = BN_new();
	if (res == NULL) {
		BN_CHECKP(res);
		/* for ccc-analyzer */
		/* NOTREACHED */
		return;
	}
	fth_to_bn(res, fth_pop_ficl_cell(vm)); 
	BN_set_negative(res, !BN_is_negative(res));
	ficlStackPushFTH(vm->dataStack, fth_make_bignum(res));
}

static void
ficl_babs(ficlVm *vm)
{
	ficlBignum res;

	FTH_STACK_CHECK(vm, 1, 1);
	res = BN_new();
	if (res == NULL) {
		BN_CHECKP(res);
		/* for ccc-analyzer */
		/* NOTREACHED */
		return;
	}
	fth_to_bn(res, fth_pop_ficl_cell(vm)); 
	if (BN_is_negative(res))
		BN_set_negative(res, 0);
	ficlStackPushFTH(vm->dataStack, fth_make_bignum(res));
}

static void
ficl_bmin(ficlVm *vm)
{
	FTH m, n;
	ficlBignum res;
	BIGNUM x, y;

	FTH_STACK_CHECK(vm, 2, 1);
	n = fth_pop_ficl_cell(vm);
	m = fth_pop_ficl_cell(vm);
	res = BN_new();
	BN_CHECKP(res);
	BN_init(&x);
	BN_init(&y);
	fth_to_bn(&x, m);
	fth_to_bn(&y, n);
	if (BN_cmp(&x, &y) < 0)
		BN_CHECKP(BN_copy(res, &x));
	else
		BN_CHECKP(BN_copy(res, &y));
	BN_free(&x);
	BN_free(&y);
	ficlStackPushFTH(vm->dataStack, fth_make_bignum(res));
}

static void
ficl_bmax(ficlVm *vm)
{
	FTH m, n;
	ficlBignum res;
	BIGNUM x, y;

	FTH_STACK_CHECK(vm, 2, 1);
	n = fth_pop_ficl_cell(vm);
	m = fth_pop_ficl_cell(vm);
	res = BN_new();
	BN_CHECKP(res);
	BN_init(&x);
	BN_init(&y);
	fth_to_bn(&x, m);
	fth_to_bn(&y, n);
	if (BN_cmp(&x, &y) >= 0)
		BN_CHECKP(BN_copy(res, &x));
	else
		BN_CHECKP(BN_copy(res, &y));
	BN_free(&x);
	BN_free(&y);
	ficlStackPushFTH(vm->dataStack, fth_make_bignum(res));
}

static void
ficl_btwostar(ficlVm *vm)
{
	ficlBignum res;

	FTH_STACK_CHECK(vm, 1, 1);
	res = BN_new();
	BN_CHECKP(res);
	fth_to_bn(res, fth_pop_ficl_cell(vm)); 
	BN_CHECK(BN_lshift1(res, res));
	ficlStackPushFTH(vm->dataStack, fth_make_bignum(res));
}

static void
ficl_btwoslash(ficlVm *vm)
{
	ficlBignum res;

	FTH_STACK_CHECK(vm, 1, 1);
	res = BN_new();
	BN_CHECKP(res);
	fth_to_bn(res, fth_pop_ficl_cell(vm)); 
	BN_CHECK(BN_rshift1(res, res));
	ficlStackPushFTH(vm->dataStack, fth_make_bignum(res));
}

static void
ficl_blshift(ficlVm *vm)
{
#define h_blshift "( b1 n -- b2 )  b2 = b1 * 2^n"
	FTH m;
	int n;
	ficlBignum res;

	FTH_STACK_CHECK(vm, 2, 1);
	n = (int)ficlStackPopInteger(vm->dataStack);
	m = fth_pop_ficl_cell(vm);
	res = BN_new();
	BN_CHECKP(res);
	fth_to_bn(res, m);
	BN_CHECK(BN_lshift(res, res, n));
	ficlStackPushFTH(vm->dataStack, fth_make_bignum(res));
}

static void
ficl_brshift(ficlVm *vm)
{
#define h_brshift "( b1 n -- b2 )  b2 = b1 / 2^n"
	FTH m;
	int n;
	ficlBignum res;

	FTH_STACK_CHECK(vm, 2, 1);
	n = (int)ficlStackPopInteger(vm->dataStack);
	m = fth_pop_ficl_cell(vm);
	res = BN_new();
	BN_CHECKP(res);
	fth_to_bn(res, m);
	BN_CHECK(BN_rshift(res, res, n));
	ficlStackPushFTH(vm->dataStack, fth_make_bignum(res));
}

/* 
 * Parse ficlBignum (in base 10) via bn(3).
 */
int
ficl_parse_bignum(ficlVm *vm, ficlString s)
{
	ficlBignum bn;
	ficlUnsigned i, j;
	int flag;
	bool allocated;
	char *str;

	if (s.length < 10)
		return (FICL_FALSE);

	flag = FICL_FALSE;
	bn = NULL;		/* NULL: dec2bn allocates */
	if (s.length >= FICL_PAD_SIZE) {
		str = FTH_MALLOC(s.length + 1);
		allocated = true;
	} else {
		str = vm->pad;
		allocated = false;
	}
	i = j = 0;
	if (s.text[0] == '-') {
		str[0] = s.text[0];
		i = j = 1;
	} else if (s.text[0] == '+')
		j = 1;
	for (; j < s.length; i++, j++) {
		if (isdigit((int)s.text[j]))
			str[i] = s.text[j];
		else
			goto finish;
	}
	str[i] = '\0';
	if (BN_dec2bn(&bn, str) != 0) {
		ficlStackPushFTH(vm->dataStack, fth_make_bignum(bn));
		if (vm->state == FICL_VM_STATE_COMPILE)
			ficlPrimitiveLiteralIm(vm);
		flag = FICL_TRUE;
	}
finish:
	if (allocated)
		FTH_FREE(str);
	return (flag);
}

#endif				/* HAVE_BN */

/* === RATIO via bn(3) === */

#define h_list_of_ratio_functions "\
*** RATIONAL PRIMITIVES ***\n\
ratio? (rational?)\n\
make-ratio     >ratio\n\
rationalize    exact->inexact      inexact->exact\n\
numerator      denominator\n\
q.   s>q  q>s  f>q  q>f\n\
q0=  q0<> q0<  q0>  q0<= q0>=\n\
q=   q<>  q<   q>   q<=  q>=\n\
q+   q-   q*   q/   1/q  q** (qpow)\n\
qnegate   qfloor    qceil    qabs"

static void
ficl_ratio_p(ficlVm *vm)
{
#define h_ratio_p "( obj -- f )  test if OBJ is a rational number\n\
nil    ratio? => #f\n\
1/2    ratio? => #t\n\
pi f>r ratio? => #t\n\
Return #t if OBJ is a ratio object, otherwise #f."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 1);
	obj = fth_pop_ficl_cell(vm);
	ficlStackPushBoolean(vm->dataStack, FTH_RATIO_P(obj));
}

#if HAVE_BN

#define FTH_RATIO_NUM(Obj)	FTH_RATIO_OBJECT(Obj)->num
#define FTH_RATIO_DEN(Obj)	FTH_RATIO_OBJECT(Obj)->den

static FTH
rt_inspect(FTH self)
{
	return (fth_make_string_format("%s: %S",
	    FTH_INSTANCE_NAME(self), rt_to_string(self)));
}

static FTH
rt_to_string(FTH self)
{
	FTH fs;
	char *num, *den;

	num = BN_bn2dec(FTH_RATIO_NUM(self));
	BN_CHECKP(num);
	den = BN_bn2dec(FTH_RATIO_DEN(self));
	BN_CHECKP(den);
	fs = fth_make_string_format("%s/%s", num, den);
	OPENSSL_free(num);
	OPENSSL_free(den);
	return (fs);
}

static FTH
rt_copy(FTH self)
{
	ficlRatio res;

	res = rt_init();
	BN_CHECKP(BN_copy(res->num, FTH_RATIO_NUM(self)));
	BN_CHECKP(BN_copy(res->den, FTH_RATIO_DEN(self)));
	return (fth_make_rational(res));
}

static FTH
rt_equal_p(FTH self, FTH obj)
{
	if (BN_cmp(FTH_RATIO_NUM(self), FTH_RATIO_NUM(obj)) == 0)
		return (BOOL_TO_FTH(BN_cmp(FTH_RATIO_DEN(self),
		    FTH_RATIO_DEN(obj)) == 0));
	return (false);
}

static void
rt_free(FTH self)
{
	rt_bn_free(FTH_RATIO_OBJECT(self));
}

static ficlRatio
rt_init(void)
{
	ficlRatio r;

	r = FTH_MALLOC(sizeof(FRatio));
	r->num = BN_new();
	BN_CHECKP(r->num);
	r->den = BN_new();
	BN_CHECKP(r->den);
	return (r);
}

static void
rt_bn_free(ficlRatio r)
{
	BN_free(r->num);
	BN_free(r->den);
	FTH_FREE(r);
}

static void
fth_to_rt(ficlRatio m, FTH x)
{
	int type = -1;

	if (FTH_NUMBER_T_P(x))
		type = FTH_INSTANCE_TYPE(x);
	switch (type) {
	case FTH_LLONG_T:
		int_to_bn(m->num, (ficlInteger)FTH_LONG_OBJECT(x));
		BN_CHECKP(BN_copy(m->den, &fth_bn_one));
		break;
	case FTH_FLOAT_T:
		{
			FTH n;

			n = fth_make_ratio_from_float(FTH_FLOAT_OBJECT(x));
			BN_CHECKP(BN_copy(m->num, FTH_RATIO_NUM(n)));
			BN_CHECKP(BN_copy(m->den, FTH_RATIO_DEN(n)));
		}
		break;
	case FTH_COMPLEX_T:
		{
			FTH n;

			n = fth_make_ratio_from_float(fth_float_ref(x));
			BN_CHECKP(BN_copy(m->num, FTH_RATIO_NUM(n)));
			BN_CHECKP(BN_copy(m->den, FTH_RATIO_DEN(n)));
		}
		break;
	case FTH_BIGNUM_T:
		BN_CHECKP(BN_copy(m->num, FTH_BIGNUM_OBJECT(x)));
		BN_CHECKP(BN_copy(m->den, &fth_bn_one));
		break;
	case FTH_RATIO_T:
		BN_CHECKP(BN_copy(m->num, FTH_RATIO_NUM(x)));
		BN_CHECKP(BN_copy(m->den, FTH_RATIO_DEN(x)));
		break;
	default:
		int_to_bn(m->num, fth_int_ref(x));
		BN_CHECKP(BN_copy(m->den, &fth_bn_one));
		break;
	}	
}

static ficlInteger
rt_to_int(ficlRatio r)
{
	ficl2Integer x;
	unsigned long y;

	x = (ficl2Integer)BN_get_word(r->num);
	y = BN_get_word(r->den);
	if (BN_is_negative(r->num))
		x = -x;
	return ((ficlInteger)(x / (ficl2Integer)y));
}

static ficlFloat
rt_to_float(ficlRatio r)
{
	ficlFloat x, y;

	x = (ficlFloat)BN_get_word(r->num);
	y = (ficlFloat)BN_get_word(r->den);
	if (BN_is_negative(r->num))
		x = -x;
	return (x / y);
}

static void
rt_canonicalize(ficlRatio r)
{
	if (BN_is_zero(r->den)) {
		FTH_MATH_ERROR_THROW("denominator 0");
		/* NOTREACHED */
		return;
	}
	if (BN_is_one(r->den))
		return;
	if (BN_cmp(r->num, r->den) == 0) {
		BN_CHECK(BN_one(r->num));
		BN_CHECK(BN_one(r->den));
	} else {
		BIGNUM gcd, tmp;
		BN_CTX *ctx;

		if (BN_is_negative(r->den)) {
			BN_set_negative(r->den, 0);
			BN_set_negative(r->num, !BN_is_negative(r->num));
		}
		ctx = bn_ctx_init();
		BN_init(&gcd);
		BN_init(&tmp);
		BN_CHECK(BN_gcd(&gcd, r->num, r->den, ctx));
		BN_swap(&tmp, r->num);
		BN_CHECK(BN_div(r->num, NULL, &tmp, &gcd, ctx));
		BN_swap(&tmp, r->den);
		BN_CHECK(BN_div(r->den, NULL, &tmp, &gcd, ctx));
		BN_free(&gcd);
		BN_free(&tmp);
		BN_CTX_free(ctx);
	}
}

static FTH
make_rational(ficlBignum num, ficlBignum den)
{
	FTH self;

	self = fth_make_instance(ratio_tag, NULL);
	FTH_RATIO_OBJECT_SET(self, FTH_MALLOC(sizeof(FRatio)));
	FTH_RATIO_NUM(self) = num;
	FTH_RATIO_DEN(self) = den;
	rt_canonicalize(FTH_RATIO_OBJECT(self));
	return (self);
}

FTH
fth_make_rational(ficlRatio r)
{
	FTH self;

	self = fth_make_instance(ratio_tag, NULL);
	FTH_RATIO_OBJECT_SET(self, r);
	return (self);
}

/*
 * Return a FTH ration object from NUM and DEN.
 */
FTH
fth_make_ratio(FTH num, FTH den)
{
#define h_make_ratio "( num den -- ratio )  return rational number\n\
123 456 make-ratio => 41/152\n\
355 113 make-ratio => 355/113\n\
Return a new ratio object with numerator NUM and denumerator DEN."

	if (den == FTH_ZERO) {
		FTH_MATH_ERROR_THROW("denominator 0");
		/* NOTREACHED */
		return (FTH_FALSE);
	} else {
		ficlBignum m, n;

		m = BN_new();
		BN_CHECKP(m);
		n = BN_new();
		BN_CHECKP(n);
		if (FTH_BIGNUM_P(num))
			BN_CHECKP(BN_copy(m, FTH_BIGNUM_OBJECT(num)));
		else
			fth_to_bn(m, num);
		if (FTH_BIGNUM_P(den))
			BN_CHECKP(BN_copy(n, FTH_BIGNUM_OBJECT(den)));
		else
			fth_to_bn(n, den);
		return (make_rational(m, n));
	}
}

FTH
fth_make_ratio_from_int(ficlInteger num, ficlInteger den)
{
	if (den == 0) {
		FTH_MATH_ERROR_THROW("denominator 0");
		/* NOTREACHED */
		return (FTH_FALSE);
	} else {
		ficlBignum m, n;

		m = BN_new();
		BN_CHECKP(m);
		n = BN_new();
		BN_CHECKP(n);
		int_to_bn(m, num);
		int_to_bn(n, den);
		return (make_rational(m, n));
	}
}

FTH
fth_make_ratio_from_float(ficlFloat fl)
{
#if defined(HAVE_FREXP) && defined(HAVE_LDEXP)
	ficlBignum m, n;
	int exp;

	m = BN_new();
	BN_CHECKP(m);
	n = BN_new();
	BN_CHECKP(n);
	fl = frexp(fl, &exp);
	fl = ldexp(fl, DBL_MANT_DIG);
	exp -= DBL_MANT_DIG;
	float_to_bn(m, fl);
	BN_CHECK(BN_one(n));
	if (exp < 0) {
		BIGNUM tmp;

		BN_init(&tmp);
		BN_CHECK(BN_one(&tmp));
		BN_CHECK(BN_lshift(n, &tmp, -exp));
		BN_free(&tmp);
	} else if (exp > 0) {
		BIGNUM tmp;

		BN_init(&tmp);
		BN_swap(&tmp, m);
		BN_CHECK(BN_lshift(m, &tmp, exp));
		BN_free(&tmp);
	}
	return (make_rational(m, n));
#else				/* !HAVE_FREXP */
	return (fth_make_ratio_from_int((ficlInteger)fl, 1L));
#endif				/* HAVE_FREXP */
}

static void
ficl_to_ratio(ficlVm *vm)
{
#define h_to_ratio "( numb -- ratio )  NUMB to ratio\n\
1 >ratio => 1/1\n\
1 >ratio ratio? => #t\n\
0.25 >ratio => 1/4\n\
Convert any number NUMB to a ratio object.\n\
See also s>r and f>r."
	ficlRatio res;

	FTH_STACK_CHECK(vm, 1, 1);
	res = rt_init();
	fth_to_rt(res, fth_pop_ficl_cell(vm));    
	ficlStackPushFTH(vm->dataStack, fth_make_rational(res));
}

static void
ficl_q_dot(ficlVm *vm)
{
#define h_q_dot "( numb -- )  number output\n\
1.5 r. => 3/2\n\
Print rational number NUMB."
	FTH obj;

	FTH_STACK_CHECK(vm, 1, 0);
	obj = fth_pop_ficl_cell(vm);
	if (FTH_RATIO_P(obj))
		fth_printf("%S ", obj);
	else if (FTH_BIGNUM_P(obj))
		fth_printf("%S/1 ", obj);
	else
		fth_printf("%S ",
		    fth_make_ratio_from_float(fth_float_ref(obj)));
}

static void
ficl_s_to_q(ficlVm *vm)
{
#define h_s_to_q "( n -- ratio )  convert N to rational number\n\
10 s>r => 10/1\n\
Convert integer N to a ratio object.\n\
See also f>r and >ratio."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushFTH(vm->dataStack,
	    fth_make_ratio_from_int(ficlStackPopInteger(vm->dataStack), 1L));
}

static void
ficl_f_to_q(ficlVm *vm)
{
#define h_f_to_q "( r -- ratio )  convert R to rational number\n\
1.5 f>r => 3/2\n\
Convert float R to a ratio object.\n\
See also s>r and >ratio."
	FTH_STACK_CHECK(vm, 1, 1);
	ficlStackPushFTH(vm->dataStack,
	    fth_make_ratio_from_float(ficlStackPopFloat(vm->dataStack)));
}

static void
ficl_qnegate(ficlVm *vm)
{
	ficlRatio res;

	FTH_STACK_CHECK(vm, 1, 1);
	res = rt_init();
	fth_to_rt(res, fth_pop_ficl_cell(vm));    
	BN_set_negative(res->num, !BN_is_negative(res->num));
	ficlStackPushFTH(vm->dataStack, fth_make_rational(res));
}

/*
 * XXX: Don't remove this function, required by fth.m4 to set
 * FTH_HAVE_RATIO=yes.
 */
FTH
fth_ratio_floor(FTH rt)
{
	ficlInteger x;

	x = (ficlInteger)FTH_FLOOR(rt_to_float(FTH_RATIO_OBJECT(rt)));
	return (fth_make_ratio_from_int(x, 1L));
}

static void
ficl_qfloor(ficlVm *vm)
{
#define h_qfloor "( x -- y )  y = floor(x) (ratio, result is int)"
	FTH m;
	ficlInteger x;

	FTH_STACK_CHECK(vm, 1, 1);
	m = fth_pop_ficl_cell(vm);
	x = (ficlInteger)FTH_FLOOR(rt_to_float(FTH_RATIO_OBJECT(m)));
	ficlStackPushInteger(vm->dataStack, x);
}

static void
ficl_qceil(ficlVm *vm)
{
	FTH m;
	ficlInteger x;

	FTH_STACK_CHECK(vm, 1, 1);
	m = fth_pop_ficl_cell(vm);
	x = (ficlInteger)FTH_CEIL(rt_to_float(FTH_RATIO_OBJECT(m)));
	ficlStackPushInteger(vm->dataStack, x);
}

static void
ficl_qabs(ficlVm *vm)
{
	ficlRatio res;

	FTH_STACK_CHECK(vm, 1, 1);
	res = rt_init();
	fth_to_rt(res, fth_pop_ficl_cell(vm));    
	if (BN_is_negative(res->num))
		BN_set_negative(res->num, 0);
	ficlStackPushFTH(vm->dataStack, fth_make_rational(res));
}

static void
ficl_qinvert(ficlVm *vm)
{
#define h_qinvert "( x -- y )  y = 1 / x (ratio)"
	ficlFloat fl;

	FTH_STACK_CHECK(vm, 1, 1);
	fl = ficlStackPopFloat(vm->dataStack);
	ficlStackPushFTH(vm->dataStack, fth_make_ratio_from_float(1.0 / fl));
}

/*
 * rt_cmp(m, n)
 *
 * m < n => -1
 * m = n =>  0
 * m > n =>  1
 */
static int
rt_cmp(ficlRatio m, ficlRatio n)
{
	int flag;

	if (BN_cmp(m->den, n->den) == 0)
		flag = BN_cmp(m->num, n->num);
	else {
		BIGNUM num1, num2, res;
		BN_CTX *ctx;

		ctx = bn_ctx_init();
		BN_init(&num1);
		BN_init(&num2);
		BN_init(&res);
		BN_CHECK(BN_mul(&num1, m->num, n->den, ctx));
		BN_CHECK(BN_mul(&num2, m->den, n->num, ctx));
		BN_CHECK(BN_sub(&res, &num1, &num2));
		flag = BN_cmp(&res, &fth_bn_zero);
		BN_CTX_free(ctx);
		BN_free(&num1);
		BN_free(&num2);
		BN_free(&res);
	}
	return (flag);
}

static ficlRatio
rt_addsub(FTH m, FTH n, int type)
{
	ficlRatio x, y, res;
	BIGNUM gcd, tmpx, tmpy, tmpz, tmp;
	BN_CTX *ctx;

	ctx = bn_ctx_init();
	BN_init(&gcd);
	BN_init(&tmpx);
	BN_init(&tmpy);
	BN_init(&tmpz);
	BN_init(&tmp);
	x = rt_init();
	y = rt_init();
	res = rt_init();
	fth_to_rt(x, m);
	fth_to_rt(y, n);
	BN_CHECK(BN_gcd(&gcd, x->den, y->den, ctx));
	BN_CHECK(BN_div(&tmp, NULL, y->den, &gcd, ctx));
	BN_CHECK(BN_mul(&tmpx, x->num, &tmp, ctx));
	BN_CHECK(BN_div(&tmp, NULL, x->den, &gcd, ctx));
	BN_CHECK(BN_mul(&tmpy, y->num, &tmp, ctx));
	if (type == BN_ADD)
		BN_CHECK(BN_add(&tmpz, &tmpx, &tmpy));
	else
		BN_CHECK(BN_sub(&tmpz, &tmpx, &tmpy));
	BN_CHECK(BN_div(&tmpy, NULL, x->den, &gcd, ctx));
	BN_swap(&tmp, &gcd);
	BN_CHECK(BN_gcd(&gcd, &tmpz, &tmp, ctx));
	BN_CHECK(BN_div(res->num, NULL, &tmpz, &gcd, ctx));
	BN_CHECK(BN_div(&tmpx, NULL, y->den, &gcd, ctx));
	BN_CHECK(BN_mul(res->den, &tmpx, &tmpy, ctx));
	BN_CTX_free(ctx);
	BN_free(&gcd);
	BN_free(&tmpx);
	BN_free(&tmpy);
	BN_free(&tmpz);
	BN_free(&tmp);
	rt_bn_free(x);
	rt_bn_free(y);
	return (res);
}

static ficlRatio
rt_muldiv(FTH m, FTH n, int type)
{
	ficlRatio x, y, res;
	BIGNUM gcd1, gcd2, tmp1, tmp2;
	BN_CTX *ctx;

	ctx = bn_ctx_init();
	BN_init(&gcd1);
	BN_init(&gcd2);
	BN_init(&tmp1);
	BN_init(&tmp2);
	x = rt_init();
	y = rt_init();
	res = rt_init();
	fth_to_rt(x, m);
	fth_to_rt(y, n);
	if (type == BN_DIV) {
		if (BN_is_negative(y->num)) {
			BN_set_negative(x->num, !BN_is_negative(x->num));
			BN_set_negative(y->num, !BN_is_negative(y->num));
		}
		BN_swap(y->num, y->den);
	}
	BN_CHECK(BN_gcd(&gcd1, x->num, y->den, ctx));
	BN_CHECK(BN_gcd(&gcd2, x->den, y->num, ctx));
	BN_CHECK(BN_div(&tmp1, NULL, x->num, &gcd1, ctx));
	BN_CHECK(BN_div(&tmp2, NULL, y->num, &gcd2, ctx));
	BN_CHECK(BN_mul(res->num, &tmp1, &tmp2, ctx));
	BN_CHECK(BN_div(&tmp1, NULL, x->den, &gcd2, ctx));
	BN_CHECK(BN_div(&tmp2, NULL, y->den, &gcd1, ctx));
	BN_CHECK(BN_mul(res->den, &tmp1, &tmp2, ctx));
	BN_CTX_free(ctx);
	BN_free(&gcd1);
	BN_free(&gcd2);
	BN_free(&tmp1);
	BN_free(&tmp2);
	rt_bn_free(x);
	rt_bn_free(y);
	return (res);
}

static FTH
rt_add(FTH m, FTH n)
{
	return (fth_make_rational(rt_addsub(m, n, BN_ADD)));
}

static FTH
rt_sub(FTH m, FTH n)
{
	return (fth_make_rational(rt_addsub(m, n, BN_SUB)));
}

static FTH
rt_mul(FTH m, FTH n)
{
	return (fth_make_rational(rt_muldiv(m, n, BN_MUL)));
}

static FTH
rt_div(FTH m, FTH n)
{
	return (fth_make_rational(rt_muldiv(m, n, BN_DIV)));
}

#define N_RATIO_FUNC_TEST_ZERO(Name, OP)				\
static bool								\
fth_rt_ ## Name(FTH m)							\
{									\
	bool flag; 							\
									\
	if (FTH_RATIO_P(m))						\
		flag = (rt_cmp(FTH_RATIO_OBJECT(m), &fth_rt_zero) OP 0); \
	else if (FTH_BIGNUM_P(m))					\
		flag = (BN_cmp(FTH_BIGNUM_OBJECT(m), &fth_bn_zero) OP 0); \
	else {								\
		ficlRatio x;						\
									\
		x = rt_init();						\
		fth_to_rt(x, m);					\
		flag = (rt_cmp(x, &fth_rt_zero) OP 0);			\
		rt_bn_free(x);						\
	}								\
	return (flag);							\
}									\
static void								\
ficl_ ## Name(ficlVm *vm)						\
{									\
	FTH_STACK_CHECK(vm, 1, 1);					\
	ficlStackPushBoolean(vm->dataStack,				\
	    fth_rt_ ## Name(fth_pop_ficl_cell(vm)));			\
}									\
static char* h_ ## Name = "( x -- f )  x " #OP " 0 => flag (ratio)"

/*
 * build: 
 *   bool fth_rt_qeqz(FTH m) ...    for C fth_number_equal_p etc
 *   void ficl_qeqz(ficlVm *vm) ... for Forth words
 */
N_RATIO_FUNC_TEST_ZERO(qeqz, ==);
N_RATIO_FUNC_TEST_ZERO(qnoteqz, !=);
N_RATIO_FUNC_TEST_ZERO(qlessz, <);
N_RATIO_FUNC_TEST_ZERO(qgreaterz, >);
N_RATIO_FUNC_TEST_ZERO(qlesseqz, <=);
N_RATIO_FUNC_TEST_ZERO(qgreatereqz, >=);

#define N_RATIO_FUNC_TEST_TWO_OP(Name, OP)				\
static bool								\
fth_rt_ ## Name(FTH m, FTH n)						\
{									\
	bool flag;							\
									\
	if (FTH_RATIO_P(m)) {						\
		if (FTH_RATIO_P(n))					\
			flag = (rt_cmp(FTH_RATIO_OBJECT(m),		\
			    FTH_RATIO_OBJECT(n)) OP 0);			\
		else {							\
			ficlRatio y;					\
									\
			y = rt_init();					\
			fth_to_rt(y, n);				\
			flag = (rt_cmp(FTH_RATIO_OBJECT(m), y) OP 0);	\
			rt_bn_free(y);					\
		}							\
	} else if (FTH_RATIO_P(n)) {					\
		ficlRatio x;						\
									\
		x = rt_init();						\
		fth_to_rt(x, m);					\
		flag = (rt_cmp(x, FTH_RATIO_OBJECT(n)) OP 0);		\
		rt_bn_free(x);						\
	} else {							\
		ficlRatio x, y;						\
									\
		x = rt_init();						\
		y = rt_init();						\
		fth_to_rt(x, m);					\
		fth_to_rt(y, n);					\
		flag = (rt_cmp(x, y) OP 0);				\
		rt_bn_free(x);						\
		rt_bn_free(y);						\
	}								\
	return (flag);							\
}									\
static void								\
ficl_ ## Name(ficlVm *vm)						\
{									\
	FTH m, n;							\
									\
	FTH_STACK_CHECK(vm, 2, 1);					\
	n = fth_pop_ficl_cell(vm);					\
	m = fth_pop_ficl_cell(vm);					\
	ficlStackPushBoolean(vm->dataStack, fth_rt_ ## Name(m, n));	\
}									\
static char* h_ ## Name = "( x y -- f )  x " #OP " y => flag (ratio)"

/*
 * build: 
 *   bool fth_rt_qeq(FTH m, FTH n) ... for C fth_number_equal_p etc
 *   void ficl_qeq(ficlVm *vm) ...     for Forth words
 */
N_RATIO_FUNC_TEST_TWO_OP(qeq, ==);
N_RATIO_FUNC_TEST_TWO_OP(qnoteq, !=);
N_RATIO_FUNC_TEST_TWO_OP(qless, <);
N_RATIO_FUNC_TEST_TWO_OP(qgreater, >);
N_RATIO_FUNC_TEST_TWO_OP(qlesseq, <=);
N_RATIO_FUNC_TEST_TWO_OP(qgreatereq, >=);

N_BIGNUM_MATH_FUNC_OP(qadd, +, rt_add);
N_BIGNUM_MATH_FUNC_OP(qsub, -, rt_sub);
N_BIGNUM_MATH_FUNC_OP(qmul, *, rt_mul);
N_BIGNUM_MATH_FUNC_OP(qdiv, /, rt_div);

/* 
 * Parse ficlRatio (in base 10) via bn(3) (1/2, -3/2 etc).
 */
int
ficl_parse_ratio(ficlVm *vm, ficlString s)
{
	ficlBignum bn, bd;
	char den_buf[FICL_PAD_SIZE], *snum, *sden;
	ficlUnsigned i, j;
	int flag;
	bool allocated;

	if (s.length < 3)
		return (FICL_FALSE);

	flag = FICL_FALSE;
	bn = bd = NULL;		/* NULL: dec2bn allocates */
	if (s.length >= FICL_PAD_SIZE) {
		snum = FTH_MALLOC(s.length + 1);
		sden = FTH_MALLOC(s.length + 1);
		allocated = true;
	} else {
		snum = vm->pad;
		sden = den_buf;
		allocated = false;
	}
	/* check for nominator up to '/' */
	i = j = 0;
	if (s.text[j] == '-') {
		snum[i] = s.text[j];
		i = j = 1;
	} else if (s.text[j] == '+')
		j = 1;
	for (; j < s.length; i++, j++) {
		if (isdigit((int)s.text[j]))
			snum[i] = s.text[j];
		else if (s.text[j] == '/') {
			j++;
			break;
		}
		else
			goto finish;
	}
	snum[i] = '\0';
	/* check for denominator after '/' */
	i = 0;
	for (; j < s.length; i++, j++) {
		if (isdigit((int)s.text[j]))
			sden[i] = s.text[j];
		else
			goto finish;
	}
	sden[i] = '\0';
	if (BN_dec2bn(&bn, snum) != 0 && BN_dec2bn(&bd, sden) != 0) {
		ficlStackPushFTH(vm->dataStack, make_rational(bn, bd));
		if (vm->state == FICL_VM_STATE_COMPILE)
			ficlPrimitiveLiteralIm(vm);
		flag = FICL_TRUE;
	}
finish:
	if (allocated) {
		FTH_FREE(snum);
		FTH_FREE(sden);
	}
	return (flag);
}

static FTH
number_floor(FTH x)
{
	int type;

	if (x == 0)
		return (x);
	type = -1;
	if (FTH_NUMBER_T_P(x))
		type = FTH_INSTANCE_TYPE(x);
	switch (type) {
	case FTH_FLOAT_T:
		return (fth_make_float(FTH_FLOOR(FTH_FLOAT_OBJECT(x))));
		break;
	case FTH_RATIO_T:
		return (fth_make_ratio_from_float(FTH_FLOOR(
		    rt_to_float(FTH_RATIO_OBJECT(x)))));
		break;
	default:
		return (x);
		break;
	}
}

static FTH
number_inv(FTH x)
{
	int type;

	if (x == 0)
		return (x);
	type = -1;
	if (FTH_NUMBER_T_P(x))
		type = FTH_INSTANCE_TYPE(x);
	switch (type) {
	case FTH_RATIO_T:
		return (fth_make_ratio_from_float(1.0 /
		    rt_to_float(FTH_RATIO_OBJECT(x))));
		break;
	case FTH_FLOAT_T:
		return (fth_make_float(1.0 / FTH_FLOAT_OBJECT(x)));
		break;
	default:
		return (x);
		break;
	}
}

/*
 * Return inexact number within ERR of X.
 */
FTH
fth_rationalize(FTH x, FTH err)
{
	if (FTH_INTEGER_P(x))
		return (x);
	if (FTH_RATIO_P(x) || FTH_INEXACT_P(x)) {
		ficlInteger a, a1, a2, b, b1, b2;
		ficlFloat fex, er;
		FTH ex, dx, rx, tt;
		int i;

		if (FTH_RATIO_P(x))
			ex = x;
		else
			ex = fth_make_ratio_from_float(fth_float_ref(x));
		dx = number_floor(ex);
		if (fth_number_equal_p(dx, ex))
			return (ex);
		a1 = 0;
		a2 = 1;
		b1 = 1;
		b2 = 0;
		er = fabs(fth_float_ref(err));
		tt = FTH_ONE;
		i = 1000000;
		ex = fth_number_sub(ex, dx);
		if (ex == 0)
			return (FTH_ZERO);
		rx = number_inv(ex);
		fex = FTH_RATIO_REF_FLOAT(ex);
		while (--i) {
			a = a1 * fth_int_ref(tt) + a2;
			b = b1 * fth_int_ref(tt) + b2;

			if (b != 0 &&
			    fabs(fex - (ficlFloat)a / (ficlFloat)b) <= er)
				return (fth_number_add(dx,
				    fth_make_ratio_from_int(a, b)));

			rx = number_inv(fth_number_sub(rx, tt));
			tt = number_floor(rx);
			a2 = a1;
			b2 = b1;
			a1 = a;
			b1 = b;
		}
	}
	return (FTH_ZERO);
}

static void
ficl_rationalize(ficlVm *vm)
{
#define h_rationalize "( x err -- val )  return number within ERR of X\n\
5.2  0.1  rationalize => 5.25\n\
5.4  0.1  rationalize => 5.5\n\
5.23 0.02 rationalize => 5.25\n\
Return inexact number within ERR of X."
	FTH x, err;

	FTH_STACK_CHECK(vm, 2, 1);
	err = fth_pop_ficl_cell(vm);
	x = fth_pop_ficl_cell(vm);
	if (FTH_EXACT_P(x) && FTH_EXACT_P(err))
		fth_push_ficl_cell(vm, fth_rationalize(x, err));
	else
		ficlStackPushFTH(vm->dataStack,
		    fth_exact_to_inexact(fth_rationalize(x, err)));
}

#endif				/* HAVE_BN */

#if HAVE_COMPLEX
#define N_CMP_COMPLEX_OP(Numb1, Numb2, Flag, OP) do {			\
	ficlComplex x, y;						\
									\
	x = fth_complex_ref(Numb1);					\
	y = fth_complex_ref(Numb2);					\
	if (creal(x) == creal(y))					\
		Flag = (cimag(x) OP cimag(y));				\
	else								\
		Flag = (creal(x) OP creal(y));				\
} while (0)
#else				/* !HAVE_COMPLEX */
#define N_CMP_COMPLEX_OP(Numb1, Numb2, Flag, OP)
#endif				/* HAVE_COMPLEX */

#if HAVE_BN
#define N_CMP_BIGNUM_OP(Numb1, Numb2, Flag, OP, Name) do {		\
	Flag = fth_bn_b ## Name(Numb1, Numb2);				\
} while (0)

#define N_CMP_RATIO_OP(Numb1, Numb2, Flag, OP, Name) do {		\
	Flag = fth_rt_q ## Name(Numb1, Numb2);				\
} while (0)
#else				/* !HAVE_BN */
#define N_CMP_BIGNUM_OP(Numb1, Numb2, Flag, OP, Name)
#define N_CMP_RATIO_OP(Numb1, Numb2, Flag, OP, Name)
#endif				/* HAVE_BN */

#define N_CMP_TWO_OP(Numb1, Numb2, Flag, OP, Name) do {			\
	int type = -1;							\
									\
	if (FTH_NUMBER_T_P(Numb1))					\
		type = FTH_INSTANCE_TYPE(Numb1);			\
	if (FTH_NUMBER_T_P(Numb2))					\
		type = FICL_MAX(type, (int)FTH_INSTANCE_TYPE(Numb2));	\
	switch (type) {							\
	case FTH_LLONG_T:						\
		Flag = (fth_long_long_ref(Numb1) OP 			\
		    fth_long_long_ref(Numb2));				\
		break;							\
	case FTH_FLOAT_T:						\
		Flag = (fth_float_ref(Numb1) OP fth_float_ref(Numb2));	\
		break;							\
	case FTH_COMPLEX_T:						\
		N_CMP_COMPLEX_OP(Numb1, Numb2, Flag, OP);		\
		break;							\
	case FTH_BIGNUM_T:						\
		N_CMP_BIGNUM_OP(Numb1, Numb2, Flag, OP, Name);		\
		break;							\
	case FTH_RATIO_T:						\
		N_CMP_RATIO_OP(Numb1, Numb2, Flag, OP, Name);		\
		break;							\
	default:							\
		Flag = (Numb1 OP Numb2);				\
		break;							\
	}								\
} while (0)

bool
fth_number_equal_p(FTH m, FTH n)
{
	if (NUMB_FIXNUM_P(m) && NUMB_FIXNUM_P(n))
		return (FIX_TO_INT(m) == FIX_TO_INT(n));
	else {
		bool flag = false;

		N_CMP_TWO_OP(m, n, flag, ==, eq);
		return (flag);
	}
}

bool
fth_number_less_p(FTH m, FTH n)
{
	if (NUMB_FIXNUM_P(m) && NUMB_FIXNUM_P(n))
		return (FIX_TO_INT(m) < FIX_TO_INT(n));
	else {
		bool flag = false;

		N_CMP_TWO_OP(m, n, flag, <, less);
		return (flag);
	}
}

#if HAVE_COMPLEX
#define N_MATH_COMPLEX_OP(N1, N2, OP)					\
	N1 = fth_make_complex(fth_complex_ref(N1) OP fth_complex_ref(N2))
#else				/* !HAVE_COMPLEX */
#define N_MATH_COMPLEX_OP(N1, N2, OP)
#endif				/* HAVE_COMPLEX */

#if HAVE_BN
#define N_MATH_BIGNUM_OP(Numb1, Numb2, GOP) do {			\
	Numb1 = bn_ ## GOP(Numb1, Numb2);				\
} while (0)

#define N_MATH_RATIO_OP(Numb1, Numb2, GOP) do {				\
	Numb1 = rt_ ## GOP(Numb1, Numb2);				\
} while (0)
#else				/* !HAVE_BN */
#define N_MATH_BIGNUM_OP(Numb1, Numb2, GOP)
#define N_MATH_RATIO_OP(Numb1, Numb2, GOP)
#endif				/* HAVE_BN */

#define N_MATH_OP(Numb1, Numb2, OP, GOP) do {				\
	int type = -1;							\
									\
	if (FTH_NUMBER_T_P(Numb1))					\
		type = FTH_INSTANCE_TYPE(Numb1);			\
	if (FTH_NUMBER_T_P(Numb2))					\
		type = FICL_MAX(type, (int)FTH_INSTANCE_TYPE(Numb2));	\
	switch (type) {							\
	case FTH_LLONG_T:						\
		Numb1 = fth_make_long_long(fth_long_long_ref(Numb1) OP	\
		    fth_long_long_ref(Numb2));				\
		break;							\
	case FTH_FLOAT_T:						\
		Numb1 = fth_make_float(fth_float_ref(Numb1) OP 		\
		    fth_float_ref(Numb2));				\
		break;							\
	case FTH_COMPLEX_T:						\
		N_MATH_COMPLEX_OP(Numb1, Numb2, OP);			\
		break;							\
	case FTH_BIGNUM_T:						\
		N_MATH_BIGNUM_OP(Numb1, Numb2, GOP);			\
		break;							\
	case FTH_RATIO_T:						\
		N_MATH_RATIO_OP(Numb1, Numb2, GOP);			\
		break;							\
	default:							\
		Numb1 = Numb1 OP Numb2;					\
		break;							\
	}								\
} while (0)

FTH
fth_number_add(FTH m, FTH n)
{
	if (NUMB_FIXNUM_P(m) && NUMB_FIXNUM_P(n))
		return (fth_make_int(FIX_TO_INT(m) + FIX_TO_INT(n)));
	N_MATH_OP(m, n, +, add);
	return (m);
}

FTH
fth_number_sub(FTH m, FTH n)
{
	if (NUMB_FIXNUM_P(m) && NUMB_FIXNUM_P(n))
		return (fth_make_int(FIX_TO_INT(m) - FIX_TO_INT(n)));
	/* suggested from scan-build */
	if (m == 0 || n == 0)
		return (m);
	N_MATH_OP(m, n, -, sub);
	return (m);
}

FTH
fth_number_mul(FTH m, FTH n)
{
	if (NUMB_FIXNUM_P(m) && NUMB_FIXNUM_P(n))
		return (fth_make_int(FIX_TO_INT(m) * FIX_TO_INT(n)));
	N_MATH_OP(m, n, *, mul);
	return (m);
}

FTH
fth_number_div(FTH m, FTH n)
{
	if (NUMB_FIXNUM_P(m) && NUMB_FIXNUM_P(n))
		return (fth_make_int(FIX_TO_INT(m) / FIX_TO_INT(n)));
	N_MATH_OP(m, n, /, div);
	return (m);
}

FTH
fth_exact_to_inexact(FTH obj)
{
#define h_exact_to_inexact "( numb1 -- numb2 )  convert to inexact number\n\
3/2 exact->inexact => 1.5\n\
Convert NUMB to an inexact number.\n\
See also inexact->exact."
	FTH_ASSERT_ARGS(FTH_NUMBER_P(obj), obj, FTH_ARG1, "a number");
	if (FTH_EXACT_P(obj))
		return (fth_make_float(fth_float_ref(obj)));
	return (obj);
}

FTH
fth_inexact_to_exact(FTH obj)
{
#define h_inexact_to_exact "( numb1 -- numb2 )  convert to exact number\n\
1.5 inexact->exact => 3/2\n\
Convert NUMB to an exact number.\n\
See also exact->inexact."
	FTH_ASSERT_ARGS(FTH_NUMBER_P(obj), obj, FTH_ARG1, "a number");
	if (FTH_INEXACT_P(obj))
#if HAVE_BN
		return (fth_make_ratio_from_float(fth_float_ref(obj)));
#else
		return (fth_make_int((ficlInteger)fth_int_ref(obj)));
#endif
	return (obj);
}

/*
 * Return numerator from OBJ or 0.
 */
FTH
fth_numerator(FTH obj)
{
#define h_numerator "( obj -- numerator )  return numerator\n\
3/4 numerator => 3\n\
5 numerator => 5\n\
1.5 numerator => 0\n\
Return numerator of OBJ or 0.\n\
See also denominator."
	if (FTH_INTEGER_P(obj))
		return (obj);
#if HAVE_BN
	if (FTH_RATIO_P(obj)) {
		unsigned long x;

		x = BN_get_word(FTH_RATIO_NUM(obj));
		if (x < ULONG_MAX) {
			if (BN_is_negative(FTH_RATIO_NUM(obj)))
				return (fth_make_int(-x));
			else
				return (fth_make_int(x));
			
		} else {
			ficlBignum res;

			res = BN_new();
			BN_CHECKP(res);
			BN_CHECKP(BN_copy(res, FTH_RATIO_NUM(obj)));
			return (fth_make_bignum(res));
		}
	}
#endif
	return (FTH_ZERO);
}

/*
 * Return denominator from OBJ or 1.
 */
FTH
fth_denominator(FTH obj)
{
#define h_denominator "( obj -- denominator )  return denominator\n\
3/4 denominator => 4\n\
5 denominator => 1\n\
1.5 denominator => 1\n\
Return denominator of OBJ or 1.\n\
See also numerator."
#if HAVE_BN
	if (FTH_RATIO_P(obj)) {
		unsigned long x;

		x = BN_get_word(FTH_RATIO_DEN(obj));
		if (x < ULONG_MAX)
			return (fth_make_int(x));
		else {
			ficlBignum res;

			res = BN_new();
			BN_CHECKP(res);
			BN_CHECKP(BN_copy(res, FTH_RATIO_DEN(obj)));
			return (fth_make_bignum(res));
		}
	}
#endif
	return (FTH_ONE);
}

static void
ficl_odd_p(ficlVm *vm)
{
#define h_odd_p "( numb -- f )  test if NUMB is odd\n\
3 odd? => #t\n\
6 odd? => #f\n\
Return #t if NUMB is odd, otherwise #f.\n\
See also even?"
	FTH m;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	m = fth_pop_ficl_cell(vm);
#if HAVE_BN
	if (FTH_BIGNUM_P(m))
		flag = BN_is_odd(FTH_BIGNUM_OBJECT(m));
	else
		flag = ((fth_int_ref(m) % 2) != 0);
#else
	flag = ((fth_int_ref(m) % 2) != 0);
#endif
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_even_p(ficlVm *vm)
{
#define h_even_p "( numb -- f )  test if NUMB is even\n\
3 even? => #f\n\
6 even? => #t\n\
Return #t if NUMB is even, otherwise #f.\n\
See also odd?"
	FTH m;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	m = fth_pop_ficl_cell(vm);
#if HAVE_BN
	if (FTH_BIGNUM_P(m))
		flag = !BN_is_odd(FTH_BIGNUM_OBJECT(m));
	else
		flag = ((fth_int_ref(m) % 2) == 0);
#else
	flag = ((fth_int_ref(m) % 2) == 0);
#endif
	ficlStackPushBoolean(vm->dataStack, flag);
}

static void
ficl_prime_p(ficlVm *vm)
{
#define h_prime_p "( numb -- f )  test if NUMB is a prime number\n\
3   prime? => #t\n\
123 prime? => #f\n\
Return #t if NUMB is a prime number, otherwise #f."
	FTH m;
	bool flag;

	FTH_STACK_CHECK(vm, 1, 1);
	m = fth_pop_ficl_cell(vm);
#if HAVE_BN
	{
		BN_CTX *ctx;

		ctx = bn_ctx_init();
		if (FTH_BIGNUM_P(m))
			flag = BN_is_prime(FTH_BIGNUM_OBJECT(m),
			    BN_prime_checks, NULL, ctx, NULL);
		else {
			BIGNUM x;

			BN_init(&x);
			int_to_bn(&x, fth_int_ref(m));
			flag = BN_is_prime(&x,
			    BN_prime_checks, NULL, ctx, NULL);
			BN_free(&x);
		}
		BN_CTX_free(ctx);
	}
#else				/* !HAVE_BN */
	{
		ficl2Integer x;

		x = fth_long_long_ref(m);
		if (x == 2)
			flag = true;
		else if ((x % 2) != 0) {
			int i;

			for (i = 3; i < (int)sqrt((double)x); i += 2)
				if (((x % i) == 0)) {
					flag = false;
					goto finish;
				}
			flag = true;
		} else
			flag = false;
	}
finish:
#endif				/* HAVE_BN */
	ficlStackPushBoolean(vm->dataStack, flag);
}

void
init_number_types(void)
{
	/* init llong */
	llong_tag = make_object_number_type(FTH_STR_LLONG,
	    FTH_LLONG_T, N_EXACT_T);
	fth_set_object_inspect(llong_tag, ll_inspect);
	fth_set_object_to_string(llong_tag, ll_to_string);
	fth_set_object_copy(llong_tag, ll_copy);
	fth_set_object_equal_p(llong_tag, ll_equal_p);
	/* init float */
	float_tag = make_object_number_type(FTH_STR_FLOAT,
	    FTH_FLOAT_T, N_INEXACT_T);
	fth_set_object_inspect(float_tag, fl_inspect);
	fth_set_object_to_string(float_tag, fl_to_string);
	fth_set_object_copy(float_tag, fl_copy);
	fth_set_object_equal_p(float_tag, fl_equal_p);
#if HAVE_COMPLEX
	/* complex */
	complex_tag = make_object_number_type(FTH_STR_COMPLEX,
	    FTH_COMPLEX_T, N_INEXACT_T);
	fth_set_object_inspect(complex_tag, cp_inspect);
	fth_set_object_to_string(complex_tag, cp_to_string);
	fth_set_object_copy(complex_tag, cp_copy);
	fth_set_object_equal_p(complex_tag, cp_equal_p);
#endif				/* HAVE_COMPLEX */
#if HAVE_BN
	/* init bignum */
	bignum_tag = make_object_number_type(FTH_STR_BIGNUM,
	    FTH_BIGNUM_T, N_EXACT_T);
	fth_set_object_inspect(bignum_tag, bn_inspect);
	fth_set_object_to_string(bignum_tag, bn_to_string);
	fth_set_object_copy(bignum_tag, bn_copy);
	fth_set_object_equal_p(bignum_tag, bn_equal_p);
	fth_set_object_free(bignum_tag, bn_free);
	BN_init(&fth_bn_zero);
	BN_init(&fth_bn_one);
	BN_CHECK(BN_zero(&fth_bn_zero));
	BN_CHECK(BN_one(&fth_bn_one));
	/* init ratio */
	ratio_tag = make_object_number_type(FTH_STR_RATIO,
	    FTH_RATIO_T, N_EXACT_T);
	fth_set_object_inspect(ratio_tag, rt_inspect);
	fth_set_object_to_string(ratio_tag, rt_to_string);
	fth_set_object_copy(ratio_tag, rt_copy);
	fth_set_object_equal_p(ratio_tag, rt_equal_p);
	fth_set_object_free(ratio_tag, rt_free);
	fth_rt_zero.num = &fth_bn_zero;
	fth_rt_zero.den = &fth_bn_one;
#endif				/* HAVE_BN */
}

#if HAVE_BN
void
free_number_types(void)
{
	BN_free(&fth_bn_zero);
	BN_free(&fth_bn_one);
}
#endif				/* HAVE_BN */

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#if defined(HAVE_TIME_H)
#include <time.h>
#endif

void
init_number(void)
{
	ficlDictionary *env;

#if !defined(INFINITY)
	{
		double tmp, inf;

		inf = tmp = 1e+10;
		while (true) {
			inf *= 1e+10;
			if (inf == tmp)
				break;
			tmp = inf;
		}
		fth_infinity = inf;
	}
#endif
	/* int, llong, rand */
	fth_srand((ficlUnsigned)time(NULL));
	FTH_PRI1("number?", ficl_number_p, h_number_p);
	FTH_PRI1("fixnum?", ficl_fixnum_p, h_fixnum_p);
	FTH_PRI1("unsigned?", ficl_unsigned_p, h_unsigned_p);
	FTH_PRI1("long-long?", ficl_llong_p, h_llong_p);
	FTH_PRI1("off-t?", ficl_llong_p, h_llong_p);
	FTH_PRI1("ulong-long?", ficl_ullong_p, h_ullong_p);
	FTH_PRI1("uoff-t?", ficl_ullong_p, h_ullong_p);
	FTH_PRI1("integer?", ficl_integer_p, h_integer_p);
	FTH_PRI1("exact?", ficl_exact_p, h_exact_p);
	FTH_PRI1("inexact?", ficl_inexact_p, h_inexact_p);
	FTH_PRI1("make-long-long", ficl_make_long_long, h_make_long_long);
	FTH_PRI1(">llong", ficl_make_long_long, h_make_long_long);
	FTH_PRI1("make-off-t", ficl_make_long_long, h_make_long_long);
	FTH_PRI1("make-ulong-long", ficl_make_ulong_long, h_make_ulong_long);
	FTH_PRI1("make-off-t", ficl_make_ulong_long, h_make_ulong_long);
	FTH_PRI1("rand-seed-ref", ficl_rand_seed_ref, h_rand_seed_ref);
	FTH_PRI1("rand-seed-set!", ficl_rand_seed_set, h_rand_seed_set);
	FTH_PRI1("random", ficl_random, h_random);
	FTH_PRI1("frandom", ficl_frandom, h_frandom);
	FTH_PRI1(".r", ficl_dot_r, h_dot_r);
	FTH_PRI1("u.r", ficl_u_dot_r, h_u_dot_r);
	FTH_PRI1("d.", ficl_d_dot, h_d_dot);
	FTH_PRI1("ud.", ficl_ud_dot, h_ud_dot);
	FTH_PRI1("d.r", ficl_d_dot_r, h_d_dot_r);
	FTH_PRI1("ud.r", ficl_ud_dot_r, h_ud_dot_r);
	FTH_PRI1("u=", ficl_ueq, h_ueq);
	FTH_PRI1("u<>", ficl_unoteq, h_unoteq);
	FTH_PRI1("u<", ficl_uless, h_uless);
	FTH_PRI1("u<=", ficl_ulesseq, h_ulesseq);
	FTH_PRI1("u>", ficl_ugreater, h_ugreater);
	FTH_PRI1("u>=", ficl_ugreatereq, h_ugreatereq);
	FTH_PRI1("s>d", ficl_to_d, h_to_d);
	FTH_PRI1("d>s", ficl_to_s, h_to_s);
	FTH_PRI1("f>d", ficl_to_d, h_to_d);
	FTH_PRI1("d>f", ficl_to_f, h_to_f);
	FTH_PRI1("dzero?", ficl_dzero, h_dzero);
	FTH_PRI1("d0=", ficl_dzero, h_dzero);
	FTH_PRI1("d0<>", ficl_dnotz, h_dnotz);
	FTH_PRI1("d0<", ficl_dlessz, h_dlessz);
	FTH_PRI1("dnegative?", ficl_dlessz, h_dlessz);
	FTH_PRI1("d0<=", ficl_dlesseqz, h_dlesseqz);
	FTH_PRI1("d0>", ficl_dgreaterz, h_dgreaterz);
	FTH_PRI1("d0>=", ficl_dgreatereqz, h_dgreatereqz);
	FTH_PRI1("dpositive?", ficl_dgreatereqz, h_dgreatereqz);
	FTH_PRI1("d=", ficl_deq, h_deq);
	FTH_PRI1("d<>", ficl_dnoteq, h_dnoteq);
	FTH_PRI1("d<", ficl_dless, h_dless);
	FTH_PRI1("d<=", ficl_dlesseq, h_dlesseq);
	FTH_PRI1("d>", ficl_dgreater, h_dgreater);
	FTH_PRI1("d>=", ficl_dgreatereq, h_dgreatereq);
	FTH_PRI1("du=", ficl_dueq, h_dueq);
	FTH_PRI1("du<>", ficl_dunoteq, h_dunoteq);
	FTH_PRI1("du<", ficl_duless, h_duless);
	FTH_PRI1("du<=", ficl_dulesseq, h_dulesseq);
	FTH_PRI1("du>", ficl_dugreater, h_dugreater);
	FTH_PRI1("du>=", ficl_dugreatereq, h_dugreatereq);
	FTH_PRI1("d+", ficl_dadd, h_dadd);
	FTH_PRI1("d-", ficl_dsub, h_dsub);
	FTH_PRI1("d*", ficl_dmul, h_dmul);
	FTH_PRI1("d/", ficl_ddiv, h_ddiv);
	FTH_PRI1("dnegate", ficl_dnegate, h_dnegate);
	FTH_PRI1("dabs", ficl_dabs, h_dabs);
	FTH_PRI1("dmin", ficl_dmin, h_dmin);
	FTH_PRI1("dmax", ficl_dmax, h_dmax);
	FTH_PRI1("d2*", ficl_dtwostar, h_dtwostar);
	FTH_PRI1("d2/", ficl_dtwoslash, h_dtwoslash);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_LLONG, h_list_of_llong_functions);
	/* float */
	FTH_PRI1("float?", ficl_float_p, h_float_p);
	FTH_PRI1("inf?", ficl_inf_p, h_inf_p);
	FTH_PRI1("nan?", ficl_nan_p, h_nan_p);
	FTH_PRI1("inf", ficl_inf, h_inf);
	FTH_PRI1("nan", ficl_nan, h_nan);
	FTH_PRI1("f.r", ficl_f_dot_r, h_f_dot_r);
	FTH_PRI1("uf.r", ficl_uf_dot_r, h_uf_dot_r);
	FTH_PRI1("floats", ficl_dfloats, h_dfloats);
	FTH_PRI1("sfloats", ficl_dfloats, h_dfloats);
	FTH_PRI1("dfloats", ficl_dfloats, h_dfloats);
	FTH_PRI1("falign", ficl_falign, h_falign);
	FTH_PRI1("f>s", ficl_to_s, h_to_s);
	FTH_PRI1("s>f", ficl_to_f, h_to_f);
	FTH_PRI1("f**", ficl_fpow, h_fpow);
	FTH_PRI1("fpow", ficl_fpow, h_fpow);
	FTH_PRI1("fabs", ficl_fabs, h_fabs);
#if !HAVE_COMPLEX
	FTH_PRI1("magnitude", ficl_fabs, h_fabs);
#endif
	FTH_PRI1("fmod", ficl_fmod, h_fmod);
	FTH_PRI1("floor", ficl_floor, h_floor);
	FTH_PRI1("fceil", ficl_fceil, h_fceil);
	FTH_PRI1("ftrunc", ficl_ftrunc, h_ftrunc);
	FTH_PRI1("fround", ficl_fround, h_fround);
	FTH_PRI1("fsqrt", ficl_fsqrt, h_fsqrt);
	FTH_PRI1("fexp", ficl_fexp, h_fexp);
	FTH_PRI1("fexpm1", ficl_fexpm1, h_fexpm1);
	FTH_PRI1("flog", ficl_flog, h_flog);
	FTH_PRI1("flogp1", ficl_flogp1, h_flogp1);
	FTH_PRI1("flog2", ficl_flog2, h_flog2);
	FTH_PRI1("flog10", ficl_flog10, h_flog10);
	FTH_PRI1("falog", ficl_falog, h_falog);
	FTH_PRI1("fsin", ficl_fsin, h_fsin);
	FTH_PRI1("fcos", ficl_fcos, h_fcos);
	FTH_PRI1("fsincos", ficl_fsincos, h_fsincos);
	FTH_PRI1("ftan", ficl_ftan, h_ftan);
	FTH_PRI1("fasin", ficl_fasin, h_fasin);
	FTH_PRI1("facos", ficl_facos, h_facos);
	FTH_PRI1("fatan", ficl_fatan, h_fatan);
	FTH_PRI1("fatan2", ficl_fatan2, h_fatan2);
	FTH_PRI1("fsinh", ficl_fsinh, h_fsinh);
	FTH_PRI1("fcosh", ficl_fcosh, h_fcosh);
	FTH_PRI1("ftanh", ficl_ftanh, h_ftanh);
	FTH_PRI1("fasinh", ficl_fasinh, h_fasinh);
	FTH_PRI1("facosh", ficl_facosh, h_facosh);
	FTH_PRI1("fatanh", ficl_fatanh, h_fatanh);
/* math.h */
#if !defined(M_E)
#define M_E			2.7182818284590452354	/* e */
#endif
#if !defined(M_LN2)
#define M_LN2			0.69314718055994530942	/* log(2) */
#endif
#if !defined(M_LN10)
#define M_LN10			2.30258509299404568402	/* log(10) */
#endif
#if !defined(M_PI)
#define M_PI			3.14159265358979323846	/* pi */
#endif
#if !defined(M_PI_2)
#define M_PI_2			1.57079632679489661923	/* pi/2 */
#endif
#if !defined(M_TWO_PI)
#define M_TWO_PI		(M_PI * 2.0)	/* pi*2 */
#endif
#if !defined(M_SQRT2)
#define M_SQRT2			1.41421356237309504880	/* sqrt(2) */
#endif
	fth_define("euler", fth_make_float(M_E));
	fth_define("ln-two", fth_make_float(M_LN2));
	fth_define("ln-ten", fth_make_float(M_LN10));
	fth_define("pi", fth_make_float(M_PI));
	fth_define("two-pi", fth_make_float(M_TWO_PI));
	fth_define("half-pi", fth_make_float(M_PI_2));
	fth_define("sqrt-two", fth_make_float(M_SQRT2));
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_FLOAT, h_list_of_float_functions);
	/* complex */
	FTH_PRI1("complex?", ficl_complex_p, h_complex_p);
	FTH_PRI1("real-ref", ficl_creal, h_creal);
	FTH_PRI1("imag-ref", ficl_cimage, h_cimage);
	FTH_PRI1("image-ref", ficl_cimage, h_cimage);
#if HAVE_COMPLEX
	FTH_PRI1("make-rectangular", ficl_make_complex_rectangular,
	    h_make_complex_rectangular);
	FTH_PRI1(">complex", ficl_make_complex_rectangular,
	    h_make_complex_rectangular);
	FTH_PRI1("make-polar", ficl_make_complex_polar,
	    h_make_complex_polar);
	FTH_PRI1("c.", ficl_c_dot, h_c_dot);
	FTH_PRI1("s>c", ficl_to_c, h_to_c);
	FTH_PRI1("c>s", ficl_to_s, h_to_s);
	FTH_PRI1("f>c", ficl_to_c, h_to_c);
	FTH_PRI1("c>f", ficl_to_f, h_to_f);
	FTH_PRI1("q>c", ficl_to_c, h_to_c);
	FTH_PRI1("r>c", ficl_to_c, h_to_c);
	FTH_PRI1(">c", ficl_to_c, h_to_c);
	FTH_PRI1("c0=", ficl_ceqz, h_ceqz);
	FTH_PRI1("c0<>", ficl_cnoteqz, h_cnoteqz);
	FTH_PRI1("c=", ficl_ceq, h_ceq);
	FTH_PRI1("c<>", ficl_cnoteq, h_cnoteq);
	FTH_PRI1("c+", ficl_cadd, h_cadd);
	FTH_PRI1("c-", ficl_csub, h_csub);
	FTH_PRI1("c*", ficl_cmul, h_cmul);
	FTH_PRI1("c/", ficl_cdiv, h_cdiv);
	FTH_PRI1("1/c", ficl_creciprocal, h_creciprocal);
	FTH_PRI1("carg", ficl_carg, h_carg);
	FTH_PRI1("cabs", ficl_cabs, h_cabs);
	FTH_PRI1("magnitude", ficl_cabs, h_cabs);
	FTH_PRI1("cabs2", ficl_cabs2, h_cabs2);
	FTH_PRI1("c**", ficl_cpow, h_cpow);
	FTH_PRI1("cpow", ficl_cpow, h_cpow);
	FTH_PRI1("conj", ficl_cconj, h_cconj);
	FTH_PRI1("conjugate", ficl_cconj, h_cconj);
	FTH_PRI1("csqrt", ficl_csqrt, h_csqrt);
	FTH_PRI1("cexp", ficl_cexp, h_cexp);
	FTH_PRI1("clog", ficl_clog, h_clog);
	FTH_PRI1("clog10", ficl_clog10, h_clog10);
	FTH_PRI1("csin", ficl_csin, h_csin);
	FTH_PRI1("ccos", ficl_ccos, h_ccos);
	FTH_PRI1("ctan", ficl_ctan, h_ctan);
	FTH_PRI1("casin", ficl_casin, h_casin);
	FTH_PRI1("cacos", ficl_cacos, h_cacos);
	FTH_PRI1("catan", ficl_catan, h_catan);
	FTH_PRI1("catan2", ficl_catan2, h_catan2);
	FTH_PRI1("csinh", ficl_csinh, h_csinh);
	FTH_PRI1("ccosh", ficl_ccosh, h_ccosh);
	FTH_PRI1("ctanh", ficl_ctanh, h_ctanh);
	FTH_PRI1("casinh", ficl_casinh, h_casinh);
	FTH_PRI1("cacosh", ficl_cacosh, h_cacosh);
	FTH_PRI1("catanh", ficl_catanh, h_catanh);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_COMPLEX, h_list_of_complex_functions);
#endif				/* HAVE_COMPLEX */
	/* bignum */
	FTH_PRI1("bignum?", ficl_bignum_p, h_bignum_p);
#if HAVE_BN
	FTH_PROC("make-bignum", fth_make_big, 1, 0, 0, h_make_big);
	FTH_PROC(">bignum", fth_make_big, 1, 0, 0, h_make_big);
	FTH_PRI1("bn.", ficl_bn_dot, h_bn_dot);
	FTH_PROC("s>b", fth_make_big, 1, 0, 0, h_make_big);
	FTH_PRI1("b>s", ficl_to_s, h_to_s);
	FTH_PROC("f>b", fth_make_big, 1, 0, 0, h_make_big);
	FTH_PRI1("b>f", ficl_to_f, h_to_f);
	FTH_PRI1("b0=", ficl_beqz, h_beqz);
	FTH_PRI1("b0<>", ficl_bnoteqz, h_bnoteqz);
	FTH_PRI1("b0<", ficl_blessz, h_blessz);
	FTH_PRI1("b0>", ficl_bgreaterz, h_bgreaterz);
	FTH_PRI1("b0<=", ficl_blesseqz, h_blesseqz);
	FTH_PRI1("b0>=", ficl_bgreatereqz, h_bgreatereqz);
	FTH_PRI1("b=", ficl_beq, h_beq);
	FTH_PRI1("b<>", ficl_bnoteq, h_bnoteq);
	FTH_PRI1("b<", ficl_bless, h_bless);
	FTH_PRI1("b>", ficl_bgreater, h_bgreater);
	FTH_PRI1("b<=", ficl_blesseq, h_blesseq);
	FTH_PRI1("b>=", ficl_bgreatereq, h_bgreatereq);
	FTH_PRI1("b+", ficl_badd, h_badd);
	FTH_PRI1("b-", ficl_bsub, h_bsub);
	FTH_PRI1("b*", ficl_bmul, h_bmul);
	FTH_PRI1("b/", ficl_bdiv, h_bdiv);
	FTH_PRI1("b**", ficl_bpow, h_bpow);
	FTH_PRI1("bpow", ficl_bpow, h_bpow);
	FTH_PRI1("bnegate", ficl_bnegate, h_dnegate);
	FTH_PRI1("babs", ficl_babs, h_dabs);
	FTH_PRI1("bmin", ficl_bmin, h_dmin);
	FTH_PRI1("bmax", ficl_bmax, h_dmax);
	FTH_PRI1("b2*", ficl_btwostar, h_dtwostar);
	FTH_PRI1("b2/", ficl_btwoslash, h_dtwoslash);
	FTH_PRI1("blshift", ficl_blshift, h_blshift);
	FTH_PRI1("brshift", ficl_brshift, h_brshift);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_BIGNUM, h_list_of_bignum_functions);
#else				/* !HAVE_BN */
	/*
	 * If bn(3) isn't available, bignum words are aliased to Forth'
	 * double word set.
	 */
	FTH_PRI1("b=", ficl_deq, h_deq);
	FTH_PRI1("b<>", ficl_dnoteq, h_dnoteq);
	FTH_PRI1("b<", ficl_dless, h_dless);
	FTH_PRI1("b>", ficl_dgreater, h_dgreater);
	FTH_PRI1("b<=", ficl_dlesseq, h_dlesseq);
	FTH_PRI1("b>=", ficl_dgreatereq, h_dgreatereq);
	FTH_PRI1("b+", ficl_dadd, h_dadd);
	FTH_PRI1("b-", ficl_dsub, h_dsub);
	FTH_PRI1("b*", ficl_dmul, h_dmul);
	FTH_PRI1("b/", ficl_ddiv, h_ddiv);
	FTH_PRI1("bnegate", ficl_dnegate, h_dnegate);
	FTH_PRI1("babs", ficl_dabs, h_dabs);
	FTH_PRI1("bmin", ficl_dmin, h_dmin);
	FTH_PRI1("bmax", ficl_dmax, h_dmax);
	FTH_PRI1("b2*", ficl_dtwostar, h_dtwostar);
	FTH_PRI1("b2/", ficl_dtwoslash, h_dtwoslash);
#endif				/* HAVE_BN */
	/* ratio */
	FTH_PRI1("ratio?", ficl_ratio_p, h_ratio_p);
	FTH_PRI1("rational?", ficl_ratio_p, h_ratio_p);
#if HAVE_BN
	FTH_PROC("make-ratio", fth_make_ratio, 2, 0, 0, h_make_ratio);
	FTH_PRI1(">ratio", ficl_to_ratio, h_to_ratio);
	FTH_PRI1("rationalize", ficl_rationalize, h_rationalize);
	FTH_PRI1("q.", ficl_q_dot, h_q_dot);
	FTH_PRI1("r.", ficl_q_dot, h_q_dot);
	FTH_PRI1("s>q", ficl_s_to_q, h_s_to_q);
	FTH_PRI1("s>r", ficl_s_to_q, h_s_to_q);
	FTH_PRI1("q>s", ficl_to_s, h_to_s);
	FTH_PRI1("r>s", ficl_to_s, h_to_s);
	FTH_PRI1("c>q", ficl_f_to_q, h_f_to_q);
	FTH_PRI1("c>r", ficl_f_to_q, h_f_to_q);
	FTH_PRI1("f>q", ficl_f_to_q, h_f_to_q);
	FTH_PRI1("f>r", ficl_f_to_q, h_f_to_q);
	FTH_PRI1("q>f", ficl_to_f, h_to_f);
	FTH_PRI1("r>f", ficl_to_f, h_to_f);
	FTH_PRI1("q0=", ficl_qeqz, h_qeqz);
	FTH_PRI1("q0<>", ficl_qnoteqz, h_qnoteqz);
	FTH_PRI1("q0<", ficl_qlessz, h_qlessz);
	FTH_PRI1("q0>", ficl_qgreaterz, h_qgreaterz);
	FTH_PRI1("q0<=", ficl_qlesseqz, h_qlesseqz);
	FTH_PRI1("q0>=", ficl_qgreatereqz, h_qgreatereqz);
	FTH_PRI1("q=", ficl_qeq, h_qeq);
	FTH_PRI1("q<>", ficl_qnoteq, h_qnoteq);
	FTH_PRI1("q<", ficl_qless, h_qless);
	FTH_PRI1("q>", ficl_qgreater, h_qgreater);
	FTH_PRI1("q<=", ficl_qlesseq, h_qlesseq);
	FTH_PRI1("q>=", ficl_qgreatereq, h_qgreatereq);
	FTH_PRI1("q+", ficl_qadd, h_qadd);
	FTH_PRI1("q-", ficl_qsub, h_qsub);
	FTH_PRI1("q*", ficl_qmul, h_qmul);
	FTH_PRI1("q/", ficl_qdiv, h_qdiv);
	FTH_PRI1("r+", ficl_qadd, h_qadd);
	FTH_PRI1("r-", ficl_qsub, h_qsub);
	FTH_PRI1("r*", ficl_qmul, h_qmul);
	FTH_PRI1("r/", ficl_qdiv, h_qdiv);
	FTH_PRI1("q**", ficl_fpow, h_fpow);
	FTH_PRI1("qpow", ficl_fpow, h_fpow);
	FTH_PRI1("r**", ficl_fpow, h_fpow);
	FTH_PRI1("rpow", ficl_fpow, h_fpow);
	FTH_PRI1("qnegate", ficl_qnegate, h_dnegate);
	FTH_PRI1("rnegate", ficl_qnegate, h_dnegate);
	FTH_PRI1("qfloor", ficl_qfloor, h_qfloor);
	FTH_PRI1("rfloor", ficl_qfloor, h_qfloor);
	FTH_PRI1("qceil", ficl_qceil, h_fceil);
	FTH_PRI1("rceil", ficl_qceil, h_fceil);
	FTH_PRI1("qabs", ficl_qabs, h_dabs);
	FTH_PRI1("rabs", ficl_qabs, h_dabs);
	FTH_PRI1("1/q", ficl_qinvert, h_qinvert);
	FTH_PRI1("1/r", ficl_qinvert, h_qinvert);
	FTH_ADD_FEATURE_AND_INFO(FTH_STR_RATIO, h_list_of_ratio_functions);
#endif				/* HAVE_BN */
	FTH_PROC("exact->inexact", fth_exact_to_inexact, 1, 0, 0,
	    h_exact_to_inexact);
	FTH_PROC("inexact->exact", fth_inexact_to_exact, 1, 0, 0,
	    h_inexact_to_exact);
	FTH_PROC("numerator", fth_numerator, 1, 0, 0, h_numerator);
	FTH_PROC("denominator", fth_denominator, 1, 0, 0, h_denominator);
	FTH_PRI1("odd?", ficl_odd_p, h_odd_p);
	FTH_PRI1("even?", ficl_even_p, h_even_p);
	FTH_PRI1("prime?", ficl_prime_p, h_prime_p);
	/* From ficlSystemCompileCore(), ficl/primitive.c */
	env = ficlSystemGetEnvironment(FTH_FICL_SYSTEM());
	ficlDictionaryAppendConstant(env, "max-n",
	    (ficlInteger)fth_make_llong(LONG_MAX));
	ficlDictionaryAppendConstant(env, "max-u",
	    (ficlInteger)fth_make_ullong(ULONG_MAX));
	ficlDictionaryAppendConstant(env, "max-d",
	    (ficlInteger)fth_make_llong(LLONG_MAX));
	ficlDictionaryAppendConstant(env, "max-ud",
	    (ficlInteger)fth_make_ullong(ULLONG_MAX));
#if !defined(MAXFLOAT)
#define MAXFLOAT		((ficlFloat)3.40282346638528860e+38)
#endif
	ficlDictionaryAppendConstant(env, "max-float",
	    (ficlInteger)fth_make_float(MAXFLOAT));
}

/*
 * numbers.c ends here
 */
